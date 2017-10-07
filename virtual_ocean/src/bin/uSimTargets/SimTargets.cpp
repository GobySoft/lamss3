// Copyright 2015 
// Henrik Schmidt
// Massachusetts Institute of Technology 

#include <dlfcn.h>

#include <numeric>

#include <boost/thread.hpp>

#include "goby/common/logger.h"
#include "goby/acomms/connect.h"
#include "goby/common/time.h"

#include "SimTargets.h"

#include "goby/moos/moos_protobuf_helpers.h"

using namespace goby::common::logger;
using namespace goby::common::logger_lock;
namespace gpb = goby::moos::protobuf;
using goby::common::goby_time;
using goby::glog;

const double pi = 3.14159;

boost::shared_ptr<SimTargetsConfig> master_config;
SimTargets* SimTargets::inst_ = 0;
void* SimTargets::driver_library_handle_= 0;

int main(int argc, char * argv[])
{
    return goby::moos::run<SimTargets>(argc, argv);
}

SimTargets* SimTargets::get_instance()
{
    if(!inst_)    {
        master_config.reset(new SimTargetsConfig);
        inst_ = new SimTargets(*master_config);
    }
    return inst_;
}

SimTargets::SimTargets(SimTargetsConfig& cfg)
    : GobyMOOSApp(&cfg),
      cfg_(cfg),
      source_control(true),
      target_number(0),
      target_init(false),
      auv_x(0),
      auv_y(0),
      got_x(false),
      got_y(false),
      t_num(0),
      last_slow_loop_t_(0)
{

  if (cfg_.has_max_range())
    {
      rangeCrit = cfg_.max_range();
      noRangeCrit = false;
    }
  else
    {
      noRangeCrit = true;
      got_x = true;
      got_y = true;
    }

  Targets.clear();

  set_ignore_stale(false);
  
  // set up subscriptions before starting separate thread
  subscribe("TGT_STATE_IN", &SimTargets::handle_tgt_state, this);    
  subscribe("NAV_X", &SimTargets::handle_nav_x, this);
  subscribe("NAV_Y", &SimTargets::handle_nav_y, this);    

  //
  // Initialize MOOSGeodesy
  //
  
  m_Geodesy.Initialise(cfg_.common().lat_origin(),cfg_.common().lon_origin());

  //
  glog.is(VERBOSE) && glog << cfg_.DebugString() << std::endl;
  //
  // Load auto_target and wait for nav data before publis=hin target list
  //
  if (cfg_.has_auto_target())
    {
      update_target(cfg_.auto_target());
      target_init = true;
    }
}

void SimTargets::loop()
{

  double curr_time = MOOSTime();
  

  if (target_init && got_x && got_y)
    {
      target_utc = MOOSTime();
      t_num += 1;
      target_number = t_num;
      updateTargetList();
      postTargetList();
      
      last_time = target_utc;
      target_set = true;
      publish("TARGET_RESET","TRUE");
      target_init = false;
    }
  
  if(target_set == true)
    {
      //Calculate target position and range
      
      for (int i=0; i< Targets.size(); i++)
	{
	  time_now = MOOSTime();
	  elapsed = time_now - Targets[i].target_utc;
	  target_x = Targets[i].target_x+(sin(Targets[i].target_hdg*M_PI/180.0)*elapsed*Targets[i].target_spd);
	  target_y = Targets[i].target_y+(cos(Targets[i].target_hdg*M_PI/180.0)*elapsed*Targets[i].target_spd);
	  source_range = sqrt( (auv_x-target_x)*(auv_x-target_x) + (auv_y-target_y)*(auv_y-target_y) );
	  glog.is(DEBUG1) && glog << "Target " << Targets[i].target_id()
				  << ", Range " << source_range << endl;
	  glog.is(DEBUG1) &&  glog << "Target_x = " << target_x
				   << ", Target_y = " << target_y << endl;
	  string tstr = Targets[i].target_id();
	  // 2010-12-14 Stephanie Kemna:
	  //   adding bool noRangeCrit in case you don't want a critical
	  //   range (multiple vehicles, one pSimTargets)
	  if (source_range < rangeCrit || noRangeCrit)
	    {
	      Targets[i].target_on = true;
	      publish(tstr+"_SIM_STATE",Targets[i].target_state());
	    }
	  else
	    {
	      Targets[i].target_on = false;
	      publish(tstr+"_SIM_STATE",Targets[i].target_state());
	      cancelTarget(tstr);
	      postTargetList();
	    }

	}
      
    }
  // process some events at a slower rate
  const int slow_loop_period = 10;
  if(curr_time > last_slow_loop_t_ + slow_loop_period)
    slow_loop(curr_time);
}

