// Copyright 2015 
// Henrik Schmidt
// Massachusetts Institute of Technology 

#include <dlfcn.h>

#include <numeric>

#include <boost/thread.hpp>

#include "goby/util/debug_logger.h"
#include "goby/acomms/connect.h"
#include "goby/time/legacy.h"

#include "LamssCBF.h"

#include "lamss/lib_anrp_util/sliva_parse.h"
#include "goby/moos/moos_protobuf_helpers.h"

using namespace lamss::shared::fsm;
using namespace goby::util::logger;
using namespace goby::util::logger_lock;
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
    : goby::moos::GobyMOOSApp(&cfg),
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
      homing_mode_active_(false),
      filter_length_(15),
      filt_set_(false),
      got_pitch_(false),
      prev_steering_pitch_(0),
      last_slow_loop_t_(0)
{

    goby::glog.set_lock_action(goby::util::logger_lock::lock);
    goby::glog.add_group("fsm", goby::util::Colors::lt_magenta);
    goby::glog.add_group("helm", goby::util::Colors::lt_green);
    goby::glog.add_group("mac", goby::util::Colors::lt_blue);
    goby::glog.add_group("track", goby::util::Colors::yellow);

    set_ignore_stale(false);

    // set up subscriptions before starting separate thread
    subscribe("SONAR_CONTROL", &LamssCBF::handle_sonar_control, this);    
    subscribe("SONAR_PROCESS", &LamssCBF::handle_sonar_process, this);    
    subscribe("SONAR_PROCESSING_COMMAND", 
	      &LamssCBF::handle_sonar_processing_command, this);    
    subscribe("SONAR_FILENAME", &LamssCBF::handle_sonar_filename, this);    
    subscribe("SONAR_DIR", &LamssCBF::handle_sonar_dir, this);    
    subscribe("SONAR_FILE_NUM", &LamssCBF::handle_sonar_file_num, this);    
    subscribe("SOUND_SPEED", &LamssCBF::handle_sound_speed, this);    
    subscribe(cfg_.ael_pitch_variable(), &LamssCBF::handle_ael_pitch, this);    
    subscribe("HOMING_BEACON_FREQUENCY", 
	      &LamssCBF::handle_homing_beacon_frequency, this);    
    subscribe("HOMING_ACTIVE", 
	      &LamssCBF::handle_homing_mode, this);    
    subscribe("VEHICLE_ID", &LamssCBF::handle_vehicle_id, this);
    subscribe("LAMSS_PROSECUTE_IN", &LamssCBF::handle_prosecute, this);
    subscribe("LAMSS_PROCESSING_IN", &LamssCBF::handle_processing, this);
    subscribe("LAMSS_DEPLOY_IN", &LamssCBF::handle_deploy, this);

    scheduler_.initiate_processor(fsm_);  
  
    fsm_thread_.reset(new boost::thread(boost::bind(&boost::statechart::fifo_scheduler<>::operator(), &scheduler_, 0)));
  
    glog.is(VERBOSE) && glog << cfg_.DebugString() << std::endl;

    // Dynamic configuration enabled
    curr_cfg_ = cfg_;
    // initialize beamforming variables
    array_configuration();
    // initialize beamformer data structures
    initialize_beamformer();
    if (curr_cfg_.processing_type() == LamssCBFConfig::BROADBAND)
      handle_fc_bw();
    else if (curr_cfg_.processing_type() == LamssCBFConfig::TONES)
      handle_freq_list(curr_cfg_.frequency_bins());
    // update data structures depending on num_bins_
    update_beamforming_vars();
    // compute steering vectors
    compute_steering_vectors();

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
	    if (curr_cfg_.processing_type() == LamssCBFConfig::TONES)
	      queue_event<EvToTones>();
	    else if (curr_cfg_.processing_type() == LamssCBFConfig::BROADBAND)
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
    if (sonar_on_ && processing_on_)
      {
	if (curr_cfg_.angular_steering_type() != LamssCBFConfig::ARRAY_ANGLE)
	  {
	    // recompute steering_vectors if pitch changed by more than 1 deg
	    if (fabs(pitch_filter.getAvg()-prev_steering_pitch_) >= M_PI/180)
	      compute_steering_vectors();
	  }
	
	if(new_file_flag_ && got_filename_)
	  {
	    glog.is(DEBUG1) && glog << "Loading File " << sonar_filename_ << std::endl;
	    //load the data file
	    if ( LoadData(sonar_filename_) )
	      {	
		glog.is(DEBUG1) && glog << "Begin Processing Data..." << std::endl;
		if ( !Process_Data() )
		  {
		    glog.is(DEBUG1) && glog << "Processing failed!" << std::endl;
		  }
		else
		  {
		    num_files_processed_++;
		    last_frame_processed_ = frame_number_;
		    glog.is(DEBUG1) && glog << "Finished Processing Data File Number "
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

void LamssCBF::handle_sonar_processing_command(const CMOOSMsg& msg)
{
  SonarProcessingCommand processing_cmd;
  parse_for_moos(msg.GetString(), &processing_cmd);
  last_processing_cmd_ = processing_cmd;

  transmit_elevation_angle_ = processing_cmd.transmit_elevation_angle();
  pingtime_ = processing_cmd.pingtime();
  recordtime_ = processing_cmd.recordtime();
  proc_offset_ = processing_cmd.proc_offset();
  proc_duration_ = processing_cmd.proc_duration();
  waveform_ = processing_cmd.waveform();
  ping_x_ = processing_cmd.ping_x();
  ping_y_ = processing_cmd.ping_y();
  ping_depth_ = processing_cmd.ping_depth();
  ping_speed_ = processing_cmd.ping_speed();
  ping_heading_ = processing_cmd.ping_heading();
  ping_pitch_ = processing_cmd.ping_pitch();

  num_samples_skip_ = ( processing_cmd.pingtime() - processing_cmd.recordtime() + 
			processing_cmd.proc_offset()) * curr_cfg_.sampling_frequency();
  num_samples_read_ = processing_cmd.proc_duration() * curr_cfg_.sampling_frequency();
}

void LamssCBF::handle_sonar_filename(const CMOOSMsg& msg)
{
  if (msg.GetString() != "")
    {
      sonar_filename_ = msg.GetString();
      glog.is(DEBUG1) && glog << "Got filename: " << sonar_filename_ << std::endl;
      got_filename_ = true;
      std::string tmp = sonar_filename_;
      sonar_basename_ = MOOSChomp(tmp,".");
    }
}

void LamssCBF::handle_sonar_dir(const CMOOSMsg& msg)
{
  sonar_dir_ = msg.GetString();
  got_dir_ = true;
}

void LamssCBF::handle_sonar_file_num(const CMOOSMsg& msg)
{
  int previous_frame = frame_number_;
  frame_number_ = (int) msg.GetDouble();
  if (frame_number_ != previous_frame)
    {
      new_file_flag_ =true;
      glog.is(DEBUG1) && glog << "Frame " << intToString(frame_number_) 
		      << " received" << std::endl;
    }
}

void LamssCBF::handle_sound_speed(const CMOOSMsg& msg)
{
  double sound_speed_prev = sound_speed_;
  sound_speed_ = msg.GetDouble();
  if (fabs(sound_speed_ - sound_speed_prev) > 0.0 )
    compute_steering_vectors();
}

void LamssCBF::handle_ael_pitch(const CMOOSMsg& msg)
{
  double raw_pitch = msg.GetDouble();
  if (!isnan(raw_pitch))
    {
      if (filt_set_)
	pitch_filter.setVal(raw_pitch);
      got_pitch_ = true;
    }
}

void LamssCBF::handle_homing_beacon_frequency(const CMOOSMsg& msg)
{
  homing_beacon_frequency_ = msg.GetDouble();
  //  homing_frequency_bin_ = (int) homing_beacon_frequency_*curr_cfg_.fft_size()/
  //    curr_cfg_.sampling_frequency() + 1;
  //  tone_freq_list_ = intToString(homing_frequency_bin_);
}

void LamssCBF::handle_homing_mode(const CMOOSMsg& msg)
{
  bool homing_processing_mode = MOOSStrCmp(tolower(msg.GetString()),"true");
  if (homing_processing_mode && (homing_beacon_frequency_ > 1.0))
    {
      tone_freq_list_ = doubleToString(homing_beacon_frequency_);
    }
  else
    curr_cfg_ = cfg_;
  if (homing_mode_active_ != homing_processing_mode)
    set_processing_mode(homing_processing_mode);
}

void LamssCBF::handle_deploy(const CMOOSMsg& msg)
{
  LAMSS_DEPLOY deploy_cmd;
  parse_for_moos(msg.GetString(), &deploy_cmd);
  if (deploy_cmd._rcv_id() == vehicle_id_)
    {
      curr_cfg_ = cfg_;
      set_processing_mode(false);
    }
}

void LamssCBF::handle_prosecute(const CMOOSMsg& msg)
{
  LAMSS_PROSECUTE prosecute_cmd;
  parse_for_moos(msg.GetString(), &prosecute_cmd);
  // Only handle messages for me
  curr_cfg_ = cfg_;
  if (prosecute_cmd._rcv_id() == vehicle_id_)
    {
      if (prosecute_cmd.processing_type() == LAMSS_PROSECUTE::TONES &&
	  prosecute_cmd.tones_size() > 0 )
	{
	  curr_cfg_.set_processing_type(LamssCBFConfig::TONES);
	  stringstream cc;
	  cc << doubleToString(prosecute_cmd.tones(0),1) ;
	  for (int i = 1; i < prosecute_cmd.tones_size(); i++)
	    cc << "," << doubleToString(prosecute_cmd.tones(i),1) ;

	  curr_cfg_.set_frequency_bins(cc.str());
	}
      else
	{
	  curr_cfg_.set_processing_type(LamssCBFConfig::BROADBAND);
	  curr_cfg_.set_center_frequency(prosecute_cmd.center_frequency());
	  curr_cfg_.set_bandwidth(prosecute_cmd.bandwidth());
	}
      set_processing_mode(false);
    }  
}

void LamssCBF::handle_processing(const CMOOSMsg& msg)
{
  LAMSS_PROCESSING processing_cmd;
  parse_for_moos(msg.GetString(), &processing_cmd);
  // Only handle messages for me
  curr_cfg_ = cfg_;
  if (processing_cmd._rcv_id() == vehicle_id_)
    {
      if (processing_cmd.processing_type() == LAMSS_PROCESSING::TONES &&
	  processing_cmd.tones_size() > 0 )
	{
	  curr_cfg_.set_processing_type(LamssCBFConfig::TONES);
	  stringstream cc;
	  cc << doubleToString(processing_cmd.tones(0),1) ;
	  for (int i = 1; i < processing_cmd.tones_size(); i++)
	    cc << "," << doubleToString(processing_cmd.tones(i),1) ;
	  curr_cfg_.set_frequency_bins(cc.str());
	}
      else
	{
	  curr_cfg_.set_processing_type(LamssCBFConfig::BROADBAND);
	  curr_cfg_.set_center_frequency(processing_cmd.center_frequency());
	  curr_cfg_.set_bandwidth(processing_cmd.bandwidth());
	}
      set_processing_mode(false);
    }  
}

void LamssCBF::handle_vehicle_id(const CMOOSMsg& msg)
{
    vehicle_id_ = msg.GetDouble();
}

void LamssCBF::set_processing_mode(bool homing_processing_mode)
{
  if (homing_processing_mode)
    {
      // Switch to tone mode, 
      homing_mode_active_ = true;
      curr_cfg_.set_processing_type(LamssCBFConfig::TONES);
      curr_cfg_.set_frequency_bins(tone_freq_list_);
      // delete bins array before creating new
      delete [] bins;
      handle_freq_list(tone_freq_list_);
      update_beamforming_vars();
      compute_steering_vectors();
      processing_enable_ = false;
      queue_event<EvProcessingReset>();
    }
  else if (!homing_processing_mode)
    {
      // Switch back to default processing mode
      homing_mode_active_ = false;
      delete [] bins;
      if (curr_cfg_.processing_type() == LamssCBFConfig::BROADBAND)
	handle_fc_bw();
      else if (curr_cfg_.processing_type() == LamssCBFConfig::TONES)
	handle_freq_list(curr_cfg_.frequency_bins());
      // update data structures depending on num_bins_
      update_beamforming_vars();
      // compute steering vectors
      compute_steering_vectors();
      processing_enable_ = false;
      queue_event<EvProcessingReset>();
    }
}

void LamssCBF::array_configuration()
{
  glog.is(DEBUG1) && glog << "Entering array_configuration " 
			  << std::endl; 

  string token;
  string sfirst;
  string slast;
  int first_temp;
  int last_temp;
  vector<int> first_channels_used;
  vector<int> last_channels_used;
  
  pitch_filter.configAvg(filter_length_);
  filt_set_ = true;

  first_channels_used.clear();
  last_channels_used.clear();

  int sum_channels_used  = 0;
  std::string schannels_used;
  std::string sChannels = curr_cfg_.array_elements_used();
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
  glog.is(DEBUG1) && glog << "num_elements_used = " << intToString(num_elements_used_)
			  << std::endl; 
  int chused_vec_idx = 1;
  for (int k=0; k<num_channel_blocks; k++)
    {	
      for(int i = 1;i <= (last_channels_used[k]-first_channels_used[k]+1);i++)
	{
	  array_elements_used[chused_vec_idx] = first_channels_used[k] + i-1;
	  chused_vec_idx +=1;
	} 
    }
  if (curr_cfg_.array_configuration() == LamssCBFConfig::UNIFORM_SPACING)
    {
      nominal_element_locations.ReSize(curr_cfg_.number_elements());
	for(int k=1;k<=curr_cfg_.number_elements();k++)
	  {
	    nominal_element_locations(k) = (double) (k-1)*curr_cfg_.element_spacing();
	  }
    }
  else if (curr_cfg_.array_configuration() == LamssCBFConfig::GROUP_SPACING)
    {
      vector<double> spacings;
      spacings.push_back(0.0);  //why is this here? Because #sensors = #spacings+1. HS
      double spac;
      if (curr_cfg_.number_spacing_groups() > 0)
	{
	  for (int i=0; i < curr_cfg_.number_spacings_group_1(); i++)
	    spacings.push_back(curr_cfg_.spacing_group_1());
	}
      if (curr_cfg_.number_spacing_groups() > 1)
	{
	  for (int i=0; i < curr_cfg_.number_spacings_group_2(); i++)
	    spacings.push_back(curr_cfg_.spacing_group_2());
	}
      if (curr_cfg_.number_spacing_groups() > 2)
	{
	  for (int i=0; i < curr_cfg_.number_spacings_group_3(); i++)
	    spacings.push_back(curr_cfg_.spacing_group_3());
	}
      cfg_.set_number_elements(spacings.size());
      curr_cfg_.set_number_elements(spacings.size());
      nominal_element_locations.ReSize(curr_cfg_.number_elements());
      nominal_element_locations(1)=spacings[0];
      for (int k=2; k <= curr_cfg_.number_elements(); k++)
	nominal_element_locations(k)=nominal_element_locations(k-1)+spacings[k-1];
    }
  else if (curr_cfg_.array_configuration() == LamssCBFConfig::ELEMENT_LOCATIONS)
    {
      vector<double> element_locations_temp;
      int num_elements=parseArrayCoordinates(curr_cfg_.array_element_locations(),
					     element_locations_temp);
      nominal_element_locations.ReSize(num_elements);
      for(int k=1; k<=num_elements; k++)
	nominal_element_locations(k) = element_locations_temp[k-1];      
      cfg_.set_number_elements(num_elements);
      curr_cfg_.set_number_elements(num_elements);
    }
  else if (curr_cfg_.array_configuration() == LamssCBFConfig::VOLUMETRIC)
    {
      // array_x (positive starboard)
      vector<double> element_locations_temp;
      int num_elements = parseArrayCoordinates(curr_cfg_.volumetric_array_x(),
					 element_locations_temp);
      array_x.ReSize(num_elements);
      for(int k=1; k<=num_elements; k++)
	array_x(k) = element_locations_temp[k-1];
      element_locations_temp.clear();
      // array_y (positive forward)
      int num_y = parseArrayCoordinates(curr_cfg_.volumetric_array_y(),
					element_locations_temp);
      if (num_y != num_elements)
	{
	  glog.is(VERBOSE) && glog << "num_element mismatch, using minimum" << std::endl;
	  num_elements = std::min(num_elements,num_y);
	} 
      array_y.ReSize(num_elements);
      for(int k=1; k<=num_elements; k++)
	array_y(k) = element_locations_temp[k-1];
      element_locations_temp.clear();
      // array_z (positive up)
      int num_z = parseArrayCoordinates(curr_cfg_.volumetric_array_z(),
					element_locations_temp);
      if (num_z != num_elements)
	{
	  glog.is(VERBOSE) && glog << "num_element mismatch, using minimum" << std::endl;
	  num_elements = std::min(num_elements,num_z);
	} 
      array_z.ReSize(num_elements);
      for(int k=1; k<=num_elements; k++)
	array_z(k) = element_locations_temp[k-1];
      element_locations_temp.clear();
      // set in number_elements current and original config 
      cfg_.set_number_elements(num_elements);
      curr_cfg_.set_number_elements(num_elements);
    }
}

size_t LamssCBF::parseArrayCoordinates(std::string str, vector<double> & element_coordinate)
{
    string temp;
    size_t num_values=0;
    temp = MOOSChomp(str,",");
    element_coordinate.clear();
    while (!temp.empty())
    { 
        element_coordinate.push_back(atof(temp.c_str()));
        num_values++;
        temp = MOOSChomp(str,",");
    }
    return(num_values);
}
  
//***************************************************************************
//*  LamssCBF::initialize_beamformer()                                                 *
//*                                                                         *
//*  Initialize all of the data structures and constants needed to process  *
//*  data, except those associated with num_bins_.                          *
//*                                                                         *
//*************************************************************************** 
bool LamssCBF::initialize_beamformer()
{
  glog.is(DEBUG1) && glog << "Entering initialize_beamformer" << std::endl;

  
  //calculate a few constants
  num_samples_skip_ = 0;
  num_samples_read_ = curr_cfg_.number_samples();

  num_overlapping_samples_=round((double)(curr_cfg_.overlap()*curr_cfg_.fft_size()));
  int_blocks_ = round((double)(curr_cfg_.integration_time()*
			       curr_cfg_.sampling_frequency()/
			       (curr_cfg_.fft_size()-num_overlapping_samples_)));
  actual_integration_ = int_blocks_*(curr_cfg_.fft_size()-num_overlapping_samples_)/
    curr_cfg_.sampling_frequency();
  
  //Set the size the NEWMAT arrays
  
  win.ReSize(curr_cfg_.fft_size());
  windowed_data.ReSize(curr_cfg_.fft_size());

  int vbeams;
  int hbeams;  
  if (curr_cfg_.angular_steering_type() == LamssCBFConfig::ARRAY_ANGLE)
    {
      vbeams = 1;
      hbeams = curr_cfg_.number_beams();
    }
  else
    {
      //Set the size of a few NEWMAT arrays 
      vbeams = curr_cfg_.number_beams_elevation();
      hbeams = curr_cfg_.number_beams_azimuth();
    }
  P.ReSize(vbeams,hbeams);
  P = 0.0;    //Initialize all elements of BTR vector to zero

  if (curr_cfg_.save_beam_stft())
    {
        Beam_STFT_real.resize(boost::extents[vbeams+1][hbeams+1]
			      [int_blocks_+1][num_bins_+1]);
        Beam_STFT_imag.resize(boost::extents[vbeams+1][hbeams+1]
			      [int_blocks_+1][num_bins_+1]);
    }


  beam_cosines.ReSize(curr_cfg_.number_beams());
  Array_Data.ReSize(curr_cfg_.number_samples()+curr_cfg_.fft_size(), 
		    curr_cfg_.number_elements());
   //  Spectrum_real.ReSize((curr_cfg_.fft_size()/2 + 1), num_elements_used_); 
  //  Spectrum_imag.ReSize((curr_cfg_.fft_size()/2 + 1), num_elements_used_);
  //Create time-domain window used before the DFT operation
  hanning(win.Store(),curr_cfg_.fft_size());
      
  steering_parameters();
  generate_spatial_window();

  time_window_constant_=0.0;
  for (int k = 1; k <= curr_cfg_.fft_size(); k++)
    time_window_constant_ += win(k)*win(k);
  
  // Hydrophone amplitude gain
  hydrophone_amplitude_gain_ = pow(10.0,curr_cfg_.hydrophone_gain()/20.0);

  hydrophone_calibration_ = 1.0/hydrophone_amplitude_gain_;
  
  //    glog << "Array Type = " << ,sArray.c_str());
  //glog << "Array Definition Type = %d\n",array_definition_type); 
  // glog << "Beam Sampling Type = ",sSampling.c_str());
  
  glog.is(DEBUG1) && glog << "time_window_constant_ = " 
			  << doubleToString(time_window_constant_) << std::endl;
  
  glog.is(DEBUG1) && glog << "Sampling Frequency = " 
			  << doubleToString(curr_cfg_.sampling_frequency()) << std::endl;
  glog.is(DEBUG1) && glog << "Number of Samples in Each File = "
			  << intToString(curr_cfg_.number_samples()) << std::endl;
  glog.is(DEBUG1) && glog << "Number of Array Elements = "
			  << intToString(curr_cfg_.number_elements()) << std::endl; 
  glog.is(DEBUG1) && glog << "Number of Array Elements Used = "
			  << intToString(num_elements_used_) << std::endl;
  glog.is(DEBUG1) && glog << "Sound Speed = " 
			  << doubleToString(sound_speed_,2) << std::endl;
  glog.is(DEBUG1) && glog << "Number of Beams = " 
			  << intToString(curr_cfg_.number_beams()) << std::endl ;	
  glog.is(DEBUG1) && glog << "FFT Length = " 
			  << intToString(curr_cfg_.fft_size()) << std::endl;
  glog.is(DEBUG1) && glog << "Overlap Fraction = "
			  << doubleToString(curr_cfg_.overlap(),1) << std::endl;
  glog.is(DEBUG1) && glog << "Number of Overlapping Samples " 
			  << intToString(num_overlapping_samples_) << std::endl;
  glog.is(DEBUG1) && glog << "Desired Integration Time = " 
			  << doubleToString(curr_cfg_.integration_time()) << std::endl;
  glog.is(DEBUG1) && glog << "Actual Integration Time = "
			  << doubleToString(actual_integration_,2) << std::endl;
  glog.is(DEBUG1) && glog << "Integration Blocks = "
			  << intToString(int_blocks_) << std::endl;
  glog.is(DEBUG1) && glog << "Hydrophone Amplitude Gain = "
			  << doubleToString(hydrophone_amplitude_gain_) << std::endl;
  
  if (curr_cfg_.array_configuration() != LamssCBFConfig::VOLUMETRIC)
    { 
      glog.is(DEBUG2) && glog << "Nominal Element Locations (1:"
			      << intToString(curr_cfg_.number_elements()) << std::endl;
      for (int k=1; k <= curr_cfg_.number_elements(); k++)
	glog.is(DEBUG2) && glog << "[" << intToString(k) << "] = "
				<< doubleToString(nominal_element_locations(k),2) 
				<< " m" << std::endl;
      glog.is(DEBUG2) && glog << "Array Elements Used (1:" 
			      << intToString(num_elements_used_) << std::endl;
      for(int k=1;k<=num_elements_used_;k++)
	glog.is(DEBUG2) && glog << "[" << intToString(k) << "] = "
				<< intToString(array_elements_used[k]) << std::endl;
    }

  glog.is(DEBUG2) && glog << "Beam Angles " 
			  << std::endl;
  for(int k=1;k<=curr_cfg_.number_beams();k++)
    glog.is(DEBUG2) && glog << "[" << intToString(k) << "] = "
			    << doubleToString(beam_angles[k],1) <<std::endl;
  
  return true;
}

bool LamssCBF::generate_spatial_window()
{
  double sum_win_spatial;

  ColumnVector win_spatial_temp2;
  ColumnVector win_spatial_temp;
  
  win_spatial.ReSize(num_elements_used_);
  
  win_spatial_temp.ReSize(curr_cfg_.number_elements());
  win_spatial_temp2.ReSize(curr_cfg_.number_elements());
  
  
    // Adjust spatial window to fit elements used for "SINGLE" and "DUAL" arrays.  Not yet implimented for other arrays.

    if (curr_cfg_.array_type() == LamssCBFConfig::SINGLE )
      {
	if(num_elements_used_ <= 8)
	  {
	    hanning(win_spatial_temp2.Store(),8);      
	    for (int k = 1; k <=8;k++)
	      {
		win_spatial_temp(2*k-1) = win_spatial_temp2(k);
		win_spatial_temp(2*k) = win_spatial_temp2(k);
	      }
	  }
	else 
	  hanning(win_spatial_temp.Store(),16);
	
	for (int k = 1; k <= num_elements_used_; k++)
	  win_spatial(k) = win_spatial_temp(array_elements_used[k]);
      }     
    else if (curr_cfg_.array_type() == LamssCBFConfig::TEE )
      {
	if(num_elements_used_ <= 8)
	  {
	    hanning(win_spatial_temp2.Store(),8);      
	    for (int k = 1; k <=8;k++)
	      {
		win_spatial_temp(k) = win_spatial_temp2(k);
	      }
	  }
	else 
	  hanning(win_spatial_temp.Store(),16);

	for (int k = 1; k <= num_elements_used_; k++)
	  win_spatial(k) = win_spatial_temp(array_elements_used[k]);
      }     
    else if (curr_cfg_.array_type() == LamssCBFConfig::DUAL )
      {
	hanning(win_spatial_temp2.Store(),8);
	for (int k = 1; k <=8;k++)
	  {
	    win_spatial_temp(k)   = win_spatial_temp2(k);
	    win_spatial_temp(k+8) = win_spatial_temp2(k);
	  }   
	for (int k = 1; k <= num_elements_used_; k++)
	    win_spatial(k) = win_spatial_temp(array_elements_used[k]);
      }
    else if (curr_cfg_.array_type() == LamssCBFConfig::SLITA || 
	     curr_cfg_.array_type() == LamssCBFConfig::HYD || 
	     curr_cfg_.array_type() == LamssCBFConfig::DURIP || 
	     curr_cfg_.array_type() == LamssCBFConfig::VSA ) 
      {
	hanning(win_spatial.Store(),num_elements_used_);
      }
    else if (curr_cfg_.array_type() == LamssCBFConfig::VOLUME )
      {
	for (int k = 1; k <= num_elements_used_; k++)
	  win_spatial(k) = 1.0;
      }

    sum_win_spatial=0.0;
    for (int k = 1; k <= num_elements_used_; k++)
      sum_win_spatial += win_spatial(k);
    win_spatial=win_spatial/sum_win_spatial;   
    return true;
}

//***************************************************************************
//*  LamssCBF::steering_parameters()                          *
//*                                                                         *
//*  Generates beam steering parameters.                                    *
//*************************************************************************** 
void LamssCBF::steering_parameters()
{
  beam_angles.clear();
  azimuthal_beams.ReSize(curr_cfg_.number_beams_azimuth()); 
  elevation_beams.ReSize(curr_cfg_.number_beams_elevation()); 
  
  if (curr_cfg_.angular_steering_type() == LamssCBFConfig::ARRAY_ANGLE)
    {
      double min_radians = 0;
      double max_radians = M_PI;
      
      double min_cos = cos(min_radians);
      double max_cos = cos(max_radians);
      
      if (curr_cfg_.sampling() == LamssCBFConfig::EQUALLY_SPACED_WAVENUMBER)
	{
	  for (int k=1; k <= curr_cfg_.number_beams(); k++)
	    beam_cosines(k)= min_cos - ((min_cos-max_cos)/(curr_cfg_.number_beams()-1))*(k-1);
	}
      else if (curr_cfg_.sampling() == LamssCBFConfig::EQUALLY_SPACED_THETA)
	{
	  for (int k=1; k <= curr_cfg_.number_beams(); k++)
	    beam_cosines(k)= cos(min_radians + (k-1) * (max_radians-min_radians)/
				 curr_cfg_.number_beams()-1 );
	}
      
      // Publish to MOOSDB for use by Tracker
      //    beam_angles.reserve(curr_cfg_.number_beams());
      beam_angles.clear();
      for (int mm=0; mm<curr_cfg_.number_beams();mm++)
	beam_angles.push_back(acos(beam_cosines(mm+1))*180.0/M_PI);

    }
  else if ((curr_cfg_.angular_steering_type() == LamssCBFConfig::AZIMUTH_ONLY) || 
	   (curr_cfg_.angular_steering_type() == LamssCBFConfig::AZIMUTH_AND_ELEVATION))  
    {
      if (curr_cfg_.number_beams_azimuth() == 1)
	azimuthal_beams(1)=curr_cfg_.max_azimuth()*M_PI/180.0;
      else
        {
	  double start_cos;
	  double end_cos;
	  
	  if (curr_cfg_.sampling() == LamssCBFConfig::EQUALLY_SPACED_WAVENUMBER)
            {
	      if (curr_cfg_.min_azimuth() < 0)
		start_cos=-cos(curr_cfg_.min_azimuth()*M_PI/180.0)+2;
	      else
		start_cos = cos(curr_cfg_.min_azimuth()*M_PI/180.0);
	      if (curr_cfg_.max_azimuth() < 0)
		end_cos=-cos(curr_cfg_.max_azimuth()*M_PI/180.0)+2;
	      else
		end_cos = cos(curr_cfg_.max_azimuth()*M_PI/180.0);
	      
	      for (int k=1; k <= curr_cfg_.number_beams_azimuth(); k++)
                {
		  double temp = start_cos - 
		    ((start_cos-end_cos)/(curr_cfg_.number_beams_azimuth()-1))*(k-1);
		  if ( temp > 1 )
		    azimuthal_beams(k) = - acos(-(temp-2));
		  else
		    azimuthal_beams(k)=acos(temp);
                }
            }
	  else if (curr_cfg_.sampling() == LamssCBFConfig::EQUALLY_SPACED_THETA)
            {
	      //Generate azimuthal beam look directions
	      for (int k=1; k <= curr_cfg_.number_beams_azimuth(); k++)
		azimuthal_beams(k) = M_PI/180.0*(curr_cfg_.min_azimuth() + (k-1) * 
				      (curr_cfg_.max_azimuth()-curr_cfg_.min_azimuth())/
				      (curr_cfg_.number_beams_azimuth()-1) );
            }
        }
      beam_angles.clear();
      for (int mm=0; mm<curr_cfg_.number_beams_azimuth();mm++)
	beam_angles.push_back(azimuthal_beams(mm+1)*180.0/M_PI);
    }
  else //ELEVATION_ONLY
    azimuthal_beams(1)=curr_cfg_.min_azimuth()*M_PI/180.0;
  
  // ELEVATION ANGLES

  if ((curr_cfg_.angular_steering_type() == LamssCBFConfig::ELEVATION_ONLY) || 
      (curr_cfg_.angular_steering_type() == LamssCBFConfig::AZIMUTH_AND_ELEVATION))  
    {
      if (curr_cfg_.number_beams_elevation() == 1)
	elevation_beams(1)=curr_cfg_.max_elevation();
      else
	{
	  double start_sin=sin(curr_cfg_.min_elevation()*M_PI/180.0);
	  double end_sin=sin(curr_cfg_.max_elevation()*M_PI/180.0);
	  if (curr_cfg_.sampling() == LamssCBFConfig::EQUALLY_SPACED_WAVENUMBER)
	    {
	      for (int k=1; k <= curr_cfg_.number_beams_elevation(); k++)
		elevation_beams(k)= asin(start_sin + (k-1) * 
					 (end_sin-start_sin)/
					 (curr_cfg_.number_beams_elevation()-1));
	    }
	  else if (curr_cfg_.sampling() == LamssCBFConfig::EQUALLY_SPACED_THETA)
	    {
	      for (int k=1; k <= curr_cfg_.number_beams_elevation(); k++)
		elevation_beams(k)=M_PI/180.0*(curr_cfg_.min_elevation() + 
					       (k-1)*(curr_cfg_.max_elevation()-
						      curr_cfg_.min_elevation())/
					       (curr_cfg_.number_beams_elevation()-1) );
	    }
	}
      if (curr_cfg_.angular_steering_type() == LamssCBFConfig::ELEVATION_ONLY)  
	{
	  beam_angles.clear();
	  for (int mm=0; mm<curr_cfg_.number_beams_elevation();mm++)
	    beam_angles.push_back(elevation_beams(mm+1)*180.0/M_PI);
	}
    }
  else //AZIMUTH_ONLY
      elevation_beams(1)=curr_cfg_.min_elevation() * M_PI/180.0;

  string sbeam_data;
  dbl_vec2str(beam_angles,sbeam_data);
  publish("BEAM_ANGLES",sbeam_data);
  
  return ;
}


void LamssCBF::handle_freq_list(std::string sfreq_list)
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
  double bin_factor = curr_cfg_.fft_size()/curr_cfg_.sampling_frequency();
  first.clear();
  last.clear();
  
  sbins = MOOSChomp(sfreq_list,",");
  while(!sbins.empty())
    {
      token = sbins;
      sfirst = MOOSChomp(token,":");
      slast = MOOSChomp(token);
      if(slast.empty())
	slast=sfirst;
      //      first_temp = atoi(sfirst.c_str());
      //      last_temp = atoi(slast.c_str());
      first_temp = floor(atof(sfirst.c_str())*bin_factor+1);
      last_temp = ceil(atof(slast.c_str())*bin_factor+1);
      first.push_back(first_temp );
      last.push_back(last_temp);
      sum_bins += (last_temp-first_temp+1);        
      sbins = MOOSChomp(sfreq_list,",");
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
  SteeringVectors.clear();

}

void LamssCBF::handle_fc_bw()
{

  int center_bin = 1+round(curr_cfg_.center_frequency()*curr_cfg_.fft_size()/
			   curr_cfg_.sampling_frequency());
  int num_sidebins = (int) ceil(curr_cfg_.bandwidth()*0.5/
				(curr_cfg_.sampling_frequency()/curr_cfg_.fft_size()) );
  
  num_bins_ = 2*num_sidebins + 1;
  bins = new int[num_bins_+1];
  for(int k = 1; k <=num_bins_; k++)
    bins[k] = (center_bin - num_sidebins + k - 1);  
  SteeringVectors.clear();
}

void LamssCBF::update_beamforming_vars()
{
  double mean_frequency=0.0;
  bin_center_frequencies.ReSize(num_bins_);
  for(int k = 1;k <=num_bins_;k++)
    {
      bin_center_frequencies(k)=(curr_cfg_.sampling_frequency()*(bins[k]-1))/
	curr_cfg_.fft_size(); 
      mean_frequency += bin_center_frequencies(k);
    }
  mean_frequency = mean_frequency/num_bins_;
  
  //RealSignals: flag indicating whether the array element time series are real (true)
  // or complex (false). Note that currently only real time series are supported.
  bool RealSignals=true;
  glog.is(DEBUG1) && glog << "Average Processing Frequency = " 
			  << doubleToString(mean_frequency,1) << " Hz"
			  << std::endl;
  //Set the size of the matrix which contains all of the steering vectors for all beams defined in 
  //beam_cosines and for all DFT bins of interest
  //size of S: (NUM_BEAMS*num_bins_) x NUM_ELEMENTS_USED 
  
  Spectrum_real.ReSize(num_bins_, num_elements_used_); 
  Spectrum_imag.ReSize(num_bins_, num_elements_used_);
  
  //Calculate normalization constant for BTRs
  
  if (RealSignals)
    {
 //     btr_normalization_=2.0/(curr_cfg_.sampling_frequency()*time_window_constant_ *
 //		      int_blocks_*num_bins_);
      single_bin_spectral_normalization_=2.0/(curr_cfg_.sampling_frequency()*
					      time_window_constant_);
    }
  else //complex signals (not currently supported)
    {	
 //     btr_normalization_=1.0/(curr_cfg_.sampling_frequency()*time_window_constant_ *
 //		      int_blocks_*num_bins_);
      single_bin_spectral_normalization_=1.0/(curr_cfg_.sampling_frequency()*
					      time_window_constant_);    
    }	
  
 // btr_normalization_=btr_normalization_/pow(hydrophone_amplitude_gain_,2);

  spectral_normalization_=single_bin_spectral_normalization_/(int_blocks_*num_bins_);
  single_bin_spectral_normalization_=sqrt(single_bin_spectral_normalization_);

  glog.is(DEBUG1) && glog << "Num Bins = " << intToString(num_bins_) << std::endl;
  glog.is(DEBUG1) && glog << "Integration Blocks = "
			  << intToString(int_blocks_) << std::endl;
  glog.is(DEBUG1) && glog << "time_window_constant_ = " 
			  << doubleToString(time_window_constant_) << std::endl;
  
  glog.is(DEBUG1) && glog << "spectral_normalization_ = " 
			  << doubleToString(spectral_normalization_) << std::endl;
  if (curr_cfg_.angular_steering_type() == LamssCBFConfig::ARRAY_ANGLE)
    {
      glog.is(DEBUG2) && glog << "Beam Cosines (1:" 
			      << intToString(curr_cfg_.number_beams()) << ")" << std::endl;
      for (int k=1; k <= curr_cfg_.number_beams(); k++)
	glog.is(DEBUG2) && glog << "[" << intToString(bins[k]) << "] = "
				<< doubleToString(beam_cosines(k),2)  
				<< std::endl;
    }
  glog.is(DEBUG2) && glog << "Processing Frequency Bins (1:"
			  << intToString(num_bins_) << ")" << std::endl;
  for(int j = 1;j <= num_bins_;j++)
    {
      double freq=(curr_cfg_.sampling_frequency()*(bins[j]-1))/curr_cfg_.fft_size();  
      glog.is(DEBUG2) && glog << "[" << intToString(bins[j]) << "] = "
			      << doubleToString(freq,2) << " Hz" << std::endl;
    }
  glog.is(DEBUG2) && glog << "Spatial Window (1:" 
			  << intToString(num_elements_used_) << ")" << std::endl;
  for (int k = 1; k <= num_elements_used_; k++)
    glog.is(DEBUG2) && glog << "[" << intToString(k) << "] = "
			    << doubleToString(win_spatial(k),2)  
			    << std::endl;
  
  double mean_element_spacing = 0.0;
  if (curr_cfg_.array_configuration() == LamssCBFConfig::VOLUMETRIC)
    mean_element_spacing=0.5*sound_speed_/mean_frequency;
  else
    {
      for(int k = 2; k <=num_elements_used_; k++)
	mean_element_spacing+=(nominal_element_locations(array_elements_used[k]) -
			       nominal_element_locations(array_elements_used[k-1]));
      mean_element_spacing=mean_element_spacing/(num_elements_used_ - 1);
    }

  // New PB format
  BeamformingVars beamforming_vars;
  beamforming_vars.set_fft_size(curr_cfg_.fft_size());
  beamforming_vars.set_number_beams(curr_cfg_.number_beams());
  beamforming_vars.set_overlap(curr_cfg_.overlap());
  beamforming_vars.set_integration_time(curr_cfg_.integration_time());
  beamforming_vars.set_sound_speed(sound_speed_);
  beamforming_vars.set_sampling_frequency(curr_cfg_.sampling_frequency());
  beamforming_vars.set_num_elements_used(num_elements_used_);
  beamforming_vars.set_mean_frequency(mean_frequency);
  beamforming_vars.set_mean_element_spacing(mean_element_spacing);
  std::string serialized;
  serialize_for_moos(&serialized,beamforming_vars);    
  publish("BEAMFORMING_VARS_PB", serialized);
  
  // keep old var/val format until pBearingTrack converted to PB
  std::vector<double> sampling_vars;
  sampling_vars.clear();
  sampling_vars.push_back((double) curr_cfg_.fft_size());
  sampling_vars.push_back((double) curr_cfg_.number_beams());
  sampling_vars.push_back(curr_cfg_.overlap());
  sampling_vars.push_back(curr_cfg_.integration_time());
  sampling_vars.push_back(sound_speed_);
  sampling_vars.push_back(curr_cfg_.sampling_frequency());
  sampling_vars.push_back((double) num_elements_used_);
  sampling_vars.push_back(mean_frequency);
  sampling_vars.push_back(mean_element_spacing);
  
  string s_sampling;
  dbl_vec2str(sampling_vars,s_sampling);
  publish("BEAMFORMING_VARS",s_sampling);

  
}

void LamssCBF::compute_steering_vectors()
{
  //
  // First check if we already got it
  //
  double ssp_tol = 5;
  double pitch_tol = M_PI/180.; // 1 deg
  // for array angle pitch does not matter
  if (curr_cfg_.angular_steering_type() == LamssCBFConfig::ARRAY_ANGLE)
    pitch_tol = M_PI/2;
  int iter = 0;
  bool got_it = false;
  double pitch = pitch_filter.getAvg();
  glog.is(DEBUG1) && glog << "Seeking Steering Vectors, pitch = "
			  << pitch << ", ssp = " << sound_speed_ << std::endl;
  while( iter < SteeringVectors.size() && !got_it)
    {
      got_it = SteeringVectors[iter].get(pitch, sound_speed_,
				   steering_vector_real, steering_vector_imag);
      if (got_it)
	glog.is(DEBUG1) && glog 
	  << "Found Steering Vectors, "
	  << " pitch = " << SteeringVectors[iter].pitch 
	  << ", ssp = " << SteeringVectors[iter].ssp 
	  << std::endl;
      iter++;
    }
  if (!got_it)
    {
      glog.is(DEBUG1) && glog << "Computing Steering Vectors, pitch = "
			      << pitch << ", ssp = " << sound_speed_ << std::endl;
      if (curr_cfg_.angular_steering_type() == LamssCBFConfig::ARRAY_ANGLE)
	{
	  //1D beamformer
	  glog.is(DEBUG1) && glog << "Computing Steering Vectors - " << std::endl;
	  glog.is(DEBUG1) && glog << "num_bins = " << num_bins_ << std::endl;
	  double constant2 = (2*M_PI/sound_speed_);
	  steering_vector_real.ReSize(curr_cfg_.number_beams()*num_bins_, num_elements_used_);
	  steering_vector_imag.ReSize(curr_cfg_.number_beams()*num_bins_, num_elements_used_);
	  
	  for (int z=1; z <= curr_cfg_.number_beams(); z++)
	    {	
	      for (int k=1; k <= num_bins_; k++)
		{
		  for (int m=1; m <= num_elements_used_; m++)
		    {
		      // When computing the steering vector, 
		      // I assume that the array elements 
		      // are aligned along the array axis with the mth
		      // element used at position 
		      // -nominal_element_locations(array_elements_used[m]).  
		      // Note the minus sign.
		      // This is used in order to have the conical 
		      // angles referenced from forward endfire. 
		      double temp=-constant2*nominal_element_locations(array_elements_used[m])*beam_cosines(z)*bin_center_frequencies(k);
		      steering_vector_real(k+(z-1)*num_bins_, m)=cos(temp)*win_spatial(m);
		      steering_vector_imag(k+(z-1)*num_bins_, m)=sin(temp)*win_spatial(m);
		    }
		}	
	    }
	}
      else
	{
	  // Azimuth/elevation beamforming
	  double constant3;
	  ColumnVector steering_vector_temp_real;
	  ColumnVector steering_vector_temp_imag;
	  ColumnVector h(4);
	  ColumnVector tau;
	  Matrix nominal_positions123;  
	  glog.is(DEBUG1) && glog << "Computing Steering Vectors - " << std::endl;
	  glog.is(DEBUG1) && glog << "num_bins = " << num_bins_ << std::endl;
	  glog.is(DEBUG1) && glog << "Elevation beams : " << 
	    curr_cfg_.number_beams_elevation() <<std::endl;
	  glog.is(DEBUG1) && glog << "Azimuth beams : " << 
	    curr_cfg_.number_beams_azimuth() <<std::endl;
	  
	  steering_vector_real.ReSize(num_elements_used_,
				      curr_cfg_.number_beams_elevation()*
				      curr_cfg_.number_beams_azimuth()*
				      num_bins_);
	  steering_vector_imag.ReSize(num_elements_used_,
				      curr_cfg_.number_beams_elevation()*
				      curr_cfg_.number_beams_azimuth()*
				      num_bins_);
	  nominal_positions123.ReSize(num_elements_used_,3);
	  steering_vector_temp_real.ReSize(num_elements_used_);
	  steering_vector_temp_imag.ReSize(num_elements_used_);
	  constant3 = -1.0/sound_speed_;
	  
	  h(1)=1.0;
	  double pitch = pitch_filter.getAvg();
	  prev_steering_pitch_ = pitch;
	  glog.is(DEBUG1) && glog << " pitch = " << doubleToString(pitch) << std::endl;
	  int q = 1;
	  
	  //      double pitch = 0;
	  if (curr_cfg_.array_configuration() == LamssCBFConfig::VOLUMETRIC)
	    {
	      for (int m=1; m <= num_elements_used_; m++)
		{
		  nominal_positions123(m,1)=array_x(array_elements_used[m]);
		  nominal_positions123(m,2)=array_y(array_elements_used[m])*cos(pitch)-
		    array_z(array_elements_used[m])*sin(pitch);
		  nominal_positions123(m,3)=array_z(array_elements_used[m])*cos(pitch)+
		    array_y(array_elements_used[m])*sin(pitch);
		}
	    }
	  else
	    {
	      for (int m=1; m <= num_elements_used_; m++)
		{
		  nominal_positions123(m,1)=0.0;
		  nominal_positions123(m,2)=
		    -nominal_element_locations(array_elements_used[m])*cos(pitch);
		  nominal_positions123(m,3)=
		    -nominal_element_locations(array_elements_used[m])*sin(pitch);
		}
	    }
	  for (int bb=1; bb <= curr_cfg_.number_beams_elevation(); bb++)
	    {
	      for (int z=1; z <= curr_cfg_.number_beams_azimuth(); z++)
		{
		  h(2)=sin(azimuthal_beams(z))*cos(elevation_beams(bb));
		  h(3)=cos(azimuthal_beams(z))*cos(elevation_beams(bb));
		  h(4)=sin(elevation_beams(bb));
		  tau=(nominal_positions123*h.Rows(2,4))*constant3;  //Note that unit vector u=h(2:4)
		  for (int k=1; k <= num_bins_; k++)
		    {
		      int manifold_index=k+(z-1)*num_bins_+
			(bb-1)*curr_cfg_.number_beams_azimuth()*num_bins_ +
			(q-1)*curr_cfg_.number_beams_elevation() *
			curr_cfg_.number_beams_azimuth()*num_bins_;
		      for (int m=1; m <= num_elements_used_; m++)
			{
			  double temp=-2*M_PI*tau(m)*bin_center_frequencies(k);
			  steering_vector_temp_real(m)=cos(temp)*win_spatial(m);  
			  steering_vector_temp_imag(m)=sin(temp)*win_spatial(m);
			}
		      steering_vector_real.Column(manifold_index)=steering_vector_temp_real;
		      steering_vector_imag.Column(manifold_index)=steering_vector_temp_imag;
		    }
		}    
	    }
	  nominal_positions123.CleanUp();
	  steering_vector_temp_real.CleanUp();
	  steering_vector_temp_imag.CleanUp();
	  tau.CleanUp();
	  h.CleanUp();
	}
      // add new steering vectors to catalogue
      SteeringVector Empty;
      Empty.config(pitch_tol, ssp_tol);
      Empty.put(pitch,sound_speed_,steering_vector_real, steering_vector_imag);
      SteeringVectors.push_back(Empty);
    }
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
	    end_data_ = leftover + curr_cfg_.number_samples(); 
	  }  
	else //Skipped one or more data files, so don't use any leftover data
	  {
	    glog.is(DEBUG2) && glog << "Non-consecutive acoustic data files (previous: "
				    << intToString(last_frame_processed_) 
				    << ", current: " 
				    << frame_number_ << std::endl;
	    leftover = 0;
	    end_data_ = curr_cfg_.number_samples();
	  }
      }
    else
      {
        leftover = 0;
        end_data_ = curr_cfg_.number_samples();
      }
  
    if ( curr_cfg_.array_type() == LamssCBFConfig::SLITA ) 
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

        for (int kk=0; kk < curr_cfg_.number_elements(); kk++) 
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
	    glog.is(DEBUG1) && glog << "Could not open sample file "
				    << filename << std::endl;
	    return false;
	  }
	
	// signals from all channels
	if ( curr_cfg_.array_type() == LamssCBFConfig::VSA || 
	     curr_cfg_.array_type() == LamssCBFConfig::DURIP || 
	     curr_cfg_.array_type() == LamssCBFConfig::HYD )
	  { 
	    temp_data = new float [curr_cfg_.number_samples()];
	    for (int kk=0; kk < curr_cfg_.number_elements(); kk++)
	      {
		fread (temp_data, sizeof (float), curr_cfg_.number_samples(), fp);
		for (int jj=leftover; jj < end_data_; jj++)
		  { 
		    Array_Data(jj+1,kk+1) = temp_data[jj-leftover];
		  }
	      }
	    delete [] temp_data;
	  }
	else if ( curr_cfg_.array_type() == LamssCBFConfig::SINGLE || 
		  curr_cfg_.array_type() == LamssCBFConfig::DUAL || 
		  curr_cfg_.array_type() == LamssCBFConfig::TEE )
	  {
	    temp_int = new short [curr_cfg_.number_samples()];
	    //read the two timetags out of the file
	    fread(&time0,sizeof(int),1,fp);
	    fread(&time1,sizeof(int),1,fp);
	    for (int kk=0; kk < curr_cfg_.number_elements(); kk++)
	      {
		fread(temp_int,sizeof(short),curr_cfg_.number_samples(),fp);
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
	if (curr_cfg_.check_consecutive_zeros())
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

//***************************************************************************
//*  LamssCBF::ProcessData()                                                    *
//*                                                                         *
//*  Called after a new data file has been read. This method calculates the *
//*    spatial power spectrum (i.e., bearing time record) and publishes it  *
//*    to the MOOSDB                                                        *
//*************************************************************************** 
bool LamssCBF::Process_Data()
{		
  int n=0;
  int n_max=0;
  bool timing = false;
  bool detect_flag;
  char bpwr_file[70];
  char buff1[50*curr_cfg_.number_beams()];
  char buff1_max[50*curr_cfg_.number_beams()]; 
  char cc[30];
  char cc_max[30];
  char * dd;
  char * dd_max;
  std::string stft_file = sonar_basename_ + ".stft";;

  //Are these initialized big enough so nothing will break? 

  Real P_real, P_imag, temp2;
  ColumnVector s_real,s_imag;
  double elapsed;
  double maxsum=0;

  int w = 0;
  int z = 0;

  double pitch = pitch_filter.getAvg();

  buff_index_ = 0;

  while ((buff_index_ + curr_cfg_.fft_size()) <= end_data_) //Changed < to <=, AJP 21 OCT 2010
    {
      for (int k=1; k <= num_elements_used_; k++)
	{
	  windowed_data=SP(Array_Data.SubMatrix(buff_index_+1,
						buff_index_+curr_cfg_.fft_size(),
						array_elements_used[k],
						array_elements_used[k]), win);
	  RealFFT (windowed_data, s_real, s_imag);
	  for (int m=1; m <= num_bins_; m++)
	    {
	      Spectrum_real(m,k) = s_real(bins[m]) * hydrophone_calibration_;
	      Spectrum_imag(m,k) = s_imag(bins[m]) * hydrophone_calibration_;
	    }
	}
      double beamsum=0;
      
      //Find Power Spectrum
      int vbeams;
      int hbeams;

      if (curr_cfg_.angular_steering_type() == LamssCBFConfig::ARRAY_ANGLE)
	{
	  vbeams = 1;
	  hbeams = curr_cfg_.number_beams();
	}
      else
	{
	  vbeams = curr_cfg_.number_beams_elevation();
	  hbeams = curr_cfg_.number_beams_azimuth();
	} 
      for (int bb=1; bb <= vbeams; bb++)
	{
	  for (int k=1; k <= hbeams; k++)
	    {
	      temp2=0;
	      for (int m=1; m <= num_bins_; m++)
		{
		  if (curr_cfg_.angular_steering_type() == LamssCBFConfig::ARRAY_ANGLE)
		    {
		      int idx1 = m+num_bins_*(k-1);
		      P_real=DotProduct(steering_vector_real.Row(idx1),Spectrum_real.Row(m)) + 
			DotProduct(steering_vector_imag.Row(idx1),Spectrum_imag.Row(m));	
		      P_imag=DotProduct(steering_vector_real.Row(idx1),Spectrum_imag.Row(m)) - 
			DotProduct(steering_vector_imag.Row(idx1),Spectrum_real.Row(m));
		    }
		  else
		    {
		      int idx1 = m+num_bins_*(k-1)+(bb-1)*hbeams*num_bins_;
		      P_real=DotProduct(steering_vector_real.Column(idx1),
					Spectrum_real.Row(m)) + 
			     DotProduct(steering_vector_imag.Column(idx1),
			  	        Spectrum_imag.Row(m));
		      P_imag=DotProduct(steering_vector_real.Column(idx1),
					Spectrum_imag.Row(m)) - 
			     DotProduct(steering_vector_imag.Column(idx1),
					Spectrum_real.Row(m));
		    }

		  if (curr_cfg_.save_beam_stft())
                    {
		      Beam_STFT_real[bb][k][num_blocks_integrated_+1][m]=
			((double)P_real)*single_bin_spectral_normalization_;
		      Beam_STFT_imag[bb][k][num_blocks_integrated_+1][m]=
			((double)P_imag)*single_bin_spectral_normalization_;
                    }
		  temp2=temp2+P_real*P_real+P_imag*P_imag;
		}
	      beamsum=beamsum+temp2;
	      P(bb,k) = P(bb,k) + temp2;
	    }
	}
      num_blocks_integrated_++;

      if (num_blocks_integrated_ == int_blocks_)
	{	
	  double max_power = 0;
	  bool nan_in_BTR = false;
	  double best_ang = 0;
	  string P_string_dB="";
	  string P_string_linear="";
	  BTRPower btr_power;
	  BTRPower::ArrayBeam* array_beam = 0;
	  lamss::protobuf::BTR btr_msg;
	  std::string serialized;
	  if (curr_cfg_.angular_steering_type() == LamssCBFConfig::ARRAY_ANGLE)
	    btr_msg.set_ambiguous(true);
	  else if ( curr_cfg_.angular_steering_type() == LamssCBFConfig::AZIMUTH_ONLY ||
		    curr_cfg_.angular_steering_type() == LamssCBFConfig::AZIMUTH_AND_ELEVATION )
	    btr_msg.set_ambiguous(!(curr_cfg_.min_azimuth() < -10 || 
				    curr_cfg_.max_azimuth() > 190));
	  else
	      btr_msg.set_ambiguous(curr_cfg_.max_elevation() == 180);
          btr_msg.set_wavenumber(curr_cfg_.sampling() == LamssCBFConfig::EQUALLY_SPACED_WAVENUMBER);
	  std::vector<double> beam_power_db;
	  beam_power_db.clear();
	  for (int elev=1; elev<=vbeams; elev++)
	    {
	      for (int azim=1; azim<=hbeams; azim++)
		{
		  double tk=P(elev,azim)*spectral_normalization_;
		  if (beam_power_db.size() < 60)
		    beam_power_db.push_back(10.0*log10(tk));
		  //		  btr_msg.add_beam_power_db(10.0*log10(tk));
		  if (tk > max_power)
		    {
		      max_power = tk;
		    }
		  if (isnan(tk))
		    nan_in_BTR = true;
		  if (tk > max_power)
		    max_power = tk;
		  P_string_linear+=doubleToString(tk);
		  if (elev < vbeams || azim < hbeams)
		    P_string_linear+=",";
		  // New BTR Power proto messaage with time stamp
		  array_beam = btr_power.add_beam();
		  if (curr_cfg_.angular_steering_type() == LamssCBFConfig::ARRAY_ANGLE)
		    
		    array_beam -> set_angle(beam_angles[azim-1]*180/M_PI);
		  else
		    {
		      array_beam -> set_azimuth(azimuthal_beams(azim)*180/M_PI);
		      array_beam -> set_elevation(elevation_beams(elev)*180/M_PI);
		    }
		  array_beam -> set_power(tk);
		}
	    }
	  for (int i=0; i<beam_power_db.size(); i++)
	    btr_msg.add_beam_power_db(beam_power_db[i]);
	  serialize_for_moos(&serialized,btr_msg);    
	  publish("BTR_DATA_DB", serialized);
	  btr_power.set_utc_time(MOOSTime() - 0.5*actual_integration_); // center of integration box
	  serialize_for_moos(&serialized,btr_power);    
	  publish("BTR_POWER", serialized);
	  
	  publish("MAX_BEAM_POWER",max_power);
	  publish("BEAM_DATA",P_string_linear); 
	  
	  if (curr_cfg_.save_beam_stft())
	      save_stft_file(stft_file);

	  P = 0.0;
	  num_blocks_integrated_ = 0;
	  w++;
	}  //end if (num_blocks_integrated_ == int_blocks)
      
      z++;
      buff_index_ = (int) (z*(curr_cfg_.fft_size() - num_overlapping_samples_));
    }		
  return true;
} // end LamssCBF::Process_Data()

void LamssCBF::save_stft_file(std::string name)
{
  FILE * stft_file;

  if ((stft_file = fopen(name.c_str(), "wb")) == NULL)  
    glog.is(DEBUG1) && glog << "Could not open file: " << name << std::endl;
  else
    {
      glog.is(DEBUG1) && glog << "Writing STFT data file: " << name << std::endl;;
      
      //Write header
      int process_id;
      if (last_processing_cmd_.processing_objective() == SonarProcessingCommand::NOISE)
	process_id = 1;
      else
	process_id = 2;
      fwrite(&process_id, sizeof(int), 1, stft_file);
      int AngularSteeringTypeINT;     
      if (curr_cfg_.angular_steering_type() == LamssCBFConfig::AZIMUTH_ONLY ||
	  curr_cfg_.angular_steering_type() == LamssCBFConfig::ARRAY_ANGLE )
	AngularSteeringTypeINT=1;
      else if (curr_cfg_.angular_steering_type() == LamssCBFConfig::ELEVATION_ONLY )
	AngularSteeringTypeINT=2;
      else 
	AngularSteeringTypeINT=3;
      fwrite(&AngularSteeringTypeINT, sizeof(int), 1 , stft_file);
      fwrite(&sound_speed_, sizeof(double), 1 , stft_file);
      int vbeams;
      int hbeams;
      double val;
      if (curr_cfg_.angular_steering_type() == LamssCBFConfig::ARRAY_ANGLE)
	{
	  vbeams = 1;
	  hbeams = curr_cfg_.number_beams();
	  fwrite(&vbeams, sizeof(int), 1 , stft_file);
	  fwrite(&hbeams, sizeof(int), 1 , stft_file);
	  val=0;
	  fwrite(&val, sizeof(double), 1 , stft_file);
	  for (int k=1; k <= hbeams; k++)
	    {
	      val=beam_angles[k];
	      fwrite(&val, sizeof(double), 1 , stft_file);
	    }
	}
      else
	{
	  vbeams = curr_cfg_.number_beams_elevation();
	  hbeams = curr_cfg_.number_beams_azimuth();
	  for (int k=1; k <= vbeams; k++)
	    {
	      val=elevation_beams(k);
	      fwrite(&val, sizeof(double), 1 , stft_file);
	    }
	  for (int k=1; k <= hbeams; k++)
	    {
	      val=azimuthal_beams(k);
	      fwrite(&val, sizeof(double), 1 , stft_file);
	    }
	} 

      fwrite(&pingtime_, sizeof(double), 1 , stft_file);
      fwrite(&recordtime_, sizeof(double), 1 , stft_file);
      fwrite(&proc_offset_, sizeof(double), 1 , stft_file);
      fwrite(&proc_duration_, sizeof(double), 1 , stft_file);
      
      int waveform_ID;
      if (waveform_ == SonarProcessingCommand::WAVEFORM_CW)
	waveform_ID=1;
      else 
	waveform_ID=2;
      fwrite(&waveform_ID, sizeof(int), 1, stft_file);
      val = curr_cfg_.sampling_frequency();
      fwrite(&val,sizeof(double),1,stft_file);
      fwrite(&int_blocks_, sizeof(int), 1, stft_file);
                    
      double time_temp;
      for (int k=1; k<=int_blocks_; k++)
	{
	  if (k==1)
	    time_temp=proc_offset_+0.5*curr_cfg_.fft_size()/
	      curr_cfg_.sampling_frequency();
	  else
	    time_temp=time_temp+
	      (curr_cfg_.fft_size()-num_overlapping_samples_)/
	      curr_cfg_.sampling_frequency();
	  
	  fwrite(&time_temp, sizeof(double), 1 , stft_file);
	}
                    
      val = curr_cfg_.center_frequency();                    
      fwrite(&val, sizeof(double), 1 , stft_file);
      val = curr_cfg_.bandwidth(); 
      fwrite(&val, sizeof(double), 1 , stft_file);
      val = curr_cfg_.fft_size(); 
      fwrite(&val, sizeof(int), 1 , stft_file);
      fwrite(&num_overlapping_samples_, sizeof(int), 1 , stft_file);
      fwrite(&num_bins_, sizeof(int), 1, stft_file);
      for (int k=1; k <= num_bins_; k++)
	{
	  int ival=bins[k];
	  fwrite(&ival, sizeof(int), 1 , stft_file);
	}

      for (int k=1; k <= num_bins_; k++)
	{
	  val=bin_center_frequencies(k);
	  fwrite(&val, sizeof(double), 1 , stft_file);
	}

      fwrite(&ping_x_, sizeof(double), 1 , stft_file);
      fwrite(&ping_y_, sizeof(double), 1 , stft_file);
      fwrite(&ping_depth_, sizeof(double), 1 , stft_file);
      fwrite(&ping_speed_, sizeof(double), 1 , stft_file);
      fwrite(&ping_heading_, sizeof(double), 1 , stft_file);
      fwrite(&ping_pitch_, sizeof(double), 1 , stft_file);
      
      double processing_array_pitch_angle=pitch_filter.getAvg();
      fwrite(&processing_array_pitch_angle, sizeof(double), 1 , stft_file);
      fwrite(&transmit_elevation_angle_, sizeof(double), 1 , stft_file);

      //save Beam_STFT_real
      for (int bb=1; bb <= vbeams; bb++)
	{
	  for (int k=1; k <= hbeams; k++)
	    {    
	      for (int t_idx=1; t_idx <= int_blocks_; t_idx++)
		{
		  for (int m=1; m <= num_bins_; m++)
		    {
		      val=Beam_STFT_real[bb][k][t_idx][m];
		      fwrite(&val, sizeof(double), 1 , stft_file);
		    }
		}
	    }
	}
      //save Beam_STFT_imag
      for (int bb=1; bb <= vbeams; bb++)
	{
	  for (int k=1; k <= hbeams; k++)
	    {    
	      for (int t_idx=1; t_idx <= int_blocks_; t_idx++)
		{
		  for (int m=1; m <= num_bins_; m++)
		    {
		      val=Beam_STFT_imag[bb][k][t_idx][m];
		      fwrite(&val, sizeof(double), 1 , stft_file);
		    }
		}
	    }
	}
      ///////////////////////////////////////////
      double max_stft;
      for (int bb=1; bb <= vbeams; bb++)
	{
	  for (int k=1; k <= curr_cfg_.number_beams_azimuth(); k++)
	    {    
	      max_stft=0;
	      for (int t_idx=1; t_idx <= int_blocks_; t_idx++)
		{
		  for (int m=1; m <= num_bins_; m++)
		    max_stft=std::max(max_stft,
				      pow(Beam_STFT_imag[bb][k][t_idx][m],2)+
				      pow(Beam_STFT_imag[bb][k][t_idx][m],2));
		}
	    }
	}
      
      //////////////////////////////////
                    
      //save P
      for (int bb=1; bb <= curr_cfg_.number_beams_elevation(); bb++)
	{
	  for (int k=1; k <= curr_cfg_.number_beams_azimuth(); k++)
	    {    
	      val=P(bb,k)*spectral_normalization_; 
	      fwrite(&val, sizeof(double), 1 , stft_file);
	    }
	}
      fclose (stft_file);
      MOOSTrace("Finished\n");
      publish("BEAM_STFT_FILE",name);
    }
}

//***************************************************************************
//*  LamssCBF::publish_active_processing_parameters()                   *
//*                                                                         *
//*  Publish Parameters needed by active sonar processing to the MOOSDB     *
//*************************************************************************** 
void LamssCBF::publish_active_processsing_parameters()
{ 
  ActiveSonarParameters active_params;
  if (last_processing_cmd_.processing_objective() == SonarProcessingCommand::NOISE)
    active_params.set_processing_objective(ActiveSonarParameters::NOISE);
  else if (last_processing_cmd_.processing_objective() == SonarProcessingCommand::TARGET)
    active_params.set_processing_objective(ActiveSonarParameters::TARGET);

  active_params.set_fft_size(curr_cfg_.fft_size());
  active_params.set_overlap(curr_cfg_.overlap());
  active_params.set_sampling_frequency(curr_cfg_.sampling_frequency());
  active_params.set_bandwidth(curr_cfg_.bandwidth());
  active_params.set_num_bins(num_bins_);
  
  std::string binstr = ",bins={";
  for (int k=1; k<=num_bins_; k++)
    {
      if(k > 1)
	binstr += ",";
      binstr += intToString(bins[k]);
    }
  binstr += "}";
  active_params.set_frequency_bins(binstr);

  binstr = "bin_center_frequencies={";
  for (int k=1; k<=num_bins_; k++)
    {
      if(k > 1)
	binstr += ",";
      binstr += doubleToString(bin_center_frequencies(k),4);
    } 
  binstr += "}";
  active_params.set_bin_center_frequencies(binstr);

  glog.is(DEBUG1) && glog << "Publishing ACTIVE_PROCESSOR_PARAMETERS to MOOSDB"
			  << std::endl;
  std::string serialized;
  serialize_for_moos(&serialized,active_params);    
  publish("ACTIVE_PROCESSING_PARAMETERS",serialized);
}


void LamssCBF::clear_buffers()
//***************************************************************************
//*  LamssCBF::clear_buffers()                                                        *
//*                                                                         *
//*  Clear beamformer arrays for re-initialization                          *
//*                                                           *
//*************************************************************************** 
{
  delete [] array_elements_used;
  delete [] bins;
}

void LamssCBF::hanning(Real *window, int M)
{
    for (int n=0; n < M; n++)
        window[n] = (double) (0.5*(1 - cos(2*M_PI*(n+1)/(M+1))));
}


//***************************************************************************
//*  LamssCBF::hamming()                                                        *
//*                                                                         *
//*  Create a Hamming window of length M.                                   *
//*************************************************************************** 
void LamssCBF::hamming(Real *window, int M)
{
    for (int n = 0; n <= (M-1); n++)
        window[n] = 0.54 - 0.46 * cos (2.0 * M_PI * (double) n / (double) (M - 1));	
}


//***************************************************************************
//*  LamssCBF::round()                                                          *
//*                                                                         *
//*  Round an input double to the nearest integer                           *
//*************************************************************************** 
int LamssCBF::round(double num) 
{
    if (num >= 0.0)
        return ((int) (num + 0.5));
    else    
        return ((int) (num - 0.5));
}

// >>>>>>>>>>>>>>> vector utility methods <<<<<<<<<<<<
  
//***************************************************************************
//*  LamssCBF::zeros()                                                          *
//*                                                                         *
//*  Initialize the input array of length length with zeros.                *
//*************************************************************************** 
void LamssCBF::zeros(double * array,int length)
{
    for (int ii = 0; ii < length; ii++ )
	array[ii] = 0.0;
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


//
// Storage of steering vectors
//

void SteeringVector:: config(double p_tol, double s_tol)
{
  pitch_tol = p_tol;
  ssp_tol = s_tol;
}

void SteeringVector:: put(double sv_pitch, double sv_ssp, Matrix sv_real, Matrix sv_imag)
{
  pitch = sv_pitch;
  ssp = sv_ssp;
  steering_vector_real =sv_real;  
  steering_vector_imag =sv_imag;
}

bool SteeringVector:: get(double sv_pitch, double sv_ssp, Matrix& sv_real, Matrix& sv_imag)
{
  if (fabs(sv_pitch-pitch) < pitch_tol && fabs(sv_ssp - ssp) < ssp_tol)
    { 
      sv_real = steering_vector_real;  
      sv_imag = steering_vector_imag;
      return true;
    }
  else
    return false;
}

