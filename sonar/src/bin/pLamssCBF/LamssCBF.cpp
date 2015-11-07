// Copyright 2015 
// Henrik Schmidt
// Massachusetts Institute of Technology 

#include <dlfcn.h>

#include <numeric>

#include <boost/thread.hpp>

#include "goby/common/logger.h"
#include "goby/acomms/connect.h"
#include "goby/common/time.h"

#include "LamssCBF.h"

#include "goby/moos/moos_protobuf_helpers.h"

using namespace lamss::shared::fsm;
using namespace goby::common::logger;
using namespace goby::common::logger_lock;
namespace gpb = goby::moos::protobuf;
using goby::common::goby_time;
using goby::glog;

const double pi = 3.14159;

LamssCBF* LamssCBF::inst_ = 0;
void* LamssCBF::driver_library_handle_= 0;

int main(int argc, char * argv[])
{
    return goby::moos::run<LamssCBF>(argc, argv);
}

LamssCBF* LamssCBF::get_instance()
{
    if(!inst_)    {
        lamss::shared::fsm::cfg_.reset(new LamssCBFConfig);
        inst_ = new LamssCBF(*lamss::shared::fsm::cfg_);
    }
    return inst_;
}

LamssCBF::LamssCBF(LamssCBFConfig& cfg)
    : GobyMOOSApp(&cfg),
      cfg_(cfg),
      scheduler_(true),
      fsm_(scheduler_.create_processor<LamssCBFFSM>()),
      sonar_on_(false),
      processing_on_(false),
      frame_number_(0),
      num_blocks_integrated_(0),
      num_files_processed_(0),
      got_filename_(false),
      sound_speed_(cfg_.sound_speed()),
      last_slow_loop_t_(0)
{

    goby::glog.set_lock_action(lock);
    goby::glog.add_group("fsm", goby::common::Colors::lt_magenta);
    goby::glog.add_group("helm", goby::common::Colors::lt_green);
    goby::glog.add_group("mac", goby::common::Colors::lt_blue);
    goby::glog.add_group("track", goby::common::Colors::yellow);

    set_ignore_stale(false);

    // set up subscriptions before starting separate thread
    subscribe("SONAR_CONTROL", &LamssCBF::handle_sonar_control, this);    
    subscribe("SONAR_PROCESS", &LamssCBF::handle_sonar_process, this);    
    subscribe("SONAR_FILENAME", &LamssCBF::handle_sonar_filename, this);    
    subscribe("SOUND_SPEED", &LamssCBF::handle_sonar_filename, this);    

    scheduler_.initiate_processor(fsm_);  
  
    fsm_thread_.reset(new boost::thread(boost::bind(&boost::statechart::fifo_scheduler<>::operator(), &scheduler_, 0)));
  
    glog.is(VERBOSE) && glog << cfg_.DebugString() << std::endl;

    // initialize beamforming variables
    update_array_configuration();
    if (cfg_.processing_type() == LamssCBFConfig::BROADBAND)
      handle_fc_bw();
    else if (cfg_.processing_type() == LamssCBFConfig::TONES)
      handle_bin_list(cfg_.frequency_bins());
    update_beamforming_vars();
}

void LamssCBF::loop()
{
  //  boost::lock_guard<boost::mutex> lock(moos_mutex_);

    double curr_time = MOOSTime();

    // set time for fsm
    current_time_ = curr_time;

    try_publish(); // publish messages queued from state machine

    if (processing_on_ && sonar_on_)
      {
	boost::lock_guard<boost::mutex> lock(moos_mutex_);
	if (!processing_enable_)
	  {
	    processing_enable_ = true;
	    processing_cond_.notify_one();
	    // add Broadband/Tones logic
	    queue_event<EvToBroadband>();
	  }
      }
    else
      {
	boost::lock_guard<boost::mutex> lock(moos_mutex_);
	processing_enable_ = false;
	if (processing_active_)
	  queue_event<EvProcessingOff>();
      }

// process some events at a slower rate
    const int slow_loop_period = 10;
    if(curr_time > last_slow_loop_t_ + slow_loop_period)
        slow_loop(curr_time);
}

