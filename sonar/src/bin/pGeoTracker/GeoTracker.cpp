#include "GeoTracker.h"

using namespace std;


CGeoTracker::CGeoTracker()
{
  tstate = 0;
  Mnum = 0;
  total_time = 0;
  solution = false;
  last_btime = 0.0;
  meas_count = 0;
  track_count = 0;
  close_range = false;

  prosecute_target_range = 800;

  vehicleID = -1;   
  prosecute = false;
  tracking_mode = SINGLE_VEHICLE_TRACKING;
  target_id = "";
}

CGeoTracker::~CGeoTracker()
{
   
}

void CGeoTracker::reset()
{
  CList.reset();
  tstate = 0;
  Mnum = 0;
  total_time = 0;
  solution = false;
  last_btime = 0.0;
  trange = 0.0;  
  nmeas = 0;
  meas_count = 0;
  track_count = 0; 
  close_range = false;
  times.clear();
  bearings.clear();
  tracking_mode = SINGLE_VEHICLE_TRACKING;

}

void CGeoTracker::single_reset()
{
  // CList.reset_solution();
  CList.reset();
  track_count = 0;
  tracking_mode = SINGLE_VEHICLE_TRACKING;
}

bool CGeoTracker::Iterate()
{  
  char buff[500];
  CINFO  Temp_Contact;

  //  MOOSTrace("Entering Iterate\n");
 
  // Set current target info for tracker
  CList.setTargetInfo(tgt_x, tgt_y, tgt_spd, tgt_hdg, tgt_utc);
  //if we're tracking at least one target
  //hard-wired for one target
  if ( CList.get_num_contacts() > 0 && CList.tgt_msg_ready == 1 && (tracking_mode == SINGLE_VEHICLE_TRACKING))
    {
      //GET THE CONTACT INFO
      CList.get_Cinfo(0,&Temp_Contact);

      double cx    = Temp_Contact.x_pos;
      double cy    = Temp_Contact.y_pos;
      double ch    = Temp_Contact.heading;
      double cs    = Temp_Contact.speed; 
      double range = Temp_Contact.range;
      double stime = Temp_Contact.Last_Meas_Time;
      nmeas = Temp_Contact.num_meas;   

      
      //if we have a full solution
      if (Temp_Contact.Ctype == RANGE_BEARING_HEADING_SPEED )
	{ 
	  //m_Comms.Notify("TRACKING","TRACKING");
	  m_Comms.Notify("TARGET_X",cx);
	  m_Comms.Notify("TARGET_Y",cy);
	  m_Comms.Notify("TARGET_HEADING",ch);
	  m_Comms.Notify("TARGET_SPEED",cs);	   
	  
	  //  CList.setTargetInfo(cx,cy,cs,ch,stime); //Set in Iterate
			  
	  //send the TRACK_STAT message to the MOOS DB
	  sprintf(buff,"node=%d,state=%d,x=%.3f,y=%.3f,heading=%.3f,speed=%.3f,time=%.2f",vehicleID,2,cx,cy,ch,cs,stime); 	   
	  m_Comms.Notify("TRACK_STAT",buff);
	  CList.tgt_msg_ready = 0;
	  //
	  // Henrik 040209. Post Target info for track and trail behaviors	
	  postTargetNav(cx, cy, ch ,cs,stime);
	}
      else if (Temp_Contact.Ctype == AMBIGUOUS_BEARING)
	{ 
	  //send the TRACK_STAT message to the MOOS DB
	  sprintf(buff,"node=%d,state=%d,x=%.3f,y=%.3f,mnum=%d",vehicleID,1,0.0,0.0,Temp_Contact.num_meas); 
	  m_Comms.Notify("TRACK_STAT",buff);  
	  CList.tgt_msg_ready = 0;
	}      
    }  
  else if ( CList.tgt_est && !CList.tgt_track )
    {
      //      MOOSTrace("Posting Target Nav estimate\n");
      postTargetNav(CList.tgt_x_est, CList.tgt_y_est, CList.tgt_hdg_est ,CList.tgt_spd_est , MOOSTime());
      //      MOOSTrace("Posted Target Nav estimate\n");
      CList.tgt_est = false;
    }

  // MOOSTrace("Exiting Iterate\n");


  return true;
}