void SimTargets::slow_loop(double curr_time)
{
  // Here we should add slow loop actions.

  // Publish node report
  if (target_set)
    {
      for (int i=0; i< Targets.size(); i++)
	{
	  publish ("NODE_REPORT", target_AIS(i));
	}
    }

  last_slow_loop_t_ = curr_time;
}

void SimTargets::handle_nav_x(const CMOOSMsg& msg)
{
  auv_x = msg.GetDouble();
  got_x = true;
}

void SimTargets::handle_nav_y(const CMOOSMsg& msg)
{
  auv_y = msg.GetDouble();
  got_y = true;
}

void SimTargets::handle_tgt_state(const CMOOSMsg& msg)
{
  LAMSS_SIMULATE_TARGET tgt_msg;
  parse_for_moos(msg.GetString(),&tgt_msg);
  glog.is(DEBUG1) && glog << "Got TGT_STATE message, target number " 
			  << tgt_msg.tgt_num() << endl;
  update_target(tgt_msg);
}

void SimTargets::update_target(LAMSS_SIMULATE_TARGET tgt_msg)
{
  target_number = tgt_msg.tgt_num();
  target_x_start = tgt_msg.tgt_x();
  target_y_start = tgt_msg.tgt_y();
  target_depth = tgt_msg.tgt_dep();
  target_heading = tgt_msg.tgt_hdg();
  target_speed = tgt_msg.tgt_spd();
  target_utc = tgt_msg.tgt_utc();
  target_freq = tgt_msg.tgt_freq();
  target_bw = tgt_msg.tgt_bw();
  target_spl = tgt_msg.tgt_spl();
  target_str = tgt_msg.tgt_str();
  // Tones currently limited to 4. To be defined repeated.
  tone_freq.clear();
  tone_dbl.clear();

  if (tgt_msg.tn1_freq() > 0)
    {
      tone_freq.push_back(tgt_msg.tn1_freq());
      tone_dbl.push_back(tgt_msg.tn1_dbl());
      if (tgt_msg.tn2_freq() > 0)
	{
	  tone_freq.push_back(tgt_msg.tn2_freq());
	  tone_dbl.push_back(tgt_msg.tn2_dbl());
	  if (tgt_msg.tn3_freq() > 0)
	    {
	      tone_freq.push_back(tgt_msg.tn3_freq());
	      tone_dbl.push_back(tgt_msg.tn3_dbl());
	      if (tgt_msg.tn4_freq() > 0)
		{
		  tone_freq.push_back(tgt_msg.tn4_freq());
		  tone_dbl.push_back(tgt_msg.tn4_dbl());
		}
	    }
	}
    }

  if (target_number > 0)
    {
      updateTargetList();
      postTargetList();
    }
  else if (target_number < 0)
    {
      CTarget SimTgt;
      SimTgt.target_number = -target_number;
      string tstr = SimTgt.target_id();
      cancelTarget(tstr);
      postTargetList();
      
    }
  else
    glog.is(DEBUG1) && glog << "Target ID 0 ignored" << endl;
  
  last_time = target_utc;
  target_set = true;
  publish("TARGET_RESET","TRUE");
  
}

//-----------------------------------------------------------------
// Procedure: updateTargetList
//   Purpose: Maintain a list of known unique target names, and the
//            times they were last written to.
//   HS 032108 Modified to full target nav info and UTC of report to allow inheritance
//             and extrapolation of unreported varibles in NaFCon messages 

void SimTargets::updateTargetList()
{

    double moos_time = MOOSTime();

    unsigned int i;

    vector<double> values;
    values.clear();

    CTarget Empty;
  
    for(i=0; i<Targets.size(); i++) 
      {
        if(Targets[i].target_number == target_number) 
	  {

	    values.push_back(target_utc);
	    values.push_back(target_x_start);
	    values.push_back(target_y_start);
	    values.push_back(target_speed);
	    values.push_back(target_heading);
	    values.push_back(target_depth);
	    values.push_back(target_spl);
	    values.push_back(target_freq);
	    values.push_back(target_bw);
	    values.push_back(target_str);
	    for (int j=0; j<tone_freq.size();j++)
	      {
		values.push_back(tone_freq[j]);
		values.push_back(tone_dbl[j]);
	      }
	    Targets[i].setTargetValues(target_number,false,values);
	    glog.is(VERBOSE) && glog << "Modified Target " << i+1
				    << " Time = " << target_utc
				    << " Speed = " << target_speed
				    << " Heading = " << target_heading << endl;
	    glog.is(DEBUG1) && glog << Targets[i].printTargetValues() << endl;
	    return;
	  }
      }
    Targets.push_back(Empty);    
    values.push_back(target_utc);
    values.push_back(target_x_start);
    values.push_back(target_y_start);
    values.push_back(target_speed);
    values.push_back(target_heading);
    values.push_back(target_depth);
    values.push_back(target_spl);
    values.push_back(target_freq);
    values.push_back(target_bw);
    values.push_back(target_str);
    for (int j=0; j<tone_freq.size();j++)
      {
	values.push_back(tone_freq[j]);
	values.push_back(tone_dbl[j]);
      }
    Targets[Targets.size()-1].setTargetValues(target_number,false,values);

    glog.is(VERBOSE) && glog << "New Target " << Targets.size()
			    << " Time= " << target_utc
			    << " Speed= " << target_speed
			    << " Heading= " << target_heading << endl;
    glog.is(DEBUG1) && glog << Targets[Targets.size()-1].printTargetValues() << endl;
}