void LamssCBF::slow_loop(double curr_time)
{
  // Here we should add slow loop actions.

  last_slow_loop_t_ = curr_time;
}

void LamssCBF::try_publish()
{
    boost::mutex::scoped_try_lock lk(moos_mutex_);
    if(lk.owns_lock())   
    {
        while(!publish_queue_.empty())
	{
            glog.is(DEBUG2) && glog << "Publishing: " << publish_queue_.front().GetKey() << ": " << publish_queue_.front().GetAsString()  << std::endl;
            publish(publish_queue_.front());
            publish_queue_.pop_front();
	}
    }
}

void LamssCBF::handle_sonar_control(const CMOOSMsg& msg)
{
  std::string sonar_control = msg.GetString();
  sonar_on_ = MOOSStrCmp(sonar_control,"ON");
}

void LamssCBF::handle_sonar_process(const CMOOSMsg& msg)
{
  std::string sonar_process = msg.GetString();
  processing_on_ = MOOSStrCmp(sonar_process,"ON");
}

void LamssCBF::handle_sonar_filename(const CMOOSMsg& msg)
{
  sonar_filename_ = msg.GetString();
  got_filename_ = true;
}

void LamssCBF::handle_sound_speed(const CMOOSMsg& msg)
{
  sound_speed_ = msg.GetDouble();
}

void LamssCBF::update_array_configuration()
{
  string token;
  string sfirst;
  string slast;
  int first_temp;
  int last_temp;
  vector<int> first_channels_used;
  vector<int> last_channels_used;
  
  first_channels_used.clear();
  last_channels_used.clear();

  int sum_channels_used  = 0;
  std::string schannels_used;
  std::string sChannels = cfg_.array_elements_used();
  schannels_used = MOOSChomp(sChannels,",");
  while(!schannels_used.empty())
    {
      token = schannels_used;
      sfirst = MOOSChomp(token,":");
      slast = MOOSChomp(token);
      if(slast.empty())
	slast=sfirst;
    	first_temp = atoi(sfirst.c_str());
    	last_temp = atoi(slast.c_str());
    	first_channels_used.push_back(first_temp );
    	last_channels_used.push_back(last_temp);
    	sum_channels_used += (last_temp-first_temp+1);        
    	schannels_used = MOOSChomp(sChannels,",");
    }
  int num_channel_blocks=first_channels_used.size();
  
  num_elements_used_ = sum_channels_used;
  array_elements_used = new int[num_elements_used_+1];
  
  int chused_vec_idx = 1;
  for (int k=0; k<num_channel_blocks; k++)
    {	
      for(int i = 1;i <= (last_channels_used[k]-first_channels_used[k]+1);i++)
	{
	  array_elements_used[chused_vec_idx] = first_channels_used[k] + i-1;
	  chused_vec_idx +=1;
	} 
    }
}

void LamssCBF::handle_bin_list(std::string sbin_list)
{
  int sum_bins  = 0;
  string sbins;
  string token;
  string sfirst;
  string slast;
  int first_temp;
  int last_temp;
  vector<int> first;
  vector<int> last;
  
  first.clear();
  last.clear();
  
  sbins = MOOSChomp(sbin_list,",");
  while(!sbins.empty())
    {
      token = sbins;
      sfirst = MOOSChomp(token,":");
      slast = MOOSChomp(token);
      if(slast.empty())
	slast=sfirst;
      first_temp = atoi(sfirst.c_str());
      last_temp = atoi(slast.c_str());
      first.push_back(first_temp );
      last.push_back(last_temp);
      sum_bins += (last_temp-first_temp+1);        
      sbins = MOOSChomp(sbin_list,",");
    }
  int num_blocks=first.size();
  num_bins_ = sum_bins;
  bins = new int[num_bins_+1];
  int bin_vec_idx = 1;
  for(int k=0; k<num_blocks; k++)
    {	
      for(int i = 1;i <= (last[k]-first[k]+1);i++)
	{
	  bins[bin_vec_idx] = first[k] + i-1;
	  bin_vec_idx +=1;
	} 
    }
}