bool CGeoTracker::OnStartUp()
{

  //CMOOSApp::OnStartUp();
        
    double th;
    
    double dfmeas;
    if(m_MissionReader.GetConfigurationParam("init_meas",dfmeas))
    {
      init_meas = (int) dfmeas;
    }
    else
    {
        MOOSTrace("GeoTracker:: inital measurements not set - FAIL\n");
        
        return false;
    }

    double rangeg;
    if(m_MissionReader.GetConfigurationParam("range_guess",rangeg))
    {
      CList.range_guess = rangeg;
      MOOSTrace("GeoTracker:: range_guess = %f \n",rangeg);
    }
    else
    {
        MOOSTrace("GeoTracker:: range_guess not set - FAIL\n");
        return false;
    }

    double rmax;
    if(m_MissionReader.GetConfigurationParam("range_max",rmax))
    {
      CList.range_max = rmax;
      MOOSTrace("GeoTracker:: range_max = %f \n",rmax);
    }
    else
    {
      CList.range_max = 5000.0;
      MOOSTrace("GeoTracker:: range_max not set - using default %f \n",CList.range_max);
      //        return false;
    }

    double vmax;
    if(m_MissionReader.GetConfigurationParam("speed_max",vmax))
    {
      CList.speed_max = vmax;
      MOOSTrace("GeoTracker:: speed_max = %f \n",vmax);
    }
    else
    {
      CList.speed_max = 20.0;
      MOOSTrace("GeoTracker:: speed_max not set - using default %f \n",CList.speed_max);
      //        return false;
    }

    double meas;
    if(m_MissionReader.GetConfigurationParam("min_measurements",meas))
    {
      CList.min_meas = (int) meas;
    }
    else
    {
        MOOSTrace("GeoTracker:: min_measurements not set - FAIL\n");
        return false;
    }

    double len;
    if(m_MissionReader.GetConfigurationParam("Z_length",len))
    {
      
      CList.Z_length = (int) len;
    }
    else
    {
        MOOSTrace("GeoTracker:: Z_length not set - FAIL\n");
        
        return false;
    }

    double theta;
    if(m_MissionReader.GetConfigurationParam("sampling_theta",theta))
    {
      
      dtheta = (double) theta;
    }
    else
    {
        MOOSTrace("GeoTracker:: sampling_theta not set - FAIL\n");
        
        return false;
    }

    if(m_MissionReader.GetConfigurationParam("bearing_rate_interval",
					     bearing_rate_interval))
      {
	
    }
    else
    {
      bearing_rate_interval = 300;
        MOOSTrace("GeoTracker:: Using default bearing_rate_interval \n");
    }

    if(m_MissionReader.GetConfigurationParam("bearing_rate_threshold",
					     bearing_rate_threshold))
      {
	
    }
    else
    {
      bearing_rate_threshold = 3;
      MOOSTrace("GeoTracker:: Using default bearing_rate_threshold %f \n",
		bearing_rate_threshold);
    }

 
    return true;
}



