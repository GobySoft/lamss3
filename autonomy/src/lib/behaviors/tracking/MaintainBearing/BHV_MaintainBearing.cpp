/***********************************************************/
/*    NAME: Henrik Schmidt                                  */
/*    FILE: BHV_MaintainBearing.cpp                             */
/*    DATE: March 12 2009                                   */
/***********************************************************/

#include <iostream>
#include <math.h> 
#include <stdlib.h>
#include "BHV_MaintainBearing.h"
#include "MBUtils.h"
#include "ZAIC_PEAK.h"
#include "BuildUtils.h"

using namespace std;

//-----------------------------------------------------------
// Procedure: Constructor

BHV_MaintainBearing::BHV_MaintainBearing(IvPDomain gdomain) : 
  IvPBehavior(gdomain)
{
  this->setParam("descriptor", "(d)bhv_ArrayAngleNew");
  this->setParam("unifbox", "course=3");
  this->setParam("gridbox", "course=9");

  m_domain = subDomain(m_domain, "course");
 
  heading_variable = "AEL_HEADING";
  bearing_variable = "BEARING_STAT";

  bearing_type = "relative";
  bearing_mode = "dual";
  
  over_shoot = 0.0;
  desired_angle = 90.0;

  bearing_rate_threshold = 3  ; // Default 1 deg/minute
  alternate = false;
  new_run = true;
}

//-----------------------------------------------------------
// Procedure: setParam

bool BHV_MaintainBearing::setParam(string param, string val) 
{

  // Dangerous: you're not returning false on an unrecognized
  // parameter - mikerb, 02/27/08
  // Inherited template! fixed HS 03/06/08

  //  IvPBehavior::setParamCommon(param, val);

  if(IvPBehavior::setParamCommon(param, val))
    return(true);

 if(param == "width") 
   {
     width = (int) atof(val.c_str());
   }
 else if(param == "peak_width") 
   {
     pwidth = (int) atof(val.c_str());
   }
 else if(param == "over_shoot") 
   {
     over_shoot = atof(val.c_str());
   }
 else if(param == "desired_angle") 
   {
     desired_angle = atof(val.c_str());
   }
 else if (param == "heading_variable")
   {
     heading_variable = val;
   }
 else if (param == "bearing_variable")
   {
     bearing_variable = val;
   }
 else if (param == "bearing_type")
   {
     bearing_type = val;
     absolute_bearing = (bearing_type == "absolute");
   }
 else if (param == "bearing_mode")
   {
     string buf = tolower(val);
     if (buf  == "single" || buf == "quad")
       bearing_mode = buf;
     else
       bearing_mode = "dual";
   }
 else if (param == "alternate_interval")
   {
     alternate_interval = atof(val.c_str());
     if (alternate_interval > 0.0)
       alternate = true;
     else
       alternate = false;
   }
 else if (param == "bearing_rate_threshold")
   {
     bearing_rate_threshold = atof(val.c_str());
   }
 else
   return(false);

  addInfoVars(heading_variable);
  addInfoVars(bearing_variable);

  return(true);
}


//-----------------------------------------------------------
// Procedure: onRunState

