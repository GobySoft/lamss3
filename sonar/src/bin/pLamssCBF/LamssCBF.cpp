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

#include "lamss/lib_anrp_util/sliva_parse.h"
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
      buff_index_(0),
      end_data_(0),
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

    // Processing block

     if(new_file_flag_ && got_filename_)
      {
	glog.is(DEBUG1) && glog << "Loading File " << sonar_filename_ << std::endl;
	//load the data file
	if ( LoadData(sonar_filename_) )
	  {	
	    glog.is(DEBUG2) && glog << "Begin Processing Data..." << std::endl;
	    //	    if ( !Process_Data() )
	    if ( 0 )
	      {
		glog.is(DEBUG1) && glog << "Processing failed!" << std::endl;
	      }
	    else
	      {
		num_files_processed_++;
		last_frame_processed_ = frame_number_;
		glog.is(DEBUG2) && glog << "Finished Processing Data File Number "
					<<  last_frame_processed_ << std::endl;
	        new_file_flag_ = false;
		return;
	      }
	  }
	else //Added 21 OCT 2010 AJP
	  {
	    glog.is(DEBUG1) && glog << "Loading Data Failed!" << std::endl;
	    new_file_flag_ = false;
	    return;
	  }
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

//***************************************************************************
//*  LamssCBF::LoadData(string filename)                                        *
//*                                                                         *
//*  LoadData() reads in the most recent sonar file.                        *
//*************************************************************************** 
bool LamssCBF::LoadData(std::string filename)
{	
    FILE *fp;
    char fnamechar[256];  
    int time0,time1;
int leftover = 0;  
    short *temp_int;
    float *temp_data;
  
    glog.is(DEBUG2) && glog << "Entering LoadData" << std::endl;
    sprintf(fnamechar,"%s",filename.c_str());
  
    if (num_files_processed_ > 0)
      {
	if (frame_number_==(last_frame_processed_+1))  //Previous and current data files contain contiguous timeseries data
	  {
	    glog.is(DEBUG2) && glog << "Saving leftover data from file number "
				    << intToString(last_frame_processed_)
				    << " before reading file number "
				    << intToString(frame_number_) << std::endl;
	    leftover = (end_data_ - buff_index_);
	    for (int uu = 1; uu <= leftover; uu++)
	      Array_Data.Row(uu) = Array_Data.Row(buff_index_+uu);
	    end_data_ = leftover + cfg_.number_samples(); 
	  }  
	else //Skipped one or more data files, so don't use any leftover data
	  {
	    glog.is(DEBUG2) && glog << "Non-consecutive acoustic data files (previous: "
				    << intToString(last_frame_processed_) 
				    << ", current: " 
				    << frame_number_ << std::endl;
	    leftover = 0;
	    end_data_ = cfg_.number_samples();
	  }
      }
    else
      {
        leftover = 0;
        end_data_ = cfg_.number_samples();
      }
  
    if ( cfg_.array_type() == LamssCBFConfig::SLITA ) 
      {
	glog.is(DEBUG2) && glog << "Reading SLITA File" << std::endl;
        SlivaReader *sr;
        try 
	{
	    sr = new SlivaReader(filename);
	} 
        catch(const exception &e) 
	{
	  glog.is(DEBUG1) && glog << "Couldn't load SLITA file "
				  << filename << std::endl;
	  return false;
	}
      
	glog.is(DEBUG2) && glog << "Sample rate " << intToString(sr->fs)
				<< ", ch sz " 
				<< intToString(sr->channel_data[1].size()) << std::endl;

        for (int kk=0; kk < cfg_.number_elements(); kk++) 
	  {
	    for (int jj=leftover; jj < end_data_; jj++) 
	      {
	        Array_Data(jj+1,kk+1) = (float)sr->channel_data[kk+1][jj-leftover].data;
	      }
	  }
      
        delete sr;
	
	glog.is(DEBUG2) && glog << "Done Reading SLITA File" << std::endl;
      } 
    else	
      {
	//open sample file
	if ((fp = fopen(fnamechar,"rb")) == NULL)
	  {
	    glog.is(DEBUG2) && glog << "Could not open sample file "
				    << filename << std::endl;
	    return false;
	  }
	
	// signals from all channels
	if ( cfg_.array_type() == LamssCBFConfig::VSA || 
	     cfg_.array_type() == LamssCBFConfig::DURIP || 
	     cfg_.array_type() == LamssCBFConfig::HYD )
	  { 
	    temp_data = new float [cfg_.number_samples()];
	    for (int kk=0; kk < cfg_.number_elements(); kk++)
	      {
		fread (temp_data, sizeof (float), cfg_.number_samples(), fp);
		for (int jj=leftover; jj < end_data_; jj++)
		  { 
		    Array_Data(jj+1,kk+1) = temp_data[jj-leftover];
		  }
	      }
	    delete [] temp_data;
	  }
	else if ( cfg_.array_type() == LamssCBFConfig::SINGLE || 
		  cfg_.array_type() == LamssCBFConfig::DUAL || 
		  cfg_.array_type() == LamssCBFConfig::TEE )
	  {
	    temp_int = new short [cfg_.number_samples()];
	    //read the two timetags out of the file
	    fread(&time0,sizeof(int),1,fp);
	    fread(&time1,sizeof(int),1,fp);
	    for (int kk=0; kk < cfg_.number_elements(); kk++)
	      {
		fread(temp_int,sizeof(short),cfg_.number_samples(),fp);
		for (int jj=leftover; jj < end_data_; jj++)
		  { 
		    Array_Data(jj+1,kk+1) = temp_int[jj-leftover];
		  }
	      }
	    delete [] temp_int;
	  }
	
	fclose(fp);
	
	glog.is(DEBUG2) && glog << "Done reading sample file "
				<< filename << std::endl;
	
	// F. Baralli Data file is cancelled after reading to save disk space
	//	if(bDelFileAfterProc==true)
	//	{
	//		int iRet = 0;
	//		string sTmp = "rm ";
	//		
	//		sTmp.append(fnamechar, strlen(fnamechar));
	//		iRet = system(sTmp.c_str());
	//		if(iRet==0)
	//			MOOSTrace("Data file %s cancelled\n",fnamechar);
	//		else
	//			MOOSTrace("Failed cancelling %s data file\n",fnamechar);
	//	}
	//	End F. Baralli
	
	// Following code added by K. Cockrell on Aug 6 2008, 
	// to eliminate bad DURIP array data.
	// When data is bad, many consecutive zeros occur on all channels at once,
	// preceded by a large spike.
	// So here, we'll choose a arbitrary channel, and if we find consecutive zeros, 
	// then we'll output a moos variable letting us know it is bad, 
	// and replace the spike. This code only runs if
	// def_check_for_consec_zeros is set to true in the .moos file 
	if (cfg_.check_consecutive_zeros())
	  {
	    // Choose an arbitrary channel to look for consecutive zeros.
	    int arb_channel = 1;
	    int cons_zero = 0;
	    int max_zeros = 10;
	    // Loop through each time sample on channel arb_channel looking 
	    // for consecutive zeros. 
	    // AJP Changed "leftover + 1" to "leftover", 26 NOV 2010
	    for (int jj=leftover; jj < end_data_; jj++)
	      {
		if(Array_Data(jj+1, arb_channel) == 0)
		  cons_zero = cons_zero + 1;
		else
		  cons_zero = 0;
		
		if (cons_zero > 10)
		  {
		    glog.is(DEBUG1) && glog << "Warning: Acoustic data contains more "
					    << "than 10 consecutive zeros" << std::endl;
		    jj = end_data_; // If we have lots of zeros, then just stop checking.
		  }
	      }
	  }
      }
    glog.is(DEBUG2) && glog << "Done with LoadData" << std::endl;
    
    return true;
}

// >>>>>>>>>>>>>>> vector utility methods <<<<<<<<<<<<
  
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