bool CGeoTracker::OnNewMail(MOOSMSG_LIST &NewMail)
{
  CMOOSMsg Msg;
  Measurement *New_Measurement;
  int cidx;
  double sint;
  bool decode_stat;

  //NewMail.sort();
  //  MOOSMSG_LIST::reverse_iterator p;
  MOOSMSG_LIST::iterator p;
  for(p = NewMail.begin(); p != NewMail.end(); p++) 
    {

    CMOOSMsg &Msg = *p;
    
    if(Msg.m_sKey == "TARGET_CUE")
      {	
	string val;
	val = Msg.m_sVal;
        // decode_tgt_state(val);
	    parse_tgt_stat(val, 
		   cue_number,
		   cue_x,
		   cue_y,
		   cue_z,
		   cue_hdg,
		   cue_spd,
		   cue_utc,
		   cue_freq,
		   cue_bw,
		   cue_spl,
		   cue_str,
		   cue_tone_freq,
		   cue_tone_dbl);

	// Set cued target info for tracker
	CList.setTargetCue(cue_x, cue_y, cue_spd, cue_hdg, cue_utc);
      }
    else if(Msg.m_sKey == "BEARING_STAT")
      {

       New_Measurement = new(Measurement);
       string bstat = Msg.m_sVal;

       //returns false if bearing is from another node
       decode_stat = parse_measurement(bstat,New_Measurement);
       	
       int bearing_state = New_Measurement->state;


       //     if (bearing_state != 0 )
       if ((bearing_state == 2) && prosecute &&(tracking == true) && (decode_stat == true))
	 {
	   if(meas_count > 0)
	     {
	       sint =((tan(dtheta*M_PI/180.0)*prosecute_target_range)/auv_speed);
	       // MOOSTrace("Interval = %f\n",sint);  
	     }
	   else
	     sint = 0;
	       
	   if(close_range = (meas_count >= init_meas))
	     m_Comms.Notify("CLOSE_RANGE","TRUE");
	   else
	     m_Comms.Notify("CLOSE_RANGE","FALSE");

	   double curr_time = MOOSTime();
	   bool rate_ok = checkBearingRate(New_Measurement->Meas_Time, 
					   New_Measurement->bearing);

	   //we don't use every bearing measurement because they are redundant
	   if(((curr_time - last_btime) > sint) && rate_ok)
	     {
	       if (close_range == true)
		 {
		   MOOSTrace("\n >>>> New measurement: <<<<\n");
		   cidx = CList.associate_bearing(New_Measurement);
		   cout << "cidx = " << cidx << endl;
		   CList.update_contact(cidx,New_Measurement);
		   track_count++;
		 }
	       else
		 track_count = 0;

	       meas_count++;
	       MOOSTrace("meas_count = %d , track_count = %d \n",meas_count,track_count);
	       if(bearing_state == 2)
		 last_btime = curr_time;
	       else
		 last_btime = 0;
	     }
	   else if (!rate_ok)
	     cout << "Measurement ignored due to low bearing rate " << endl;

	 }
       else
	 {
	   int num_contacts = CList.get_num_contacts();
	   //MOOSTrace("num_Contacts = %d\n",num_contacts);
	   //if we have lost contact, reset
	   if (CList.get_num_contacts() > 0)
	     {
	       CList.reset();
	       meas_count = 0;
               track_count = 0;
	       MOOSTrace("resetting in newmail\n");
	       m_Comms.Notify("CLOSE_RANGE","FALSE");
	     }
	 }
       
       delete New_Measurement;	  	      
       //  MOOSTrace("Deleted Measurement\n");
      }
    else if(Msg.m_sKey == "CLOSE_RANGE")
      {
        string close_state = Msg.m_sVal;
        if (MOOSStrCmp(close_state,"TRUE"))
	  {
	    if( close_range == false)
	      {
		close_range = true;
		MOOSTrace("In tracking range \n");
	      }
	  }
	else
	  close_range = false;
      }
    else if (Msg.m_sKey == "NAV_SPEED")
      {
	auv_speed = Msg.m_dfVal;
      }
	    // the following is the new generic state variable
    else if(Msg.m_sKey == "MISSION_STATE")
      {	
	string d_state = Msg.m_sVal;
	MOOSTrace("mission_state = %s\n",d_state.c_str());
	
	prosecute = MOOSStrCmp(d_state,"PROSECUTE");
	//	reset();
      }
    else if(Msg.m_sKey == "TRACKER_COMMAND")
      {     
	string t_state = Msg.m_sVal;
        
	if(MOOSStrCmp(t_state,"RESET"))
	  reset();
        else if (MOOSStrCmp(t_state, "ON"))
	  {
	    if (!tracking)
	      {
		tracking = true;
		MOOSTrace("Tracking ON\n");
	      }	  
	  }
	else
	  {
	    if (tracking)
	      {
		tracking = false;
		MOOSTrace("Tracking OFF \n");
	      }
	  }
      }
    else if(Msg.m_sKey == "TRACKING_MODE")
      {
        string deploy_mode = Msg.m_sVal;
        if ( MOOSStrCmp(deploy_mode,"MULTIPLE") )
	  {
	    if (tracking_mode != MULTI_VEHICLE_TRACKING)
	      {
		tracking_mode = MULTI_VEHICLE_TRACKING;
		MOOSTrace("MULTI VEHICLE TRACKING\n");
		// reset();
	      }
	  }
	else if ( MOOSStrCmp(deploy_mode,"SINGLE") )
	  {
	    if (tracking_mode != SINGLE_VEHICLE_TRACKING)
	      {
		tracking_mode = SINGLE_VEHICLE_TRACKING;
		MOOSTrace("SINGLE VEHICLE TRACKING\n");
		// single_reset();
	      }
	  }
      }

    else if (Msg.m_sKey == "PROSECUTE_RANGE")
      {
	prosecute_target_range=Msg.m_dfVal;
	//MOOSTrace("Target Range = %f\n",prosecute_target_range);
      }
    /*
    else if (Msg.m_sKey == "TRACKING")
      {
        string tracking_state = Msg.m_sVal;
        if (MOOSStrCmp(tracking_state, "TRACKING"))
	  {
	    if (!tracking)
	      {
		tracking = true;
		MOOSTrace("Prosecuting - Tracking mode \n");
	      }	  
	  }
	else
	  {
	    if (tracking)
	      {
		tracking = false;
		MOOSTrace("Prosecuting - Tracking off \n");
	      }
	  }
      }
    */
    else if(Msg.m_sKey == "VEHICLE_ID")
     {
	vehicleID = (int)Msg.m_dfVal;
     }
    else if(Msg.m_sKey == "TARGET_ID")
      {
	target_id = Msg.m_sVal;
	string cnid = toupper(target_id);
	tgt_navx_var  = cnid+"_NAV_X";
        m_Comms.Register(tgt_navx_var,0);
	tgt_navy_var  = cnid+"_NAV_Y";
        m_Comms.Register(tgt_navy_var,0);
	tgt_navutc_var  = cnid+"_NAV_UTC";
        m_Comms.Register(tgt_navutc_var,0);
	tgt_navhdg_var  = cnid+"_NAV_HEADING";
        m_Comms.Register(tgt_navhdg_var,0);
	tgt_navspd_var  = cnid+"_NAV_SPEED";
        m_Comms.Register(tgt_navspd_var,0);
      }
    /*
    else if(MOOSStrCmp(Msg.m_sKey,tgt_navx_var))
      {
	tgt_x = Msg.m_dfVal;
      }     
    else if(MOOSStrCmp(Msg.m_sKey,tgt_navy_var))
      {
	tgt_y = Msg.m_dfVal;
      }     
    else if(MOOSStrCmp(Msg.m_sKey,tgt_navutc_var))
      {
	tgt_utc = Msg.m_dfVal;
      }     
    else if(MOOSStrCmp(Msg.m_sKey,tgt_navhdg_var))
      {
	tgt_hdg = Msg.m_dfVal;
      }     
    else if(MOOSStrCmp(Msg.m_sKey,tgt_navspd_var))
      {
	tgt_spd = Msg.m_dfVal;
      }     
    else
      MOOSTrace("Unrecognized command: [%s]\n",Msg.m_sKey.c_str());
    */
    }
  

    return UpdateMOOSVariables(NewMail);
}


