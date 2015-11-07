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
      last_slow_loop_t_(0)
{

    goby::glog.set_lock_action(lock);
    goby::glog.add_group("fsm", goby::common::Colors::lt_magenta);
    goby::glog.add_group("helm", goby::common::Colors::lt_green);
    goby::glog.add_group("mac", goby::common::Colors::lt_blue);
    goby::glog.add_group("track", goby::common::Colors::yellow);

    set_ignore_stale(false);

    // set up subscriptions before starting separate thread
    subscribe("SONAR_CONTROL", 
	      &LamssCBF::handle_sonar_control, this);    
    subscribe("SONAR_PROCESS", 
	      &LamssCBF::handle_sonar_process, this);    

    scheduler_.initiate_processor(fsm_);  
  
    fsm_thread_.reset(new boost::thread(boost::bind(&boost::statechart::fifo_scheduler<>::operator(), &scheduler_, 0)));
  
    glog.is(VERBOSE) && glog << cfg_.DebugString() << std::endl;

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