IvPFunction *BHV_MaintainBearing::onRunState()  
{
  bool ok1,ok2;
  

  //get the heading of the array
  double osCourse = getBufferDoubleVal(heading_variable, ok1);
  
  //get current tracking state
  string tState = getBufferStringVal(bearing_variable, ok2);

  if(!ok1 || !ok2){
    postEMessage("error,BHV_MaintainBearing: ownship data not available");
    return (0);
  }
    
  int new_state = decode(tState);
  double current_angle;
  if (bearing_state == 2)
    {
      current_angle = (true_bearing-osCourse);
      if (current_angle <= -180.0)
	current_angle += 360.0;
      else if(current_angle > 180.0)
	current_angle -= 360.0;

      curr_time = getBufferCurrTime();


      double abs_brg = true_bearing;
      int num_brg =bearings.size();
      // unwrap absolute bearing
      if (num_brg == 0)
	bearings.push_back(abs_brg);
      else 
	{
	  while (abs_brg - bearings[num_brg-1] > 180)
	    abs_brg -= 360;
	  while (abs_brg - bearings[num_brg-1] <= - 180)
	    abs_brg += 360;
	  bearings.push_back(abs_brg); 
	}
      times.push_back(curr_time);
      num_brg =bearings.size();

      vec.prune_timeseries(times,bearings,curr_time,0.5*alternate_interval,
			   times,bearings);
	      // absolute bearing rate by linear regression
      double bearing;
      double bearing_rate;
      double rms_error;
      vec.lin_least_square(times,bearings,curr_time,bearing,bearing_rate,rms_error);
      postMessage("BEARINGS",(double) bearings.size());
      // convert to degrees/minute
      bearing_rate *= 60;
      rms_error*=60;
      postMessage("BEARING_RATE",bearing_rate);
      postMessage("BEARING_RATE_RMS",rms_error);
      postMessage("PENDING_HEADING_SWITCH", last_switch + alternate_interval - 
		  curr_time);
      
      if (new_run)
	{
	  starboard = (current_angle >= 0);
	  last_switch = curr_time;
	  new_run = false;
	} 
      
      bool rate_ok = (fabs(bearing_rate) > bearing_rate_threshold);
      bool rate_est_ok = (rms_error < fabs(bearing_rate_threshold)) ;
      bool rate_fair = (fabs(bearing_rate) > 0.5*bearing_rate_threshold);
      bool hdg_switch = ((!rate_ok && rate_est_ok &&
			  curr_time > last_switch + alternate_interval) ||
			 (!rate_fair && rate_est_ok &&
			  curr_time > last_switch + 0.5*alternate_interval)
			 );

      if (alternate && hdg_switch)
	{
	  starboard = !starboard;
	  last_switch = curr_time;
	  bearings.clear();
	  times.clear();
	}
      current_angle = fabs(current_angle);
    }
  else
    {
      return (0);
    }

  //postMessage("ARRAY_ANGLE",current_angle);

  double relevance = getRelevance();

  if(relevance <= 0)
    return(0);

  // overshoot desired angle
  double auv_desired_angle = desired_angle + over_shoot*(desired_angle-current_angle);
  //postMessage("DESIRED_AUV_ANGLE",auv_desired_angle);
  //pick starboard as positive bearing
  double leftabs = (true_bearing - auv_desired_angle);
  while (leftabs < 0.0)
    leftabs += 360.0;
  while (leftabs >= 360.0)
    leftabs -= 360.0;

  
  //pick right side desired direction
  double rightabs = (true_bearing + auv_desired_angle);
  while (rightabs >= 360.0)
    rightabs -= 360.0;
  while (rightabs < 0.0)
    rightabs += 360.0;

  double firstabs = leftabs;
  double secondabs = rightabs;
  if (alternate && !starboard)
    {
      firstabs = rightabs;
      secondabs = leftabs;
    }
  //postMessage("LEFTABS",leftabs);
  //postMessage("RIGHTABS",rightabs);

  ZAIC_PEAK crs_zaic(m_domain,"course");
  crs_zaic.setSummit(firstabs);
  crs_zaic.setValueWrap(true);
  crs_zaic.setPeakWidth(width);
  crs_zaic.setBaseWidth(180.0 - width);
  crs_zaic.setSummitDelta(50.0);
  crs_zaic.setMinMaxUtil(0,100);

  if (bearing_mode == "dual" || bearing_mode == "quad")
    {
      if (!alternate)
	{
	  int new_index = crs_zaic.addComponent();  
	  crs_zaic.setSummit(secondabs, new_index);
	  crs_zaic.setPeakWidth(width, new_index);
	  crs_zaic.setBaseWidth(180.0 - width,new_index);
	  crs_zaic.setSummitDelta(50.0, new_index);
	  crs_zaic.setMinMaxUtil(0,100,new_index);
	}

      if (bearing_mode == "quad" && desired_angle != 90.0 && desired_angle != 270.0)
	{
	  
	  double leftrear = (true_bearing - (180.0-auv_desired_angle));
	  while (leftrear < 0.0)
	    leftrear += 360.0;
	  while (leftrear >= 360.0)
	    leftrear -= 360.0;
	  if (!alternate || !starboard)
	    {
	      int new_index = crs_zaic.addComponent();  
	      crs_zaic.setSummit(leftrear, new_index);
	      crs_zaic.setPeakWidth(width, new_index);
	      crs_zaic.setBaseWidth(90.0 - width,new_index);
	      crs_zaic.setSummitDelta(50.0, new_index);
	      crs_zaic.setMinMaxUtil(0,100,new_index);
	    }
	  double rightrear = (true_bearing + (180.0-auv_desired_angle));
	  while (rightrear < 0.0)
	    rightrear += 360.0;
	  while (rightrear >= 360.0)
	    rightrear -= 360.0;
	  if (!alternate || starboard)
	    {
	      int new_index = crs_zaic.addComponent();  
	      crs_zaic.setSummit(rightrear, new_index);
	      crs_zaic.setPeakWidth(width, new_index);
	      crs_zaic.setBaseWidth(90.0 - width,new_index);
	      crs_zaic.setSummitDelta(50.0, new_index);
	      crs_zaic.setMinMaxUtil(0,100,new_index);
	    }
	}
    }

  
  IvPFunction *of = crs_zaic.extractOF();
  
  of->setPWT(m_priority_wt);
  
  return(of);
}

void BHV_MaintainBearing::onIdleState()  
{
  new_run = true;
  bearings.clear();
  times.clear();
  bearing_mode = "dual";
  bearing_type = "relative";
}

int BHV_MaintainBearing::decode(string status)
{
  vector<string> svector;
  vector<string> svector2;


  // Parse the waypoint status string for "us"  
  svector = parseString(status, ',');
  for(unsigned int i=0; i<svector.size(); i++) 
    {
      svector2 = parseString(svector[i], '=');
      if(svector2.size() != 2) 
	{
	  postEMessage("error,BHV_MaintainBearing: Invalid waypoint string");
	  return(0);
	}

    string left  = stripBlankEnds(svector2[0]);
    string right = stripBlankEnds(svector2[1]);
   
    if(left == "bearing")   true_bearing  = atof(right.c_str());
    if(left == "state")   bearing_state  = atof(right.c_str());
    
    }

  return(1);
}

double BHV_MaintainBearing::getRelevance()
{
 
  //for now, this behavior is fully relevant when it is active
  return(1.0);
}