bool CGeoTracker::OnConnectToServer()
{
    
  m_Comms.Register("MISSION_STATE",0);
  
  m_Comms.Register("BEARING_STAT",0);
  m_Comms.Register("CLOSE_RANGE",0);
  m_Comms.Register("NAV_SPEED",0);
  m_Comms.Register("PROSECUTE_RANGE",0);
  // m_Comms.Register("PROSECUTE_STATE",0);
  // m_Comms.Register("DEPLOY_STATE",0);
  //  m_Comms.Register("TRACKING",0);
  m_Comms.Register("TRACKER_COMMAND",0);
  m_Comms.Register("VEHICLE_ID",0);
  m_Comms.Register("TARGET_ID",0);
  m_Comms.Register("TRACKING_MODE",0);
  m_Comms.Register("TARGET_CUE",0);
  
  m_Comms.Notify("CLOSE_RANGE","FALSE");
 
    return true;
}

bool CGeoTracker::postTargetNav(double cnx, double cny, double cnh, double cnv, double cnt)
{
  if (target_id == "")
    {
      return(false);
    }
  else
    {
      string moosPar;
      string cnid = toupper(target_id);
      moosPar  = cnid+"_NAV_X";
      m_Comms.Notify(moosPar,cnx);
      moosPar  = cnid+"_NAV_Y";
      m_Comms.Notify(moosPar,cny);
      moosPar  = cnid+"_NAV_HEADING";
      m_Comms.Notify(moosPar,cnh);
      moosPar  = cnid+"_NAV_SPEED";
      m_Comms.Notify(moosPar,cnv);
      moosPar  = cnid+"_NAV_UTC";
      m_Comms.Notify(moosPar,cnt);
      moosPar = "contact="+cnid;
      m_Comms.Notify("MY_TARGET",moosPar);
      return(true);
    }
}

bool CGeoTracker::checkBearingRate(double curr_time,double true_bearing)
{
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

      // absolute bearing rate by linear regression
      double bearing;
      double bearing_rate;
      double rms_error;
      vec.lin_least_square(times,bearings,curr_time,bearing,bearing_rate,rms_error);
      // convert to degrees/minute
      bearing_rate *= 60;
      rms_error*=60;
      cout << "Absolute Bearing Rate = " << bearing_rate <<
	", Bearing Rate RMS = " << rms_error << endl;
      bool rate_ok = ((fabs(bearing_rate) > bearing_rate_threshold &&
		       rms_error < fabs(bearing_rate)) || 
		      (curr_time <=  times[0] + 0.5*bearing_rate_interval));

      vec.prune_timeseries(times,bearings,curr_time,bearing_rate_interval,
		     times,bearings);
      return(rate_ok);
}

Contact_List::Contact_List()
{
  num_contacts = 0;
  tgt_msg_ready = 0;
  skipped = 0; 
 }

Contact_List::~Contact_List()
{
}

void Contact_List::set_num_contacts(int num)
{
  num_contacts = num;
}


Contact::Contact()
{
  num_meas = 0;
  id = 1;
  total_time = 0.0;
}