void LamssCBF::handle_fc_bw()
{

  int center_bin = 1+round(cfg_.center_frequency()*cfg_.fft_size()/
			   cfg_.sampling_frequency());
  int num_sidebins = (int) ceil(cfg_.bandwidth()*0.5/
				(cfg_.sampling_frequency()/cfg_.fft_size()) );
  
  num_bins_ = 2*num_sidebins + 1;
  bins = new int[num_bins_+1];
  for(int k = 1; k <=num_bins_; k++)
    bins[k] = (center_bin - num_sidebins + k - 1);  
}

void LamssCBF::update_beamforming_vars()
{
    double mean_frequency=0.0;
    for(int k = 1;k <=num_bins_;k++)
    {
      double freq=(cfg_.sampling_frequency()*(bins[k]-1))/cfg_.fft_size(); 
      mean_frequency += freq;
    }
    mean_frequency = mean_frequency/num_bins_;
    glog.is(DEBUG1) && glog << "Average Processing Frequency = " 
			    << doubleToString(mean_frequency,1) << " Hz"
			    << std::endl;
    double mean_element_spacing = 0.0;
    for(int k = 2; k <=num_elements_used_; k++)
      mean_element_spacing+=(nominal_relative_array_element_locations(array_elements_used[k]) -
			     nominal_relative_array_element_locations(array_elements_used[k-1]));
    mean_element_spacing=mean_element_spacing/(num_elements_used_ - 1);
  // New PB format
  BeamformingVars beamforming_vars;
  beamforming_vars.set_fft_size(cfg_.fft_size());
  beamforming_vars.set_number_beams(cfg_.number_beams());
  beamforming_vars.set_overlap(cfg_.overlap());
  beamforming_vars.set_integration_time(cfg_.integration_time());
  beamforming_vars.set_sound_speed(sound_speed_);
  beamforming_vars.set_sampling_frequency(cfg_.sampling_frequency());
  beamforming_vars.set_num_elements_used(num_elements_used_);
  beamforming_vars.set_mean_frequency(mean_frequency);
  beamforming_vars.set_mean_element_spacing(mean_element_spacing);
  // keep old var/val format until pBearingTrack converted to PB
  std::vector<double> sampling_vars;
  sampling_vars.clear();
  sampling_vars.push_back((double) cfg_.fft_size());
  sampling_vars.push_back((double) cfg_.number_beams());
  sampling_vars.push_back(cfg_.overlap());
  sampling_vars.push_back(cfg_.integration_time());
  sampling_vars.push_back(sound_speed_);
  sampling_vars.push_back(cfg_.sampling_frequency());
  sampling_vars.push_back((double) num_elements_used_);
  sampling_vars.push_back(mean_frequency);
  sampling_vars.push_back(mean_element_spacing);

  string s_sampling;
  dbl_vec2str(sampling_vars,s_sampling);
  m_Comms.Notify("BEAMFORMING_VARS",s_sampling);

}

double LamssCBF::angle360(double angle)
{
  double res = angle;
  //  unwrap angles
  while (res >= 360.0)
    res -= 360.0;
  while (res < 0.0)
    res += 360.0;
  
  return(res);
  
}

// >>>>>>>>>>>>>>> vector utility methods <<<<<<<<<<<<
int LamssCBF::str2dbl_vec(std::string msg, std::vector<double>& values)
{
  std::string bite;
  
  values.clear();
  int nz = 0;
  while (msg.length() > 0)
    {
      bite = MOOSChomp(msg,",");
      values.push_back(atof(bite.c_str()));
      nz++;
    }
  
  return(nz);
}

bool LamssCBF::dbl_vec2str(std::vector<double> numbers, std::string& output)
{
  output = "";
  for(int i=0; i<numbers.size(); i++) 
    {
      if(i > 0)
	output += ",";
      std::string istr = doubleToString(numbers[i], 2); // 2 is the precision 123.45
      output += istr;
    }
  return(true);
}