void SimTargets::cancelTarget(string target_name)
{
    target_name = toupper(stripBlankEnds(target_name));

    double moos_time = MOOSTime();
    glog.is(DEBUG1) && glog << "Entering cancelTarget, target " << target_name << endl;
    
    for(int j=0; j<Targets.size(); j++) 
      {
        if(Targets[j].target_id() == target_name)
	  { 
	    glog.is(DEBUG1) && glog << "Removing target " << target_name << endl;
	    for (int i=j; i<Targets.size()-1; i++)
	      {
		glog.is(DEBUG1) && glog << "Shifting target " 
					<< Targets[i+1].target_id() << endl;
		Targets[i] = Targets[i+1];
	      }	      
	    glog.is(DEBUG1) && glog << "Popping target " 
				    << Targets[Targets.size()-1].target_id() << endl;
	    Targets.pop_back();

	    return;
	  }
      }
}



//-----------------------------------------------------------------
// Procedure: postTargetList
//   Purpose: 

void SimTargets::postTargetList()
{
    double moos_time = MOOSTime();

    int vsize = Targets.size();

    string target_list_string;
  
    if (vsize > 0)
      {
	vector<int> targets;
	targets.clear();
	for (int i = 0; i< vsize; i++)
	  targets.push_back(Targets[i].target_number);
	int_vec2str(targets,target_list_string);
      }
    else
      {
	target_list_string = "";
	target_set = false;
      }

    publish("TARGET_SIM_LIST", target_list_string);


}

string SimTargets::target_AIS(int index)
{

  vector<double> values;
  int tgt_num;
  bool tgt_on;

  Targets[index].getTargetValues(tgt_num,tgt_on,values);

  double target_utc = values[0];  
  double target_x = values[1];  
  double target_y = values[2];  
  double target_spd = values[3];
  double target_hdg = values[4];  
  double target_dep = values[5];  
  double target_lat;
  double target_lon;

  double time_now = MOOSTime();
  double elapsed = time_now - target_utc;
  glog.is(DEBUG1) && glog << "elapsed = " << elapsed
			  << ", target_utc = " << target_utc << endl;

  double tgt_x_now = target_x+(sin(target_hdg*M_PI/180.0)*elapsed*target_spd);
  double tgt_y_now = target_y+(cos(target_hdg*M_PI/180.0)*elapsed*target_spd);

  // For use by IvPContactBehavior  
  std::string prefix="TGT_" + intToString(tgt_num) + "_SIM";
  publish (prefix+"_NAV_X",tgt_x_now);
  publish (prefix+"_NAV_Y",tgt_y_now);
  publish (prefix+"_NAV_DEPTH",target_depth);
  publish (prefix+"_NAV_SPEED",target_speed);
  publish (prefix+"_NAV_HEADING",target_hdg);
  publish (prefix+"_NAV_UTC",time_now);

  // Create NODE_REPORT
  if (!m_Geodesy.UTM2LatLong(tgt_x_now,tgt_y_now,target_lat,target_lon))
    {
      glog << "Error in UTM2LatLong conversion" << endl;
    }

  string output="NAME=tgt_" + intToString(tgt_num) +"_sim,TYPE=track,";

  output += "UTC_TIME=" + doubleToString(time_now,1);
  output += ",";

  output += "X=" + doubleToString(tgt_x_now,2);
  output += ",";

  output += "Y=" + doubleToString(tgt_y_now,2);
  output += ",";

  output += "LAT=" + doubleToString(target_lat,6);
  output += ",";

  output += "LON=" + doubleToString(target_lon,6);
  output += ",";

  output += "SPD=" + doubleToString(target_spd,2);
  output += ",";

  output += "HDG=" + doubleToString(target_hdg,2);
  output += ",";

  output += "DEPTH=" + doubleToString(target_dep,2);

  return(output);
}