Contact::Contact(int idx)
{
  id = idx;
  num_meas = 0;
  total_time = 0.0;
}

Contact::~Contact()
{
}

void Contact_List::reset()
{
  MOOSTrace("in CList reset()\n");
  int num_c = get_num_contacts();

  for (int jj = 0; jj < num_c;jj++)
      Contacts[jj].Topt.reset(); 

  if (num_contacts > 0)
    Contacts.pop_back();
 
  skipped = 0;

  num_contacts = 0;
  tgt_msg_ready = 0;
  tgt_est = false;
  tgt_track = false;
}

void Contact_List::reset_solution()
{
  MOOSTrace("in CList reset_solution()\n");
  int num_c = get_num_contacts();

  for (int jj = 0; jj < num_c;jj++)
      Contacts[jj].Topt.reset_solution(); 

}

int Contact_List::associate_bearing(Measurement* Meas)
{

  Contact* New_Contact;
  double new_bearing;
  int beamno;
  int cid;

  new_bearing = Meas->true_bearing;
  beamno = Meas->beam;
 

  //right now hard-wired for a single contact
  //in the future add an algorithm to track multiple
  //contacts and associate a contact with each bearing
  

  //add a contact
  if (num_contacts == 0)
    {
      cid = 0;
       New_Contact = new Contact(0);
      //New_Contact = new(Contact);
      //add a contact to our list
      Contacts.push_back(*New_Contact);
      num_contacts++;
      //delete(New_Contact);
      //Contacts[cid].Topt.set_range(range_guess,0.0);  
      Contacts[cid].Topt.set_Z_length(Z_length);
      Contacts[cid].Ctype = AMBIGUOUS_BEARING;
      MOOSTrace("Added contact!!!!!!!!!!!!!!!!!!!!!!!!!!!!! %d\n",cid);

    }
  
  cid = 0;
 
  return (0);
}

int Contact_List::get_num_contacts()
{
  return num_contacts;
}

void Contact_List::update_contact(int cid, Measurement* Meas)
{
  int id_num;

  Measurement* New_measurement; 

  New_measurement = Meas;

  id_num = cid;
  MOOSTrace("updating contact, state = %d, skipped = %d\n",Meas->state,skipped);
  //tracking bearing
  if((Meas->state == 2))
    {
      if (skipped >=1)
	{
	  Add_Measurement(id_num,New_measurement);
	  SolveTrack(cid,New_measurement->auv_x,New_measurement->auv_y);
	}
      else
	skipped++;
    }
  else
    {  
       
      Contacts[cid].Last_Meas_Time = New_measurement->Meas_Time;
    }
}

bool Contact_List::Add_Measurement(int cid,Measurement* Meas)
{
  double dt;
  bool silent = false;
  
  if (Contacts[cid].num_meas == 0)
    {
      Contacts[cid].start_time = Meas->Meas_Time;
      dt = 0.0;
      Contacts[cid].num_track =0;
      Contacts[cid].track_utc.resize(50);
      Contacts[cid].track_x.resize(50);
      Contacts[cid].track_y.resize(50);
    }
  else if (Contacts[cid].num_meas > 0)  
    dt = Meas->Meas_Time - Contacts[cid].start_time;
 
  Contacts[cid].total_time = dt;

  cout << "total time = " <<  Contacts[cid].total_time << endl;
  auv_x = Meas->auv_x;
  auv_y = Meas->auv_y;

  //  if(!silent)
    MOOSTrace("\n Setting Measurement #:%d: auv_x = %f, auv_y = %f, bear = %f, dt = %f\n",Contacts[cid].num_meas+1,auv_x,auv_y,Meas->true_bearing,dt);

  
  Contacts[cid].Topt.set_measurement(auv_x,auv_y, Meas->true_bearing,dt);

  Contacts[cid].Last_Meas_Time = Meas->Meas_Time;
  Contacts[cid].num_meas += 1;

  // Update target location for behaviors
  bearing_time = Meas->Meas_Time;

  double cue_ang = (90.0 - cue_hdg)*M_PI/180.0;

  double brg_ang = Meas->true_bearing;
  
  // Set track guess as intersection between cued track and current bearing
  
  double D = - tan(brg_ang) + tan(cue_ang);
  double Dy = - tan(brg_ang) * (cue_y - cue_x * tan(cue_ang)) + tan(cue_ang) * (auv_y - auv_x * tan(brg_ang));
  double Dx = - (cue_y - cue_x * tan(cue_ang)) + (auv_y - auv_x * tan(brg_ang));

  tgt_y_est = Dy/D;
  tgt_x_est = Dx/D;
  tgt_hdg_est = 90. - atan2(tgt_y_est - cue_y,tgt_x_est - cue_x)*180.0/M_PI;
  if (cos((tgt_hdg_est-cue_hdg)*M_PI/180) < 0)
    tgt_hdg_est += 180;
  if (tgt_hdg_est >= 360)
    tgt_hdg_est -= 360;

  tgt_spd_est = cue_spd;
  MOOSTrace("tgt_x_est = %f, tgt_y_est = %f \n",tgt_x_est,tgt_y_est);
  MOOSTrace("tgt_hdg_est = %f, tgt_spd_est = %f  \n",tgt_hdg_est,tgt_spd_est);
  if (  Contacts[cid].num_meas < min_meas)
    tgt_est = true;

  // Use scalar product to determine whether cue and measurements are consistent
  if ( cos( atan2(tgt_y_est-auv_y,tgt_x_est-auv_x) - brg_ang ) < 0.0)
    {
      MOOSTrace("Cue and bearing inconsistent, using default range\n");
      
      MOOSTrace("auv_x = %f, auv_y = %f,  \n",auv_x,auv_y);
      MOOSTrace("brg_ang = %f, tgt_ang = %f  \n",brg_ang,atan2(tgt_y_est-auv_y,tgt_x_est-auv_x));
      tgt_x_est = auv_x + range_guess * cos(brg_ang);
      tgt_y_est = auv_y + range_guess * sin(brg_ang);
      tgt_hdg_est = 90. - atan2(tgt_y_est - cue_y,tgt_x_est - cue_x)*180.0/M_PI;
      tgt_spd_est = cue_spd;
      // modify cue heading
      cue_hdg = tgt_hdg_est;
      MOOSTrace("Range Guess - %f \n",range_guess);
      MOOSTrace("Modified tgt_x_est = %f, tgt_y_est = %f  \n",tgt_x_est,tgt_y_est);
      MOOSTrace("Modified tgt_hdg_est = %f, tgt_spd_est = %f  \n",tgt_hdg_est,tgt_spd_est);
  tgt_est = true;

   }
  
  return true;
}

void Contact_List::setTargetInfo(double x, double y, double spd, double hdg, double utc)
{
  tgt_x = x;
  tgt_y = y;
  tgt_spd = spd;
  tgt_hdg = hdg;
  tgt_utc = utc;
  return;
}

void Contact_List::setTargetCue(double x, double y, double spd, double hdg, double utc)
{
  cue_x = x;
  cue_y = y;
  cue_spd = spd;
  cue_hdg = hdg;
  cue_utc = utc;
  return;
}

bool Contact_List::SolveTrack(int track_id, double xnow, double ynow)
{
  status stat;
  bool silent = false;
  double xzero,yzero,xdot,ydot,x_pos_check,y_pos_check,solver_range;

  //if we've taken enough measurements
  //make this a parameter
  if(Contacts[track_id].num_meas >= min_meas)
    {
      double curr_dt=Contacts[track_id].total_time;
      //initialize tracker algorithm. initial guess based on prosecute information or previous target info on same target 
      if (tgt_spd > speed_max)
	{
	  tgt_spd = speed_max;
	}
      double curr_time = MOOSTime(); 
      double tgt_ang = (90.0 - tgt_hdg)*M_PI/180.0;
      double tgt_xdot = tgt_spd*cos(tgt_ang);
      double tgt_ydot = tgt_spd*sin(tgt_ang);

      double tgt_xnow = tgt_x + tgt_xdot * (curr_time-tgt_utc);
      double tgt_ynow = tgt_y + tgt_ydot * (curr_time-tgt_utc);

      MOOSTrace("xnow = %f, ynow = %f \n", xnow, ynow );
      MOOSTrace("tgt_x = %f, tgt_y = %f \n", tgt_x, tgt_y );
      MOOSTrace("tgt_spd = %f, tgt_hdg = %f, tgt_utc = %lf \n", tgt_spd, tgt_hdg, tgt_utc );
      MOOSTrace("tgt_xnow = %f, tgt_ynow = %f \n", tgt_xnow, tgt_ynow );
      MOOSTrace("tgt_xdot = %f, tgt_ydot = %f \n", tgt_xdot, tgt_ydot );

      Contacts[track_id].Topt.set_guess(tgt_xdot,tgt_ydot,tgt_xnow,tgt_ynow,curr_dt);
      double tgt_range_guess = sqrt(pow(tgt_xnow-xnow,2)+pow(tgt_ynow-ynow,2));

      //      if(tgt_range_guess > range_max && tgt_spd > speed_max )
      if(tgt_range_guess > range_max)
	tgt_range_guess = range_guess;
      MOOSTrace("Range_guess = %f \n", tgt_range_guess);

      Contacts[track_id].Topt.set_range(tgt_range_guess,curr_dt);  
      // set max range and speed
      Contacts[track_id].Topt.set_range_max(range_max);  
      Contacts[track_id].Topt.set_speed_max(speed_max);  
      //try to get a solution
      stat = Contacts[track_id].Topt.get_track();

      //if the tracker algorithm converged
      if (stat == success)
	{
	  //this is the solution the tracker algorithm found
	  // note this is current position
	  xzero = Contacts[track_id].Topt.xzero;
	  yzero = Contacts[track_id].Topt.yzero;
	  xdot  = Contacts[track_id].Topt.xdot;
	  ydot  = Contacts[track_id].Topt.ydot;
	  
	  MOOSTrace("xzero = %f, yzero = %f \n", xzero, yzero );
	  MOOSTrace("xdot = %f, ydot = %f \n", xdot, ydot );

	  //based on the solution what is the target range?
	  //	  x_pos_check = xzero + Contacts[track_id].total_time*xdot;
	  //	  y_pos_check = yzero + Contacts[track_id].total_time*ydot;
	  x_pos_check = xzero;
	  y_pos_check = yzero;
	  solver_range = sqrt(pow((x_pos_check-xnow),2) + pow((y_pos_check-ynow),2));
	  double solver_speed = sqrt(pow(xdot,2) +pow(ydot,2));
	  //is the solution within the detection range and realistic speed?
	  //make this a parameter
	  MOOSTrace("solver_range = %f, solver_speed = %f \n", solver_range, solver_speed );
	  if(solver_range < range_max && solver_range > 0.05 * range_max && 
	     solver_speed < speed_max )
	    {
	      //position
	      //	    Contacts[track_id].x_pos = xzero + Contacts[track_id].total_time*xdot;
	      //Contacts[track_id].y_pos = yzero + Contacts[track_id].total_time*ydot;
	      Contacts[track_id].x_pos = xzero;
	      Contacts[track_id].y_pos = yzero;
	      //bearing to target
	      Contacts[track_id].bearing = 90.0-atan2((Contacts[track_id].y_pos-ynow),(Contacts[track_id].x_pos-xnow))*180.0/M_PI;
	      if (Contacts[track_id].bearing  < 0.0)
		Contacts[track_id].bearing = Contacts[track_id].bearing +360.0;
	      if (Contacts[track_id].bearing  >= 360.0)
		Contacts[track_id].bearing = Contacts[track_id].bearing -360.0;
	      //speed of target
	      Contacts[track_id].speed = sqrt(xdot*xdot + ydot*ydot);
	      //target heading
	      Contacts[track_id].heading = 90.0-atan2(ydot,xdot)*180.0/M_PI;
	      if (Contacts[track_id].heading  < 0.0)
		Contacts[track_id].heading = Contacts[track_id].heading +360.0;
	      if (Contacts[track_id].heading  >= 360.0)
		Contacts[track_id].heading = Contacts[track_id].heading -360.0;
	      //indicate we have a full solution
	      Contacts[track_id].Ctype = RANGE_BEARING_HEADING_SPEED;
	      
	      if (!silent)
		MOOSTrace("Good solution: x = %f, y = %f, heading = %f, speed = %f\n",Contacts[track_id].x_pos,Contacts[track_id].y_pos,Contacts[track_id].heading,Contacts[track_id].speed);
	      //target range
	      Contacts[track_id].range = sqrt(pow((Contacts[track_id].x_pos-xnow),2) + pow((Contacts[track_id].y_pos-ynow),2));
	      // update track history and guess
	      Contacts[track_id].num_track += 1;
	      int num_max = Contacts[track_id].track_utc.size();
	      if (Contacts[track_id].num_track > num_max)
		{
		  for (int i=0; i<num_max-1; i++)
		    {
		      Contacts[track_id].track_utc[i] = Contacts[track_id].track_utc[i+1];
		      Contacts[track_id].track_x[i] = Contacts[track_id].track_x[i+1];
		      Contacts[track_id].track_y[i] = Contacts[track_id].track_y[i+1];
		    }
		  Contacts[track_id].num_track -=1;
		}
	      Contacts[track_id].track_utc[Contacts[track_id].num_track-1] =
		Contacts[track_id].Last_Meas_Time;
	      Contacts[track_id].track_x[Contacts[track_id].num_track-1] =
		Contacts[track_id].x_pos;
	      Contacts[track_id].track_y[Contacts[track_id].num_track-1] =
		Contacts[track_id].y_pos;
	      if (Contacts[track_id].num_track > num_max/2)
		{
		  std::vector<double> t,x,y;
		  t.resize(Contacts[track_id].num_track);
		  x.resize(Contacts[track_id].num_track);
		  y.resize(Contacts[track_id].num_track);
		  for(int i = 0; i<x.size(); i++)
		    {
		      t[i] =  Contacts[track_id].track_utc[i]; 
		      x[i] = Contacts[track_id].track_x[i]; 
		      y[i] = Contacts[track_id].track_y[i]; 
		    }
		  double slopex,slopey,slope_error;
		  double tlen = t[t.size()-1]-t[0];
		  double xlen = x[x.size()-1]-x[0];
		  double ylen = y[y.size()-1]-y[0];
		  CmyVectorUtils vec;
		  if ( fabs(xlen) >= fabs(ylen) )
		    {
		      vec.lin_least_square(x,y,Contacts[track_id].x_pos,
					   Contacts[track_id].yguess,slopey,
					   slope_error);
		      Contacts[track_id].xdotguess = xlen/tlen;
		      //		      Contacts[track_id].ydotguess = (Contacts[track_id].yguess-xlen*slopey)/tlen;
		      Contacts[track_id].ydotguess = Contacts[track_id].xdotguess*slopey;
		      Contacts[track_id].xguess = Contacts[track_id].x_pos;
		    }
		  else
		    {
		      vec.lin_least_square(y,x,Contacts[track_id].y_pos,
					   Contacts[track_id].xguess,slopex,
					   slope_error);
		      Contacts[track_id].ydotguess = ylen/tlen;
		      //		      Contacts[track_id].xdotguess = (Contacts[track_id].xguess-ylen*slopex)/tlen;
		      Contacts[track_id].xdotguess = Contacts[track_id].ydotguess*slopex;
		      Contacts[track_id].yguess = Contacts[track_id].y_pos;
		    }
		  double hdg_smooth = 180.0/M_PI * atan2(Contacts[track_id].xdotguess,
							 Contacts[track_id].ydotguess);
		  while (hdg_smooth >= 360.0)
		    hdg_smooth -= 360.0;
		  while (hdg_smooth < 0.0)
		    hdg_smooth += 360.0;
		  double spd_smooth =  sqrt(pow(Contacts[track_id].xdotguess,2)+
					    pow(Contacts[track_id].ydotguess,2));
		  MOOSTrace("Smoothed solution: x = %f, y = %f, heading = %f, speed = %f\n",Contacts[track_id].xguess,Contacts[track_id].yguess,hdg_smooth,spd_smooth);

		}
	      
	      tgt_msg_ready = 1;
	      tgt_track = true;
	    }
	  else
	    {
	      //	      Contacts[track_id].Topt.reset();
	      Contacts[track_id].Topt.set_reset_init(1);
	      MOOSTrace("Reset initial guess \n");
              tgt_track = false;
	    }
	      // initialize to smoothed track
	  if (Contacts[track_id].num_track > 1)
	    {
	      tgt_x = Contacts[track_id].xguess;
	      tgt_y = Contacts[track_id].yguess;
	      tgt_spd = sqrt(pow(Contacts[track_id].xdotguess,2)+
			     pow(Contacts[track_id].ydotguess,2));
	      tgt_hdg = atan2(Contacts[track_id].xdotguess,
			      Contacts[track_id].ydotguess)*180/M_PI;
	    }
	}
      else
	{
	  if (!silent)
	    MOOSTrace("No Solution\n");
	}  
    }

  return true;
}


void Contact::set_range_guess(double range)
{
  range_guess = range;
}

void Contact_List::get_Cinfo(int cid, CINFO *this_contact)
{
  time_t the_time;
  struct tm *tm_ptr;
  double Mtime;
  
  this_contact->id = Contacts[cid].id;
  this_contact->x_pos = Contacts[cid].x_pos;
  this_contact->y_pos = Contacts[cid].y_pos;
  this_contact->num_meas = Contacts[cid].num_meas;
  this_contact->range = Contacts[cid].range;
  this_contact->bearing = Contacts[cid].bearing;
  this_contact->heading = Contacts[cid].heading;
  this_contact->speed = Contacts[cid].speed;
  this_contact->Last_Meas_Time = Contacts[cid].Last_Meas_Time;
  
  Mtime = Contacts[cid].Last_Meas_Time;
  the_time = (time_t) Mtime;  tm_ptr = gmtime(&the_time);
  
  this_contact->hours   = tm_ptr->tm_hour; 
  this_contact->minutes = tm_ptr->tm_min;
  this_contact->seconds = tm_ptr->tm_sec;
  this_contact->Ctype   = Contacts[cid].Ctype;
}


CINFO::CINFO()
{
}

CINFO::~CINFO()
{
}


