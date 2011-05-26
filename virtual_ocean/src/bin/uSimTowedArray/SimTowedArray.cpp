//======================================================================
// uSimTowedArray
// Copyright Henrik Schmidt, MIT 
// Towed array simulator
// Translated from matlab version ArraySim.m
// 
#include "MBUtils.h"
// #include "AngleUtils.h"
#include "SimTowedArray.h"

#include <sstream>

using namespace std;

CSimTowedArray::CSimTowedArray()
{
  MOOSTrace("====================================\n");
  MOOSTrace("\n");
  MOOSTrace("             pSimTowedArray\n");
  MOOSTrace("\n");
  MOOSTrace("        V 1.0 10/06/2008 \n");
  MOOSTrace("\n");
  MOOSTrace("(c) Copyright, 2007 Henrik Schmidt MIT\n");
  MOOSTrace("====================================\n");

  array_type = "towed";

  // Tow point offset default for Unicorn
  tow_off_x = 0;
  tow_off_y = -2.1;
  tow_off_z = -0.25;

    tow_pos_x = 0;
    tow_pos_y = 0;
    tow_pos_z = 0;

    tow_vel_x = 0;
    tow_vel_y = 0;
    tow_vel_z = 0;

    first = false;
    ok_x = false;
    ok_y = false;
    ok_z = false;

    ok_spd = false;
    ok_yaw = false;
    ok_pitch = false;

    got_spd = false;
    got_yaw = false;
    got_pitch = false;

    m_dummy.push_back(0.0);

    // ======================================================================
    // Towed array simulation.
    //======================================================================
    Array_init=false;

    //======================================================================
    // Initialize tension filter
    // This seems to be necessary to prevent the array solution from
    // blowing up. This is probably the equivalent of towing with a
    // spring/damper arrangement at the towpoint (?).
    //======================================================================
    history_time = 20;
    filter_length = 40;
    filter_v = 30;
    Array.count = 0;
    B=1;
    hist_length=15;
    xtowp_hist.clear();
    ytowp_hist.clear();
    get_back = 0;

    MOOSTrace("Clearing vectors\n");

    tfilter.clear();  
    for (int i=0;i<filter_length;i++)
      tfilter.push_back(0.0);

    // History of velocity providing sufficient speed for integration
    vxfilter = vecUtils.zeros(1,filter_v);
    vyfilter = vecUtils.zeros(1,filter_v);
    vzfilter = vecUtils.zeros(1,filter_v);

    // History of all velocities
    vx_hist = vecUtils.zeros(1,filter_v);
    vy_hist = vecUtils.zeros(1,filter_v);
    vz_hist = vecUtils.zeros(1,filter_v);
    first = true;
    pp=0;

    got_dir = false;                               //DHG 2007/03/14
    got_frame = false;                             //DHG 2007/03/14

    start_time = MOOSTime();
    prev_time = start_time;
    iterate_time = 0;
}

CSimTowedArray::~CSimTowedArray()
{
}

bool CSimTowedArray::OnStartUp()
{
    //Initialize m_Geodesy from lat lon origin in .moos file
    string sVal;
    double dfLatOrigin;
    double dfLongOrigin;
    
    if(m_MissionReader.GetValue("LatOrigin",sVal))
    {
        dfLatOrigin = atof(sVal.c_str());
        MOOSTrace("  LatOrigin  = %10.5f deg.\n",dfLatOrigin);
    }
    else
    {
        MOOSTrace("LatOrigin not specified in mission file - FAIL\n");
        return false;
    }
    
    if(m_MissionReader.GetValue("LongOrigin",sVal))
    {
        dfLongOrigin = atof(sVal.c_str());
        MOOSTrace("  LongOrigin = %10.5f deg.\n",dfLongOrigin);
    }
    else
    {
        MOOSTrace("LongOrigin not specified in mission file - FAIL\n");
        return false;
    }

    if(!m_Geodesy.Initialise(dfLatOrigin,dfLongOrigin))
    {
        MOOSTrace("Geodesy Init inside pSimTowedArray failed - FAIL\n");
        return false;
    }
    //Done initializaing m_Geodesy

    if(m_MissionReader.GetConfigurationParam("cond_check",sVal))
    {
      cond_check = atoi(sVal.c_str());
    }
    else
    {
        MOOSTrace("pSimTowedArray:: cond_check not specified - FAIL\n");
        return false;
    }

    if(m_MissionReader.GetConfigurationParam("start_speed",sVal))
    {
      start_speed = atof(sVal.c_str());
    }
    else
    {
        MOOSTrace("pSimTowedArray:: start_speed not specified - FAIL\n");
        return false;
    }

    if(m_MissionReader.GetConfigurationParam("filter_length",sVal))
    {
      filter_length = atoi(sVal.c_str());
      tfilter = vecUtils.zeros(1,filter_length);
      filter_v = filter_length;
    }
    else
    {
        MOOSTrace("pSimTowedArray:: filter_length not specified - FAIL\n");
        return false;
    }

    if(m_MissionReader.GetConfigurationParam("history_time",sVal))
    {
      history_time = atof(sVal.c_str());
    }

    if(m_MissionReader.GetConfigurationParam("array_type",sVal))
    {
      if (MOOSStrCmp(sVal,"fixed"))
	array_type = sVal;
      else
	array_type = "towed";

      MOOSTrace("Array is %s\n",array_type.c_str());
    }
    else
    {
      MOOSTrace("Array is %s\n",array_type.c_str());
    }

    if(m_MissionReader.GetConfigurationParam("tow_point_x",sVal))
    {
      tow_off_x = atof(sVal.c_str());
    }

    if(m_MissionReader.GetConfigurationParam("tow_point_y",sVal))
    {
      tow_off_y = atof(sVal.c_str());
    }

    if(m_MissionReader.GetConfigurationParam("tow_point_z",sVal))
    {
      tow_off_z = atof(sVal.c_str());
    }

    if(m_MissionReader.GetConfigurationParam("number_sectors",sVal))
      {
	int num_sec = atoi(sVal.c_str());
	MOOSTrace("Reading %d sector parameters\n", num_sec);

	Array.Nsect = num_sec;
	Array.Sectors.clear();
	// read sector parameters
	for (int i=0; i<num_sec; i++)
	  {
	    string param;
	    string svalues;
	    char buff[12];
	    sprintf(buff,"sector_%1d",i+1);
	    param = buff;
	    bool sec_info = m_MissionReader.GetConfigurationParam(param,svalues);
	    if(sec_info)
	      {
		MOOSTrace("Reading %s \n", param.c_str());

		int nparam = vecUtils.str2dbl_vec(svalues,m_buffer);
		CSector Sect;
		if (Sect.setSectorValues(m_buffer))
		  {
		      Array.Sectors.push_back(Sect);
		  }
		else
		  {
		    MOOSTrace("pSimTowedArray::%s: Insufficient number of parameters - FAIL\n",param.c_str());
		    return false;
		  }
	      }
	  else
	    {
	      MOOSTrace("pSimTowedArray::%s not specified - FAIL\n",param.c_str());
	      return false;
	    }
	  }
      }
    else 
      {
	MOOSTrace("pSimTowedArray:: number_sectors not specified - FAIL\n");
	return false;
      }
    
    if(m_MissionReader.GetConfigurationParam("tow_body",sVal))
      {
	int ntowbody = atoi(sVal.c_str());
	MOOSTrace("Reading %d towbody parameters\n", ntowbody);
	if (ntowbody == 1)
	  {
	    Array.towbody = true;
	    bod.clear();
	    // read towbody parameters
	    string svalues;
	    if(m_MissionReader.GetConfigurationParam("towbody_parameters",svalues))
	      {
		int nparam = vecUtils.str2dbl_vec(svalues,m_buffer);
		CTowbody Towb;
		if (Towb.setTowbodyValues(m_buffer))
		  {
		    Array.Towbody =  Towb;
		  }
		else
		  {
		    MOOSTrace("pSimTowedArray::towbody_parameters: Insufficient number of parameters - FAIL\n");
		    return false;
		  }
	      }
	    else
	      {
		MOOSTrace("pSimTowedArray:towbody_parameters not specified - FAIL\n");
		return false;
	      }
	  }
	else
	  Array.towbody = false;
      }
    else if (array_type == "towed")
      {
	MOOSTrace("pSimTowedArray:: tow_body not specified - FAIL\n");
	return false;
      }
    

    if(m_MissionReader.GetConfigurationParam("array_section_number",sVal))
    {
      Array.array_section_number = atoi(sVal.c_str());
    }
    else 
    {
        MOOSTrace("pSimTowedArray:: array_section_number not specified - FAIL\n");
        return false;
    }

    if(m_MissionReader.GetConfigurationParam("first_phone_offset",sVal))
    {
      Array.first_phone_offset = atof(sVal.c_str());
    }
    else 
    {
        MOOSTrace("pSimTowedArray:: first_phone_offset not specified - FAIL\n");
        return false;
    }

    if(m_MissionReader.GetConfigurationParam("phone_spacing_groups",sVal))
      {
	int num_spacing_groups = atoi(sVal.c_str());
	MOOSTrace("Reading %d phone spacings\n", num_spacing_groups);
	vector<double> spacings;
	spacings.push_back(0.0);
	double spac;
	for (int i=0; i<num_spacing_groups; i++)
	  {
	    string param;
	    string svalues;
	    char buff[24];
	    sprintf(buff,"spacing_group_%1d",i+1);
	    param = buff;
	    if (m_MissionReader.GetConfigurationParam(param,svalues))
	      {
		spac = atof(svalues.c_str());
	      }
	    else
	      {
		MOOSTrace("pSimTowedArray::%s not specified - FAIL\n",param.c_str());
		return false;
	      }
	    sprintf(buff,"num_spacings_group_%1d",i+1);
	    param = buff;
	    if (m_MissionReader.GetConfigurationParam(param,svalues))
	      {
		int num_spac = atoi(svalues.c_str());
		for (int i=0; i<num_spac; i++)
		  spacings.push_back(spac);
	      }
	    else
	      {
		MOOSTrace("pSimTowedArray::%s not specified - FAIL\n",param.c_str());
		return false;
	      }
	  }
	vecUtils.cumsum(spacings,Array.phone_position);
	Array.m_nelm = Array.phone_position.size();
	spacings[0]=Array.first_phone_offset;
	vecUtils.cumsum(spacings,Array.phone_offset);
      }
    else 
      {
	MOOSTrace("pSimTowedArray:: phone_spacing_groups not specified - FAIL\n");
	return false;
      }
    

    return(true);
}

bool CSimTowedArray::OnConnectToServer()
{
 
    m_Comms.Register( "NAV_X",0);
    m_Comms.Register( "NAV_Y",0);
    m_Comms.Register( "NAV_Z",0);
    m_Comms.Register( "NAV_SPEED",0);
    m_Comms.Register( "NAV_YAW",0);
    m_Comms.Register( "NAV_PITCH",0);
    return(true);
}


bool CSimTowedArray::Iterate()
{

  // first check that we got all nav and compute tow point dynamics
  if (towing = (ok_x && ok_y && ok_z && ok_spd && ok_yaw && ok_pitch))
    {
      double cy = cos(nav_yaw);
      double sy = sin(nav_yaw);
      double cp = cos(nav_pitch);
      double sp = sin(nav_pitch);

      tow_pos_x = nav_x + (tow_off_x*cy - tow_off_y * sy)*cp; 
      tow_pos_y = nav_y + (tow_off_x*sy + tow_off_y * cy)*cp;
      tow_pos_z = nav_z + tow_off_z * cp + tow_off_y *sp; 
      tow_vel_x =-nav_spd * cp * sy;
      tow_vel_y = nav_spd * cp * cy;
      // speed is implemented as pos up (right handed coordinate system)
      tow_vel_z = nav_spd * sp; 
    }
    // Make sure array is towing before computing time series

    double vx,vy,vz;
    vx = tow_vel_x;
    vy = tow_vel_y;
    vz = tow_vel_z;

    double curr_time = MOOSTime();
    iterate_time = 0.95*iterate_time + 0.05* (curr_time-prev_time);
    //fixed number in average seems more robust
    //filter_v = int(history_time/iterate_time);
    prev_time = curr_time;

    while (vx_hist.size() > filter_v)
      {
	vx_hist.pop_back();
	vy_hist.pop_back();
	vz_hist.pop_back();
      }

    while (vx_hist.size() < filter_v)
      {
	if (int ll = vx_hist.size() > 0)
	  {
	    vx_hist.push_back(vx_hist[ll-1]);
	    vy_hist.push_back(vy_hist[ll-1]);
	    vz_hist.push_back(vz_hist[ll-1]);
	  }
	else
	  {
	    vx_hist.push_back(0.0);
	    vy_hist.push_back(0.0);
	    vz_hist.push_back(0.0);
	  }
      }

    MOOSTrace("filter_v = %d\n",filter_v);

    if (towing)
      {
	if (got_spd && got_yaw && got_pitch)
	  {
	    // Stores the speed history of the vehicle
	    vx = tow_vel_x;
	    vy = tow_vel_y;
	    vz = tow_vel_z;
	    
	    towing_speed = sqrt(pow(vx,2)+pow(vy,2)+pow(vz,2));
	    MOOSTrace("Towing speed: %f\n",towing_speed);
	    //	    if (towing_speed > start_speed)
	    //  {
		pp=pp+1;
		for (int i=vx_hist.size()-1; i>0; i--)
		  {
		    vx_hist[i] =vx_hist[i-1];
		    vy_hist[i] =vy_hist[i-1];
		    vz_hist[i] =vz_hist[i-1];
		  }       
		vx_hist[0] = vx;
		vy_hist[0] = vy;
		vz_hist[0] = vz;
		
	    MOOSTrace("towpoint: vx=%f , vy=%f, vz=%f, heading=%f\n", vx,vy,vz, 90-180/M_PI*atan2(vy,vx));
	    //  }
	    got_spd = false;
	    got_yaw = false;
	    got_pitch = false;
	    MOOSTrace("pp = %d\n",pp);

	  }
	else
	  return(true);

	if ( pp>hist_length )
	  {
	    //==================================================================
	    // Run array model
	    //==================================================================

	    if (towing_speed > start_speed && Array_init == false)
	      {
		double xtowp = tow_pos_x;
		double ytowp = tow_pos_y;
		// note ztowp pos down
		double ztowp = -tow_pos_z;
		Array.count = 0;
		Array.time = curr_time - start_time;
		
		double vx_f=vecUtils.mean(vx_hist);
		double vy_f=vecUtils.mean(vy_hist);
		double vz_f=0;
		MOOSTrace("mean vx,vy= %f, %f \n", vx_f,vy_f);
		towing_speed = sqrt(pow(vx_f,2)+pow(vy_f,2)+pow(vz_f,2));
		MOOSTrace("Towing speed: %f\n",towing_speed);
		
	    MOOSTrace("function_steady: vx=%f , vy=%f, vz=%f, heading=%f\n", vx_f,vy_f,vz_f, 90-180/M_PI*atan2(vy_f,vx_f));
		// Solve for steady-state array configuration using the mean
		// velocity of the vehicle

		Array.function_steady(xtowp, ytowp, ztowp, vx_f, vy_f, vz_f);
		MOOSTrace("Array model Initialized\n");
		Array_init=true;
		nt=0;
	      }
	    else if (!Array_init)
	      {
		MOOSTrace("WARNING: Insufficient vehicle speed for solution, vertical array assumed.\n");
		m_Comms.Notify("CABLE_TENSION",0.0);
	      }
	    
	  } // end if (pp > hist_length)
    
	    // At low speed assume straight array
	
	if (array_type == "towed" && towing_speed > start_speed && Array_init == true)   
	  // No integration if vehicle speed is too small
	  
	  { 
	    double ytowp = tow_pos_x;
	    double xtowp = tow_pos_y;
	    // note ztowp pos down
	    double ztowp = -tow_pos_z;
	    Array.time = curr_time - start_time;
	    
	    double vx_f=vecUtils.mean(vx_hist);
	    double vy_f=vecUtils.mean(vy_hist);
	    double vz_f=vecUtils.mean(vz_hist);
	    
	    towing_speed = sqrt(pow(vx_f,2)+pow(vy_f,2)+pow(vz_f,2));

	    if (first)
	      {
		vz_f=0;
		first = false;
	      }
	    MOOSTrace("SimTowedArray: vx=%f , vy=%f, vz=%f, heading=%f\n", vx_f,vy_f,vz_f, 90-180/M_PI*atan2(vy_f,vx_f));
	    
	    if (!Array.function_cable(nt, Array_init, xtowp, ytowp, ztowp, vx_f, vy_f, vz_f))
	      return(false);

	    if (Array.count > 3)
	      {
		MOOSTrace("initialization of the array in process... \n ");
		Array_init = false;
		m_Comms.Notify("CABLE_TENSION",0.0);
		Array.count = 0;
	      }
	    
	    // Publish average array heading and pitch for beam tracking
	    
	    int nelm = Array.array_head.size();
	    MOOSTrace("Number of elements = %d\n",nelm);

	    // unwrap array element headings
	    vector<double> a_head_buf;
	    a_head_buf = vecUtils.zeros(1,nelm);
	    vecUtils.vmov(Array.array_head,a_head_buf);
	    
	    // for ih=2:length(array_head) 
	    for (int ih=1; ih<Array.array_head.size(); ih++)
	      {
		while (a_head_buf[ih] - a_head_buf[ih-1] > 180.0)
		  a_head_buf[ih]=a_head_buf[ih] - 360;
		
		while (a_head_buf[ih] - a_head_buf[ih-1] <= -180.0)
		  a_head_buf[ih]=a_head_buf[ih] + 360;
	      }

	    MOOSTrace("mean_heading = %f\n",vecUtils.mean(a_head_buf));
	    
	    ael_heading=angleUtils.angle360(vecUtils.mean(a_head_buf)); // added by AD 072207
	    MOOSTrace("ael_heading = %f\n",ael_heading);

	    ael_pitch=vecUtils.mean(Array.array_pitch);
	    
	    // Filter the cable tension
	    cable_tension = Array.array_tension;
	    if (cable_tension > 0 && cable_tension < 200)
	      {
		vecUtils.vshift(tfilter, 1, tfilter, 0, tfilter.size() - 1);
		tfilter[tfilter.size()-1] = cable_tension;
		filt_tension=vecUtils.mean(tfilter);
		MOOSTrace("Cable tension: %f\n",filt_tension);
		
		 m_Comms.Notify("CABLE_TENSION",filt_tension);
		 m_Comms.Notify("ARRAY_HEADING",Array.arr_heading);
		 m_Comms.Notify("ARRAY_PITCH",  Array.arr_pitch);
		 m_Comms.Notify("CABLE_SHAPE_X",Array.x_array);
		 m_Comms.Notify("CABLE_SHAPE_Y",Array.y_array);
		 m_Comms.Notify("CABLE_SHAPE_Z",Array.z_array);
		 m_Comms.Notify("ARRAY_X", Array.ac_array_x);
		 m_Comms.Notify("ARRAY_Y", Array.ac_array_y);
		 m_Comms.Notify("ARRAY_Z", Array.ac_array_z);
		
		 MOOSTrace("*");
		m_Comms.Notify("AEL_PITCH",ael_pitch);
		//		m_Comms.Notify("AEL_PITCH",ael_pitch*M_PI/180.0);
		m_Comms.Notify("AEL_HEADING",ael_heading);
		m_Comms.Notify("AEL_X_OFFSET",Array.ael_x_offset);
		m_Comms.Notify("AEL_Y_OFFSET",Array.ael_y_offset);
		m_Comms.Notify("AEL_X",Array.ael_x);
		m_Comms.Notify("AEL_Y",Array.ael_y);
	      }
	    else
	      {
		MOOSTrace("\nCable Tension is out of bounds: %f\n", cable_tension);
		m_Comms.Notify("CABLE_TENSION",0.0);
	      }
	  }
	else
	  {
            if (array_type == "towed")
	      {
		if (! Array_init) 
		  {
		    std::cout << "WARNING: Insufficient vehicle speed for solution, array assumed vertical." << std::endl;
		  }
		else 
		  {
		    std::cout << "WARNING: Insufficient vehicle speed for solution, array initialised but tow speed (" << towing_speed << ") less than the start speed (" << start_speed << "). Straight pitched array assumed. " << std::endl;
		  }
	      }
	    double horiz_speed = sqrt(pow(vx,2)+pow(vy,2));
	    //	    double theta = (1-horiz_speed/start_speed)*(-90.0);
	    if (array_type == "towed")
	      ael_pitch = -acos(pow(horiz_speed/start_speed,2));
	    else
	      ael_pitch = nav_pitch;
 
	    Array.straight_array(tow_pos_x, tow_pos_y, tow_pos_z, 
				 tow_vel_x, tow_vel_y, tow_vel_z, nav_yaw,  
				 ael_heading, ael_pitch );

	    m_Comms.Notify("CABLE_TENSION",0);
	    m_Comms.Notify("ARRAY_HEADING",Array.arr_heading);
	    m_Comms.Notify("ARRAY_PITCH",  Array.arr_pitch);  
	    m_Comms.Notify("CABLE_SHAPE_X",Array.x_array);
	    m_Comms.Notify("CABLE_SHAPE_Y",Array.y_array);
	    m_Comms.Notify("CABLE_SHAPE_Z",Array.z_array);
	    m_Comms.Notify("ARRAY_X", Array.ac_array_x);
	    m_Comms.Notify("ARRAY_Y", Array.ac_array_y);
	    m_Comms.Notify("ARRAY_Z", Array.ac_array_z);
	    m_Comms.Notify("ARRAY_HEADING",Array.arr_heading);  
	    m_Comms.Notify("ARRAY_PITCH",  Array.arr_pitch);
	    m_Comms.Notify("AEL_PITCH",ael_pitch);
	    m_Comms.Notify("AEL_HEADING",ael_heading);
	    m_Comms.Notify("AEL_X_OFFSET",Array.ael_x_offset);
	    m_Comms.Notify("AEL_Y_OFFSET",Array.ael_y_offset);
	    m_Comms.Notify("AEL_X",Array.ael_x);
	    m_Comms.Notify("AEL_Y",Array.ael_y);
	    Array_init=false;
	    Array.count =0;
	    if (pp > hist_length) pp = 0;
	  }
      } // end if (towing)
    else
      {
        std::cout << "WARNING: not towing." << std::endl;
      }
    
    return(true);
}


//-------------------------------------------------------------
// Procedure: OnNewMail

bool CSimTowedArray::OnNewMail(MOOSMSG_LIST &NewMail)

{

  MOOSMSG_LIST::iterator p;

  for(p = NewMail.begin(); p != NewMail.end(); p++) {
    CMOOSMsg &Msg = *p;

      if(Msg.m_sKey == "NAV_X")
      {
	nav_x = Msg.m_dfVal;
	ok_x = true;
      }

      if(Msg.m_sKey == "NAV_Y")
      {
	nav_y = Msg.m_dfVal;
	ok_y = true;
      }

      if(Msg.m_sKey == "NAV_Z")
      {
	// note that tow z in pArraysim is a positive depth. Taken care of in Iterate 
	nav_z = Msg.m_dfVal;
	ok_z = true;
      }

      if(Msg.m_sKey == "NAV_YAW")
      {
	nav_yaw = Msg.m_dfVal; 
	ok_yaw = true;
	got_yaw = true;
      }

      if(Msg.m_sKey == "NAV_SPEED")
      {
	nav_spd = Msg.m_dfVal;
	ok_spd = true;
	got_spd = true;
      }

      if(Msg.m_sKey == "NAV_PITCH")
      {
	// note that tow z is published as positive depth 
	nav_pitch = Msg.m_dfVal;
	ok_pitch = true;
	got_pitch = true;
      }


   }
  
  return(true);
}

// >>>>>>>>>>>>> Array sector variables class <<<<<<<

bool CSector::setSectorValues(vector<double>& values)
{
  if (values.size() == 7)
    {
      Nsub = int(values[0]);
      len    = values[1];
      wgt    = values[2];
      dia    = values[3];
      td     = values[4];
      nd     = values[5];
      modEla = values[6];
      return(true);
    }
  else
    return(false);
}

// >>>>>>>>>>>> End Sector class <<<<<<<<<<<<<<<

// >>>>>>>>>>>>> Towbody variables class <<<<<<<

bool CTowbody::setTowbodyValues(vector<double>& values)
{
  if (values.size() == 3)
    {
      wgt        = values[0];
      drgcoef    = values[1];
      area       = values[2];
      return(true);
    }
  else
    return(false);
}

// >>>>>>>>>>>> End Towbody class <<<<<<<<<<<<<<<

// >>>>>>>>>>>>>>> angle utility methods <<<<<<<<<<<<
double CmyAngleUtils::angle360(double angle)
{
  double res = angle;
  //  unwrap angles
  while (res >= 360.0)
    res -= 360.0;
  while (res < 0.0)
    res += 360.0;
  
  return(res);
  
}

// >>>>>>>>>>>>>> Array dynamic model class <<<<<<<<<<<<<<<<

CArrayModel::CArrayModel()
{
  // Constructor
    old_ynew.clear();
    old_bigy.clear();
    old_bigyold.clear();
    old_bigynew.clear();
    old_InvJacob.clear();
    old_Jacobo.clear();

  array_head.clear();
  array_pitch.clear();
  dtfilt_length = 20;


}

CArrayModel::~CArrayModel()
{
  // Destructor
}

int CArrayModel::indx(int n_rows, int row, int col)
{
  // Corresponds to fortran starge by columns.
  return((col-1)*n_rows + (row-1));
} 

bool CArrayModel::straight_array(double tow_pos_x, double tow_pos_y, 
				 double tow_pos_z, double tow_vel_x, 
				 double tow_vel_y, double tow_vel_z, double nav_yaw, 
				 double & ael_heading, double & ael_pitch )
{
  // This function is a simple model that assumes the array straight behind the vehicle at a prescribed pitch angle.


  //****************************************************************************
  // 		DEFINE THE CABLE SECTION DISCRETIZATION (length and material)
  //****************************************************************************
  
  // Properties starting from the tow point and ending at the tow body.
  
  cable_disc();

  // Array locations along array
  
  offset_to_array = cablel[array_section_number-2]  ; 
  
  compass_offset_full.clear();
  depth_offset_full.clear();
  pitch_offset_full.clear();
  for (int i=0; i < phone_offset.size(); i++)
    {
      compass_offset_full.push_back(phone_offset[i]);
      depth_offset_full.push_back(phone_offset[i]);
      pitch_offset_full.push_back(phone_offset[i]);
    }

  vector<double> buf_xi;
  buf_xi.clear();
  for (int i=0; i<m_nelm; i++)
    buf_xi.push_back(offset_to_array + phone_offset[i]);

  double array_angle;
  if (pow(tow_vel_x,2)+pow(tow_vel_y,2) < 1e-6)
      array_angle = nav_yaw+0.5*M_PI;
  else
      array_angle = atan2(tow_vel_y, tow_vel_x);
  ael_heading = angleUtils.angle360(90.0-array_angle*180/PI);
  MOOSTrace("ael_heading = %f\n");

  std::vector<double> x1;
  std::vector<double> x2;
  std::vector<double> y1;
  std::vector<double> y2;
  std::vector<double> z1;
  std::vector<double> z2;

  x1.clear();
  x2.clear();
  y1.clear();
  y2.clear();
  z1.clear();
  z2.clear();
  array_head.clear();
  array_pitch.clear();

  for (int i=0; i< m_nelm; i++)
    {
      array_head.push_back(ael_heading);
      array_pitch.push_back(ael_pitch);
    }

  for (int i=0; i<sr.size(); i++)
    {
      x1.push_back(tow_pos_x-sr[i]*cos(array_angle)*cos(ael_pitch));
      y1.push_back(tow_pos_y-sr[i]*sin(array_angle)*cos(ael_pitch));
      z1.push_back(tow_pos_z-sr[i]*sin(ael_pitch));
    }

  vecUtils.dbl_vec2str(x1, x_array);
  vecUtils.dbl_vec2str(y1, y_array);
  vecUtils.dbl_vec2str(z1, z_array);

  for (int i=0; i<m_nelm; i++)
    {
      x2.push_back(tow_pos_x-buf_xi[i]*cos(array_angle)*cos(ael_pitch));
      y2.push_back(tow_pos_y-buf_xi[i]*sin(array_angle)*cos(ael_pitch));
      z2.push_back(tow_pos_z-buf_xi[i]*sin(ael_pitch));
    }

  vecUtils.dbl_vec2str(x2, ac_array_x);
  vecUtils.dbl_vec2str(y2, ac_array_y);
  vecUtils.dbl_vec2str(z2, ac_array_z);

  ael_x = vecUtils.mean(x2);
  ael_y = vecUtils.mean(y2);
  ael_x_offset = ael_x - tow_pos_x;
  ael_y_offset = ael_y - tow_pos_y;

  vecUtils.dbl_vec2str(array_head, arr_heading);
  MOOSTrace("arr_heading = %s\n",arr_heading.c_str());
  vecUtils.dbl_vec2str(array_pitch, arr_pitch);

  return(true);
}

bool CArrayModel::function_steady(double xtowp, double ytowp, double ztowp, double vx, double vy, double vz)
{
  
  var_sus=5;	//number of variables subscribed
  
  double cvx=vx;
  double cvy=vy;
  double cvz=vz;
  
  double dt = 0.8;	// update rate for model (sec)
  double dt1=dt;
  deltim = dt;
  
  over_extrap = 0; 
  
  
  double temp = cvx;     // convert to "model inertial coordinates" (reverse vx and vy)
  cvx = cvy;
  cvy = temp;
  
  double towpvx = cvx;	
  double towpvy = cvy;	
  
  double towphead = atan2(-towpvy,towpvx);
  
  if (towphead < 0)
    towphead = 2* M_PI + towphead;
  
  double towvelstart = sqrt( pow(towpvx,2) + pow(towpvy,2) );
  towvel=towvelstart; 
  double radheadstart = atan2(-towpvy,towpvx);  // these are in the "model inertial frame"
  
  // towelstart and radheadstart define the initial velocity and course of the vehicule 
  
  if (radheadstart < 0)
    radheadstart = 2*M_PI + radheadstart;
  
  double towpx=ytowp;		// reverse towpx and towpy because be consistent with the speed coordinates
  double towpy=xtowp;	       // reverse towpx and towpy because be consistent with the speed coordinates
  double towpz=ztowp;	       // ztowp and towpz are positive values 
  
  //****************************************************************************
  //		INPUT CABLE AND BODY PROPERTIES
  //****************************************************************************
  
  //****************************************************************************
  // 		TOW SHIP PARAMETERS AND OTHER VARIABLES
  //****************************************************************************  
  double keel_to_tp_offset = 0;   //  keel to towpoint offset ie subtract from ship depth
  
  ztowp = ztowp - keel_to_tp_offset;
  
  rho=1025.0;		// fluid density (kg/m^3) 
  
  g=9.81;			// acceleration due to gravity (m/s)
  
  towvel=towvelstart;	// initial tow velocity
  
  
  
  // ***************************************************************************
  // 		INPUT CURRENT PROFILE
  // ***************************************************************************
  
  // this is a read to a dummy file which has zero current; at present time of currents go from 0 to 10000 sec.  
  // There will be an error if the model times exceed this. I am keeping current in since they can 
  // be important. Later on I will put in current values
  
  double minT=0;
  
  int ith_order = 3;
  
  vector<double> cx, cy;
  
  //  this function returns the current profile.

  poly_curr_grad_ith_auv(ith_order );	
  
  int start_ind_curr = -1;
  for ( int i=0; i<currtim.size(); i++)
    {
      if ( start_ind_curr < 0 && currtim[i] >= minT )
	start_ind_curr = i;
    }

  if (start_ind_curr < 0)
    start_ind_curr = 0;

  // start_ind_curr = min( find( currtim >= minT ) );
  
  // These are in standard Cartesian coordinates ie cx is e-w, cy is n-s; now convert to 'model inertial coordinates ie reverse cx and cy
 
  vector<double> vtemp;
  vtemp = vecUtils.zeros(cx.size(),1);
  vecUtils.vmov(cx,vtemp);
  vecUtils.vmov(cy,cx);
  vecUtils.vmov(vtemp,cy);
  
  vtemp = vecUtils.zeros(pcx.size(),1);
  vecUtils.vmov(pcx,vtemp);
  vecUtils.vmov(pcy,pcx);
  vecUtils.vmov(vtemp,pcy);
  
  vtemp = vecUtils.zeros(qgx.size(),1);
  vecUtils.vmov(qgx,vtemp);
  vecUtils.vmov(qgy,qgx);
  vecUtils.vmov(vtemp,qgy);
  
  
  //****************************************************************************
  // 		DEFINE THE CABLE SECTION DISCRETIZATION (length and material)
  //****************************************************************************
  
  // Properties starting from the tow point and ending at the tow body.
  
  cable_disc();
  
  // Find the tow point velocity and heading through the water,based on the surface current (cx(1),cy(1). 
  
  // These are passed to steady_state for both the cable and the arrays! 
  
  // cx and cy vectors gives the current speed in the inertial coordinates
  
  double avgcx = 0; //sum(cx)/length(cx);
  double avgcy = 0; //sum(cy)/length(cy);
  
  double towater=sqrt(pow((-towvel*sin(radheadstart)-avgcy),2)+pow((towvel*cos(radheadstart)-avgcx),2)) ;
  
  double radheadwater=atan2(towvel*sin(radheadstart)+avgcy,towvel*cos(radheadstart)-avgcx) ;
  // MOOSTrace("towater, radheadwater= %f, %f\n", towater, radheadwater);
  
  if (radheadwater < 0.0)
    radheadwater=2.0*M_PI+radheadwater;
  
  // >>>>>>>>>>>>>>>>> Here goes all the remaining code from function_steady 
  
  //***********************************************************************************************************
  // 		TOW CABLE STEADY STATE SOLUTION
  //***********************************************************************************************************
  
  // Find an approximation to the steady state cable configuration and store in yc.
  
  // [yc,tbmass,vol]=steady_state(towbody,g,rho,wgt,dragcoef,area,towater,Nseg,weight,dia,tandrag,nordrag,cablel,ss,wgtns,dians,tandns,nordns,radheadwater);
  
  steady_state(towater,radheadwater);
  
// ss reprensents the endpoints
  
  ycold = vecUtils.zeros(6,Nseg+1);
  
  // Find the steady state cable position (xss,zss) by integrating (xc',zc') in space and adding (xtowp,ztowp).
  
  // the following comes from page 517 of the article 

  // vector<double> xsprime;
  // vector<double> xss;

  // xsprime.clear(); 

  // for (int j=1; j <= Nseg+1; j++)
  //   xsprime.push_back(cos( yc[indx(6,5,j)] ) * cos( yc[indx(6,6,j)] ));

  // vecUtils.cumtrapz(sr,xsprime, xss);
  // xsprime= cos(yc(5,1:Nseg+1)).*cos(yc(6,1:Nseg+1)); 	
  // xss(1:Nseg+1)=cumtrapz(sr,xsprime)+towpx;		% sr = ss in reverse order
  
  // ysprime=-sin(yc(5, 1:Nseg+1)).*cos(yc(6,1:Nseg+1));
  // yss(1:Nseg+1)=cumtrapz(sr,ysprime)+towpy;
  
  // zsprime=-sin(yc(6,1:Nseg+1));
  // zss(1:Nseg+1)=cumtrapz(sr,zsprime)+towpz;
  
  // Initialize the current and its gradient along the cable
  
  // FOR Ith order use
  
  // Everwhere polyval is called you need to center and scale the depth array that is passed to polyval, e.g.
  
  // zss(1:Nseg+1)=cumtrapz(sr,zsprime)+towpz;	%center and scale
  
  // zss_dmu=(zss-dmu(1))/dmu(2);
  
  // Initialize the current and its gradient along the cable
  
  // cxc= polyval(pcx(start_ind_curr,:),zss_dmu);
  // cyc= polyval(pcy(start_ind_curr,:),zss_dmu);
  // gxc= polyval(qgx(start_ind_curr,:),zss_dmu);
  // gyc= polyval(qgy(start_ind_curr,:),zss_dmu);
  
  
  // INPUT KALMAN FILTER PROPERTIES AND SET UP KALMAN
  
  // INPUT TOW CABLE PARAMETERS
  
  // Nstates  = 6*(Nseg +1);

  // Set all current values to 0  

  cxc = vecUtils.zeros(1,Nseg+1);
  cyc = vecUtils.zeros(1,Nseg+1);
  gxc = vecUtils.zeros(1,Nseg+1);
  gyc = vecUtils.zeros(1,Nseg+1);


  offset_to_array = cablel[array_section_number-2]  ; 
  
  compass_offset_full.clear();
  depth_offset_full.clear();
  pitch_offset_full.clear();
  for (int i=0; i < phone_offset.size(); i++)
    {
      compass_offset_full.push_back(phone_offset[i]);
      depth_offset_full.push_back(phone_offset[i]);
      pitch_offset_full.push_back(phone_offset[i]);
    }
  
  old_time = time;
  
  return(true);
}

bool CArrayModel::cable_disc()
{
  //function [cablel,wgtns,dians,tandns,nordns,sr,dels,s,iendsect, ...
  //          ss,Nseg,weight,dia,tandrag,nordrag,mass,mass1,ext]= ...
  //          cable_disc(Nsect,cab,g,rho);
  
  
  // This function defines the section discretization, length and material// properties starting from the tow point and ending at the free end or 
  // tow body.

  Nsub.clear();
  iendsect.clear();
  lngth.clear();
  wgtns.clear();
  dians.clear();
  tandns.clear();
  nordns.clear();
  elasns.clear();
  for (int i=0; i < Nsect;i++)
    {
      Nsub.push_back(Sectors[i].Nsub);
      lngth.push_back(Sectors[i].len);
      wgtns.push_back(Sectors[i].wgt);
      dians.push_back(Sectors[i].dia);
      tandns.push_back(Sectors[i].td);
      nordns.push_back(Sectors[i].nd);
      elasns.push_back(Sectors[i].modEla);
    }

  vecUtils.cumsum(lngth, cablel);
  
  //
  // Define the sample increments, midpoints s, endpoints sr and
  // material properties of the cable on the mesh, section by section. 
  //
  vector<double> elas;
  double sstart;
  s.clear();
  sr.clear();
  sr.push_back(0.0);
  dels.clear();
  weight.clear();
  dia.clear();
  tandrag.clear();
  nordrag.clear();
  elas.clear();

  int icm=-1;
  int ice=0;

  for (int nsec=0; nsec<Nsect; nsec++)
    {
      if (nsec==0)
	sstart=0.0;
      else
	sstart=cablel[nsec-1];


      //
      for (int nsu=0; nsu < Nsub[nsec]; nsu++)
	{
	  icm++; 
	  //
	  // Midpoints s
	  //
	  dels.push_back(lngth[nsec]/Nsub[nsec]);
	  s.push_back(sstart+(nsu+1)*dels[icm]/2);
	  //
	  // Define the weight per unit length, diameter, drag coeffieients
	  // and elastic modulus at the midpoints of the segments. They are 
	  // constant within each section.
	  //
	  weight.push_back(wgtns[nsec]);
	  dia.push_back(dians[nsec]);
	  tandrag.push_back(tandns[nsec]);
	  nordrag.push_back(nordns[nsec]);
	  elas.push_back(elasns[nsec]);
	  //
	  // Endpoints sr
	  //
	  ice++;
	  sr.push_back(sstart+(nsu+1)*dels[icm]);
	  //
	  // Save index for end of current section
	  //
	  if (nsu==Nsub[nsec]-1)
	    //	    iendsect[nsec]=ice;
	    iendsect.push_back(ice);
	}
    }
  //
  // Define the number of segments, which is the same as the number
  // of mid points or the last value of icm.
  //
  Nseg=vecUtils.intsum(Nsub);
  //
  // Define the endpoints ss in reverse order for use in ode45.
  //
  
  ss = vecUtils.zeros(Nseg+1,1);
  for (int i=0; i<=Nseg; i++)
    ss[Nseg-i] = sr[i];
  
  // 
  // Compute the mass, virtual mass per unit length and extensibility
  // at the Nseg+1 endpoints.
  //
  mass.clear();;
  mass1.clear();
  ext.clear();

  mass.push_back((weight[0]/g)+(rho*M_PI*pow(dia[0],2))/4);
  mass1.push_back(mass[0]+(rho*M_PI*pow(dia[0],2))/4);
  ext.push_back(1.0/(elas[0]*(M_PI/4)*pow(dia[0],2)));

  for (int n =1; n< Nseg; n++)
    {
      double avgwog=(weight[n]+weight[n-1])/(2*g);
      double avgrca=rho*M_PI*(pow(dia[n],2)+pow(dia[n-1],2))/8;
      mass.push_back(avgwog+avgrca);
      mass1.push_back(mass[n]+avgrca);
      double avgelas=(elas[n]+elas[n-1])/2;
      double avgdias=(pow(dia[n],2)+pow(dia[n-1],2))/2;
      ext.push_back(1.0/(avgelas*(M_PI/4)*avgdias));
    }

  mass.push_back((weight[Nseg-1]/g)+(rho*M_PI*pow(dia[Nseg-1],2))/4);
  mass1.push_back(mass[Nseg]+(rho*M_PI*pow(dia[Nseg-1],2))/4);
  ext.push_back(1.0/(elas[Nseg-1]*(M_PI/4)*pow(dia[Nseg-1],2)));

  /*
  for (int n = 0; n<Nseg ; n++)
    MOOSTrace("n=%d: weight=%f, dia=%f, tandrag=%f, nordrag=%f, elas=%f\n", n+1,weight[n],dia[n],tandrag[n],nordrag[n],elas[n]);
  */
  return(true);
}

bool CArrayModel::steady_state(double towvel, double radheadstart)
{
// function [y,tbmass,vol]=steady_state(towbody,g,rho,wgt,dragcoef,area,...
//                      towvel,Nseg,weight,dia,tandrag,nordrag,cablel, ...
//                   ss,wgtns,dians,tandns,nordns,radheadstart);

// This function finds an approximation to the steady state configuration
// vectory y. The method uses the Matlab ordinary differential equation
// integrator 'ode45' and requires the funtion "dyssdt" that returns the 
// values of the derivatives along the cable. Use iplot=1, below for a
// preview of the steady state configuration.

// First, establish the intial conditions for the steady
// state integration.

  double pi = M_PI;
  double tss1;
  double phiss1;
  
  if (towbody)
    {
      // Assume a cylinder of cross sectional area read from cable3d.dat.


      vol=4*area*sqrt(area/pi)/3;	// THIS WAS CHANGED SO THAT THE TOWED BODY IS A CYLINDER WHOSE SHAPE IS CLOSE TO THE ARRAY
      tbmass=(wgt/g)+rho*vol;
      double drag=.5*rho*area*dragcoef*pow(towvel,2);
      tss1=sqrt(pow(drag,2)+pow(wgt,2));
      double arg=wgt/drag;
      phiss1=atan(arg)+pi;
      double critang=180.0*phiss1/pi;
    }
  else
    {

      // Assume a free end at the end of the last section

      tbmass=0.0;
      vol=0.0;
      double norfac=.5*rho*dia[Nseg-1]*nordrag[Nseg-1]*pow(towvel,2);
      double tanfac=.5*rho*dia[Nseg-1]*pi*tandrag[Nseg-1]*pow(towvel,2);
      //MOOSTrace("norfac,tanfac= %f, %f \n", norfac,tanfac);
      //MOOSTrace("rho,dia,nordrag,tandrag,towvel= %f, %f, %f, %f %f\n",rho,dia[Nseg-1], nordrag[Nseg-1], tandrag[Nseg-1],towvel);
      // Start a short way up the cable from the zero tension
      // end to avoid the singularity in the differential equations.

      double smallss=0.5;

      // Find the critical angle.

      double arg=(weight[Nseg-1]-sqrt(pow(weight[Nseg-1],2)+4.0*pow(norfac,2)))/(2.0*norfac);
      phiss1=acos(arg);
      phiss1=2*pi-phiss1;
      double critang=180.0*phiss1/pi;

      // Estimate the tension a short way up the cable from the zero tension

      tss1=-(weight[Nseg-1]*sin(phiss1) + tanfac*cos(phiss1)*fabs(cos(phiss1)))*smallss;
    }
  //MOOSTrace("tss1, phiss1= %f, %f\n", tss1,phiss1);

  // Define the distance and depth at ss(1)
  
  vector<double> yinit;
  vector<double> yss;
  yss = vecUtils.zeros(4,Nseg+1);
  vector<double> st;

  double xss1=0.0;
  double zss1=0.0;
  
  // Solve the steady state differential equations using Runge-
  // Kutta to find the solution at the Nseg+1 end points of the
  // segments.

  
  // [st,yss]=ode45('dyssdt',ss,[tss1; phiss1; xss1; zss1],...
  //            [],rho,towvel,cablel,wgtns,dians,tandns,nordns);
  yinit.clear();
  yinit.push_back(tss1);
  yinit.push_back(phiss1);
  yinit.push_back(xss1);
  yinit.push_back(zss1);

  cable_ode45(ss,yinit,yss);

  vecUtils.vmov(ss,st);

  //  for (int j=1; j <= ss.size(); j++)
  //  MOOSTrace("yss(:,%d) = %f, %f, %f, %f \n", j, yss[indx(4,1,j)], yss[indx(4,2,j)], yss[indx(4,3,j)], yss[indx(4,4,j)]);

  // yss=yss';
 

// Then intialize the three dimensional cable configuration
// vector with the two dimensional steady state vector.

  yc = vecUtils.zeros(6,Nseg+1);
  for (int j=1; j <= Nseg+1; j++)
    {
      yc[indx(6,1,j)] = yss[indx(4,1,Nseg+2-j)];
      yc[indx(6,6,j)] = yss[indx(4,2,Nseg+2-j)];
      yc[indx(6,5,j)] = radheadstart;
      yc[indx(6,2,j)] = towvel*cos(yc[indx(6,6,j)]);
      yc[indx(6,3,j)] = -towvel*sin(yc[indx(6,6,j)]);
      // MOOSTrace("steady: %d, %f, %f, %f, %f, %f, %f\n",j,yc[indx(6,1,j)],yc[indx(6,2,j)],yc[indx(6,3,j)],yc[indx(6,4,j)],yc[indx(6,5,j)],yc[indx(6,6,j)]);
    }
  return(true);
}

bool CArrayModel::poly_curr_grad_ith_auv(int ith_order )
{
  // [depth,cx,cy,currtim,pcx,pcy,qgx,qgy,dmu]= poly_curr_grad_ith_auv(ith_order );	
  //function [depth,cx,cy,currtim,pcx,pcy,qgx,qgy,dmu]= ...
  //          poly_curr_grad_ith_auv(ith_order );
  
  // This function calls the Matlab function "polyfit" to generate a 
  // least squares ith_order polynomial representation of acoustic doppler
  // current profiler (adcp) measurements. The polynomial repsentation 
  // smooths the data and leads to a ith_order-1 polynomial representation
  // of its gradients, obtained by calling the Matlab function "polyder."
  // These Matlab functions return the polynomial coefficients pcx, pcy,
  // qgx and qgy that may used by the Matlab function "polyval" to obtain
  // the current and its gradient, at various depths, along a tow cable or
  // towed array. Depths must be transformed using pdep=(depth-dmu(1))
  // /dmu(2) before calling "polyval."
  
  // The adcp measurements are stored in the ASCII text file current.dat
  // and read into the array "cur" as follows:
  
  //  double deltim =  50;
  vector<double> depth_it;
  vector<double> cx_it;
  vector<double> cy_it;
  
  // max depth 250 m. 
  int Ncur = 50;
  // depth = [0:Ncur-1]*5;
  //
  depth.clear();
  for (int i=0; i<Ncur; i++)
    depth.push_back(5.0*i);
  
  int iop1=ith_order+1;
  
  //currtim = [ 0:100:10000];  // [2700:100:3600];
  currtim.clear();
  for (int i=0; i<101; i++)
    currtim.push_back(100.0*i);
 
  int Ntim = currtim.size();

  pcx = vecUtils.zeros(Ntim,iop1);
  pcy = vecUtils.zeros(Ntim,iop1);

  qgx = vecUtils.zeros(Ntim,ith_order);
  qgy = vecUtils.zeros(Ntim,ith_order);

  for (int it=0; it < Ntim; it++)
    {
      vecUtils.vmov(depth,depth_it);
      vector<double> cx_it;
      vector<double> cy_it;

      cx_it = vecUtils.zeros(1,Ncur);
      cy_it = vecUtils.zeros(1,Ncur);

      // For now current set to zero

      // ind1 = max(find(depth_it < 150) );
      //  ind1 = min(find(depth_it > 40) );
      
      // cx_it(1:ind1)= .25*ones(1,ind1);;  // cur(2,1:Ncur);
      // cy_it(1:ind1)= .25*ones(1,ind1);  %cur(3,1:Ncur);
      // cy_it(ind1:ind1+3) = 2*[.05/4 2*.05/4 3*.05/4 .05];
      // cy_it(ind1+4:Ncur)= 2*.05*ones(1,Ncur-(ind1+4)+1);   
      
      // cx_it(ind1+1:ind1+10 )= 0;
      // cy_it(ind1+1:ind1+10) = 0;  %1*(10 - [1:10])/10;
      // cx_it(ind1+11:Ncur )= 0;
      // cy_it(ind1+11:Ncur) = 0; 
      
      // ind2 = find( depth_it >= 50 & depth_it < 130);
      // cx_it(ind1+1:ind2)= 0;  // cur(2,1:Ncur);
      // cy_it(ind1+1:ind2)= .0;  %cur(3,1:Ncur);
      // cx_it(ind2+1:Ncur)= 0;  // cur(2,1:Ncur);
      // cy_it(ind2+1:Ncur)= -.0;  %cur(3,1:Ncur);
      
   
      // Find the polynomial coeffecients in the representation of 
      // the current
      // For now set to all zeros
      
      // [pcoef ps pmu]=polyfit(depth_it,cx_it,ith_order);
      // [qcoef qs qmu]=polyfit(depth_it,cy_it,ith_order);
      // pcx(it,1:iop1)=pcoef;
      // pcy(it,1:iop1)=qcoef;
      
      // Find the quadratic polynomial coeffecients in the representation
      // of the gradient
      
      // qgx(it,1:ith_order)=polyder(pcx(it,1:iop1));
      // qgy(it,1:ith_order)=polyder(pcy(it,1:iop1));
      
      // Save the first current profile and depth with the centering
      // and scaling factors dmu. It assumed that the depths the same
      // for all times after it=1.
      
      if (it==0)
	{
	  vecUtils.vmov(depth_it,depth);
	  vecUtils.vmov(cx_it,cx);
	  vecUtils.vmov(cy_it,cy);
	  dmu = vecUtils.zeros(2,1);
	}
    }
  return(true);
}

bool CArrayModel::function_cable(int& nt, bool Array_init, double xtowp, double ytowp, double ztowp, double vx, double vy, double vz)
{

  nt=nt+1;
  // MOOSTrace("function_cable: nt=%d\n",nt);
	
  if (Array_init==true)
	deltim=time-old_time;

  if (dt_hist.size() < dtfilt_length)
    dt_hist.push_back(deltim);
  else
    {
      vecUtils.vshift(dt_hist,1,dt_hist,0,dt_hist.size()-1);
      dt_hist[dt_hist.size()-1] =deltim;
    }
 

  MOOSTrace("time = %f, deltim = %f\n", time, deltim);	

  double speed_towp = sqrt(pow(vx,2)+pow(vy,2)+pow(vz,2));
	
  if (speed_towp == 0.0)		// exit the function if the speed equals 0
    {
      MOOSTrace("The speed of the vehicle was zero!!!\n");
      prob=1;
      return(false);
    }
		
  double towpz=ztowp;	
  double tim=time;
  double tim_curr=time;	
	
  //  !!!! vx and vy have to be reversed when they refer to the model inertial coordinates !!!!	

  double temp = vx;
  vx = vy;	
  vy = temp;

  //********************************************************************************************************
  // 		FIND CONFIG
  //********************************************************************************************************

  // Solve the nonlinear set of algebraic equations for the new cable configuration vector ynew that results from the change in the tow
  // point velocities.


  MOOSTrace("function_cable: vx=%f , vy=%f, vz=%f, heading=%f\n", vx,vy,vz, 180/M_PI*atan2(vy,vx));

  if (over_extrap == 0)
    {
      //	[ynew,InvJacob,Jacobo, count] = find_configf(ycold,yc,nt,vx,vy,vz,Nseg,rho,dia,tandrag,nordrag,weight,dels,...
      //	mass,mass1,ext,deltim,cxc,cyc,gxc,gyc,towbody,area,dragcoef,wgt,tbmass,vol,count, cond_check);
      if (!find_configf(ycold,yc,nt, vx, vy, vz, cxc, cyc, gxc, gyc, mass, mass1, ext, dia, tandrag, nordrag, weight, dels))
	{
	  MOOSTrace("function_cable: Error calling find_configf\n");
	  return(false);
	}
      
    }
  else if (over_extrap == 1)
    {
      //        [ynew,Jacobo] = find_configf_no_InvJacob(ycold,yc,nt,vx,vy,vz,Nseg,rho,dia,tandrag,nordrag,weight,dels,...
      //        mass,mass1,ext,deltim,cxc,cyc,gxc,gyc,towbody,area,dragcoef,wgt,tbmass,vol);
    }


  // Store the new cable configuration vector in yc for subsequent use.
	
  //yc=ynew;
  vecUtils.vmov(ynew,yc);


// Find the cable position (xs,ys,zs) by integrating (xs',ys',zs') in space and adding (xtowp,ytowp,ztowp)'.

  vector<double> xsprime;
  vector<double> xs;

  xsprime.clear(); 

  for (int j=1; j <= Nseg+1; j++)
    xsprime.push_back(cos( yc[indx(6,5,j)] ) * cos( yc[indx(6,6,j)] ));

  vecUtils.cumtrapz(sr,xsprime, xs);
  for (int i=0; i<sr.size(); i++)
    xs[i] += xtowp; 

  vector<double> ysprime;
  vector<double> ys;

  ysprime.clear(); 

  for (int j=1; j <= Nseg+1; j++)
    ysprime.push_back(-sin( yc[indx(6,5,j)] ) * cos( yc[indx(6,6,j)] ));

  vecUtils.cumtrapz(sr,ysprime, ys);
  for (int i=0; i<sr.size(); i++)
    ys[i] += ytowp; 

  vector<double> zsprime;
  vector<double> zs;

  zsprime.clear(); 

  for (int j=1; j <= Nseg+1; j++)
    zsprime.push_back(-sin( yc[indx(6,6,j)] ) );

  vecUtils.cumtrapz(sr,zsprime, zs);
  for (int i=0; i<sr.size(); i++)
    zs[i] += ztowp; 

// xs, ys and zs give the position of the cable in the inertial coordinate 
	
// Find the current and its gradient along the cable, in the inertial coordinates.

// Currents ignored for now

// zsnt=zs(nt,1:Nseg+1);

//if tim >= currtim(1) & tim <= currtim(end)
//	pcxnt= interp1(currtim,pcx,tim_curr,'linear');    //  these are the current coefficients at this time
//	pcynt= interp1(currtim,pcy,tim_curr,'linear');
//	qgxnt= interp1(currtim,qgx,tim_curr,'linear');    //  current gradients at this time
//	qgynt= interp1(currtim,qgy,tim_curr,'linear');
//else
//        tic
//        tim = toc;
//        tim_curr = tim;
//	pcxnt= interp1(currtim,pcx,tim_curr,'linear');    //  these are the current coefficients at this time
//	pcynt= interp1(currtim,pcy,tim_curr,'linear');
//	qgxnt= interp1(currtim,qgx,tim_curr,'linear');    //  current gradients at this time
//	qgynt= interp1(currtim,qgy,tim_curr,'linear');
//	disp('tim outside of range of currtim - pausing - l 39')

// Everwhere polyval is called you need to center and scale the depth array that is passed to polyval, e.g.
	
//zsnt_dmu=(zsnt-dmu(1))/dmu(2);

// Initialize the current and its gradient along the cable
	
// cxc= polyval(pcxnt,zsnt_dmu);                        //  x- current along cable
// cyc= polyval(pcynt,zsnt_dmu);                        //  y- current along cable


// gxc= polyval(qgxnt,zsnt_dmu);
// gyc= polyval(qgynt,zsnt_dmu);

// Currents set to zero
  cxc = vecUtils.zeros(1,Nseg+1);
  cyc = vecUtils.zeros(1,Nseg+1);
  gxc = vecUtils.zeros(1,Nseg+1);
  gyc = vecUtils.zeros(1,Nseg+1);

// SAVE CABLE HEADINGS AND DEPTH FOR THIS TIME STEP

  vector<double> buf_y;
  buf_y.clear();
  for (int i=1; i<=Nseg+1; i++)
    buf_y.push_back(yc[indx(6,5,i)]);

  vector<double> buf_xi;
  buf_xi.clear();
  for (int i=0; i<m_nelm; i++)
    buf_xi.push_back(offset_to_array + compass_offset_full[i]);

  vecUtils.interp(sr,buf_y, buf_xi, array_head);
  for (int i=0; i<m_nelm; i++)
    array_head[i] = 360.0 - array_head[i]*180/M_PI;

  buf_y.clear();
  for (int i=1; i<=Nseg+1; i++)
    buf_y.push_back(yc[indx(6,6,i)]);

  buf_xi.clear();
  for (int i=0; i<m_nelm; i++)
    buf_xi.push_back(offset_to_array + pitch_offset_full[i]);

  vecUtils.interp(sr,buf_y, buf_xi, array_pitch);
  // Sign of pitch changed to be in radians, positive up, consistent with MOOS coordinate system.  HS 111910
  for (int i=0; i<m_nelm; i++)
    array_pitch[i] = M_PI - array_pitch[i];
  //    array_pitch[i] = array_pitch[i]*180/M_PI - 180.0;

  buf_y.clear();
  for (int i=0; i<=Nseg; i++)
    buf_y.push_back(zs[i]);

  buf_xi.clear();
  for (int i=0; i<m_nelm; i++)
    buf_xi.push_back(offset_to_array + depth_offset_full[i]);

  vecUtils.interp(sr,buf_y, buf_xi, array_depth);

  // The folowing does not seem to be used
  // array_tens(nt,:) = interp1 (sr,yc(1,:),[0:10:sr(end)] );

// GET SAVE CABLE SHAPES

// NOTE:  x and y swapped to get to geo-coordinates

  vector<double> xp;
  vector<double> yp;
  vector<double> zp;

  // yp  =  interp1 (sr, xs(nt,1:Nseg+1) + xtowp , offset_to_array + phone_offset , 'linear');

  buf_y.clear();
  for (int i=0; i<=Nseg; i++)
    buf_y.push_back(xs[i]+xtowp);

  buf_xi.clear();
  for (int i=0; i<m_nelm; i++)
    buf_xi.push_back(offset_to_array + phone_offset[i]);

  vecUtils.interp(sr,buf_y, buf_xi, yp);

  // xp  =  interp1 (sr, ys(nt,1:Nseg+1) + ytowp , offset_to_array + phone_offset , 'linear');

  buf_y.clear();
  for (int i=0; i<=Nseg; i++)
    buf_y.push_back(ys[i]+ytowp);

  vecUtils.interp(sr,buf_y, buf_xi, xp);
 
  // for (int i=0; i<m_nelm; i++)
  //  MOOSTrace("xp(%d) = %f\n ", i+1, xp[i]);

  //zp  =  interp1 (sr, new_zs(nt,1:Nseg+1), offset_to_array + phone_offset , 'linear');

  buf_y.clear();
  for (int i=0; i<=Nseg; i++)
    buf_y.push_back(zs[i]);

  vecUtils.interp(sr,buf_y, buf_xi, zp);

  // Save x, y , z positions of array
  // not used
  //xoft(nt,:) = xp;
  //yoft(nt,:) = yp;
  //zoft(nt,:) = zp;
  
  // xsprime= cos(yc(5,1:Nseg+1)).*cos(yc(6,1:Nseg+1));
  // xss1(1:Nseg+1)=cumtrapz(sr,xsprime) ;
  xsprime.clear(); 
  for (int j=1; j <= Nseg+1; j++)
    xsprime.push_back(cos( yc[indx(6,5,j)] ) * cos( yc[indx(6,6,j)] ));

  vecUtils.cumtrapz(sr,xsprime, xss1);

  // ysprime=-sin(yc(5,1:Nseg+1)).*cos(yc(6,1:Nseg+1));
  // yss1(1:Nseg+1)=cumtrapz(sr,ysprime) ;
  ysprime.clear(); 
  for (int j=1; j <= Nseg+1; j++)
    ysprime.push_back(-sin( yc[indx(6,5,j)] ) * cos( yc[indx(6,6,j)] ));

  vecUtils.cumtrapz(sr,ysprime, yss1);

  // zsprime=-sin(yc(6,1:Nseg+1));
  // zss1(1:Nseg+1)=cumtrapz(sr,zsprime);
  zsprime.clear(); 
  for (int j=1; j <= Nseg+1; j++)
    zsprime.push_back(-sin( yc[indx(6,6,j)] ) );

  vecUtils.cumtrapz(sr,zsprime, zss1);

  //  vector<double> zneg;
  //  vecUtils.vneg(zss1,zneg);

  array_tension=yc[indx(6,1,1)];

  // Hydrophone positions for MOOS variables HYDROPHONE_X etc.

  buf_xi.clear();
  for (int i=0; i<m_nelm; i++)
   buf_xi.push_back(offset_to_array + phone_offset[i]);

  // add topw point offset
  for (int i=0; i<xss1.size(); i++)
    {
      xss1[i] += xtowp;
      yss1[i] += ytowp;
      zss1[i] -= ztowp;
    }

  vecUtils.interp(sr,yss1, buf_xi, buf_y);
  vecUtils.dbl_vec2str(buf_y, ac_array_x);
  //MOOSTrace("HYDROPHONE_X = %s\n",ac_array_x.c_str());
  ael_x = vecUtils.mean(buf_y);
  ael_x_offset = ael_x - ytowp;
  MOOSTrace("ael = %f, ael_y_offset = %f\n",ael_y,ael_y_offset);

  vecUtils.interp(sr,xss1, buf_xi, buf_y);
  vecUtils.dbl_vec2str(buf_y, ac_array_y);
  ael_y = vecUtils.mean(buf_y);
  ael_y_offset = ael_y - xtowp;
  MOOSTrace("ael = %f, ael_y_offset = %f\n",ael_y,ael_y_offset);


  //  vecUtils.interp(sr,zneg, buf_xi, buf_y);
  vecUtils.interp(sr,zss1, buf_xi, buf_y);
  vecUtils.dbl_vec2str(buf_y, ac_array_z);
  //MOOSTrace("HYDROPHONE_Z = %s\n",ac_array_z.c_str());

  // cable positions. MOOS variables CABLE_X etc.

  vecUtils.dbl_vec2str(xss1, y_array);

  vecUtils.dbl_vec2str(yss1, x_array);

  //  vecUtils.dbl_vec2str(zneg, z_array);
  vecUtils.dbl_vec2str(zss1, z_array);
  //MOOSTrace("CABLE_Z = %s\n",z_array.c_str());

  // pitch and heading

  vecUtils.dbl_vec2str(array_head, arr_heading);
  //MOOSTrace("ARRAY_HEADING = %s\n",arr_heading.c_str());
  vecUtils.dbl_vec2str(array_pitch, arr_pitch);
  //MOOSTrace("ARRAY_PITCH = %s\n",arr_pitch.c_str());

// SAVE AND EXTRAPOLATE TOW CABLE SOL

// ycold=yc;
//  vecUtils.vmov(yc,ycold);


  if (over_extrap == 0)
    {
      if ( nt==1 )		
	vecUtils.vmov(yc,ycold);
      // ycold=yc;	// Save cable configuration vector in ycold for next time step
      else
	{
	  // Extrapolate the cable configuration to the next time step. Save cable configuration vector in ycold for next time step.
	  // Note that yc, ycold are already stored columnwize in a vector, FORTRAN style
	  
	  //	bigy=yc(:);
	  //	bigyold=ycold(:);
	  //    bigynew=bigy-InvJacob*Jacobo*(bigy-bigyold); 
	  double dt_mean = vecUtils.mean(dt_hist);
	  vector<double> bigynew;
	  vecUtils.vsub(yc,ycold,buf_y);
	  vecUtils.mmul(Jacobo,yc.size(),yc.size(),buf_y,1,bigynew);
	  vecUtils.mmul(InvJacob,yc.size(),yc.size(),bigynew,1,buf_y);
	  vecUtils.vsmul(buf_y,dt_mean/deltim,buf_y);
	  vecUtils.vsub(yc,buf_y,bigynew);
	  
	  
	  vecUtils.vmov(yc,ycold);
	  vecUtils.vmov(bigynew,yc);
	  
	  //	  vecUtils.vmov(ynew,old_ynew);
	  // vecUtils.vmov(yc,old_bigy);
	  // vecUtils.vmov(ycold,old_bigyold);
	  // vecUtils.vmov(bigynew,old_bigynew);
	  // vecUtils.vmov(InvJacob,old_Jacobo);
	}
    }

  // clear ynew bigy bigyold bigynew InvJacob Jacobo;

  prob=0;


  old_time=time;			// this variable evaluates the time between two steps

  return(true);
}

bool CArrayModel::dyssdt(double sd, vector<double> yss, vector<double>& dydt )
{
  // This function finds the spatial derivatives that appear in the
  // differential equations for the steady state cable configuration
  // on pg. 516 of Milinazzo, et. al., Ocean Engng., Vol. 14, No. 6,
  // 513-526 (1987). The water desity is rho=1,000 kg/m^3 and towvel
  // is the tow velocity in m/s.
  
  // Find the index nsd of the first point greater than or equal to
  // sd in the array cablel of cable section end points measured
  // from the tow point.
  
  double pi = M_PI;
  int nsd;

  int nlt = -1;
  for ( int i=0; i<cablel.size() && nlt<0; i++)
    {
      if (sd <= cablel[i])
	nlt = i;
    }

  if(nlt>=0)
    nsd=nlt;
  else
    nsd=cablel.size()-1;

  
  // The spatial derivative of the tension
  dydt.clear();

  dydt.push_back(wgtns[nsd]*sin(yss[1])+.5*rho*dians[nsd]*pi*tandns[nsd]*towvel*cos(yss[1])*fabs(towvel*cos(yss[1])));
  
  // The spatial derivative of the elevation angle
  
  dydt.push_back((wgtns[nsd]*cos(yss[1])-.5*rho*dians[nsd]*nordns[nsd]*towvel*sin(yss[1])*fabs(towvel*sin(yss[1])))/yss[1] );
  
  // The spatial derivatives of the x and z positions
  
  dydt.push_back(cos(yss[1]));
  dydt.push_back(-sin(yss[1]));
  
}

bool CArrayModel::cable_ode45(vector<double> x, vector<double> y_init, vector<double>& y)
{
  vector<double> k1;
  vector<double> k2;
  vector<double> k3;
  vector<double> k4;
  vector<double> buf;
  vector<double> yold;
  vector<double> ybuf;

  y.clear();
  vecUtils.vmov(y_init,y);
  vecUtils.vmov(y_init,yold);

  for (int i=1; i<x.size(); i++)
    {
      double h = x[i]-x[i-1];
      dyssdt(x[i-1],yold,buf);
      vecUtils.vsmul(buf, h, k1);
      
      vecUtils.vsmul(k1, 0.5, buf);
      vecUtils.vadd(yold,buf,ybuf);
      dyssdt(x[i-1]+h*0.5,ybuf,buf);
      vecUtils.vsmul(buf, h, k2);

      vecUtils.vsmul(k2, 0.5, buf);
      vecUtils.vadd(yold,buf,ybuf);
      dyssdt(x[i-1]+h*0.5,ybuf,buf);
      vecUtils.vsmul(buf, h, k3);

      vecUtils.vadd(yold, k3, ybuf);
      dyssdt(x[i-1]+h,ybuf,buf);
      vecUtils.vsmul(buf, h, k4);

      vecUtils.vadd(k2, k3, ybuf);
      vecUtils.vsmul(ybuf, 2.0, ybuf);
      vecUtils.vadd(k1, ybuf, ybuf);
      vecUtils.vadd(k4, ybuf, ybuf);
      vecUtils.vsmul(ybuf, 0.16666667 , ybuf);
      vecUtils.vadd(yold, ybuf, ybuf);
      
      for (int j=0; j<ybuf.size(); j++)
	y.push_back(ybuf[j]);
      
      vecUtils.vmov(ybuf,yold);
    }
  return(true);  
}

bool CArrayModel::find_configf(vector<double> yold, vector<double> y, int nt, double vx, double vy, double vz, vector<double> cx, vector<double> cy, vector<double> gx, vector<double> gy, vector<double> mass, vector<double> mass1, vector<double> ext, vector<double> dia, vector<double> tandrag, vector<double> nordrag, vector<double> weight, vector<double> dels)
{
  //function [ynew,InvJacob,Jacobo,count]=find_configf(yold,y,nt,vx,vy,vz,...
  //    Nseg,rho,dia,tandrag,nordrag,weight,dels,...
  //    mass,mass1,ext,deltim,cx,cy,gx,gy,...
  //    towbody,area,dragcoef,wgt,tbmass,vol,count, cond_check);
  
  
  // This function applies the Newton-Raphson iteration to solve the
  // set of nonlinear algebraic equations found in Milinazzo, Wilkie
  // and Latchman, Ocean Engng., Vol. 14, No. 6, 513-526 (1987). The
  // solution of these equations yields the new cable configuration
  // vector ynew that results from a change in the tow point velocities.
  // An initial estimate for ynew is assumed to be stored in y. This
  // version includes the current (cx,cy) and its gradient (gx,gy)
  // in inertial coordinates.
  
  // This version is designed for Matlab Version 6.5, Just In Time
  // processing and does all caclulations in line. (R. B. Evans,
  // SAIC, Mystic, CT, 11/5/02)
  
  // Start the Newton-Raphson iteration.
  
  int icmax=50;
  double ftol=10;
  int icount=0;
  double pi = M_PI;
  int bw=8;
  int dimJacob = 6*(Nseg+1);

  vector<double> yavg;
  vector<double> cable_resvect;
  vector<double> resvect;
  
  bool debug = false;

  //  bool debug = nt <=3 || nt%20 == 0;
  
  // re-index cable arrays cx,cy,gx and gy etc for consistency with matlab original.
  
  cx.push_back(0.0);
  vecUtils.vshift(cx,0,cx,1,cx.size()-1);
  cy.push_back(0.0);
  vecUtils.vshift(cy,0,cy,1,cy.size()-1);
  gx.push_back(0.0);
  vecUtils.vshift(gx,0,gx,1,gx.size()-1);
  gy.push_back(0.0);
  vecUtils.vshift(gy,0,gy,1,gy.size()-1);
  mass.push_back(0.0);
  vecUtils.vshift(mass,0,mass,1,mass.size()-1);
  mass1.push_back(0.0);
  vecUtils.vshift(mass1,0,mass1,1,mass1.size()-1);
  ext.push_back(0.0);
  vecUtils.vshift(ext,0,ext,1,ext.size()-1);
  dia.push_back(0.0);
  vecUtils.vshift(dia,0,dia,1,dia.size()-1);
  tandrag.push_back(0.0);
  vecUtils.vshift(tandrag,0,tandrag,1,tandrag.size()-1);
  nordrag.push_back(0.0);
  vecUtils.vshift(nordrag,0,nordrag,1,nordrag.size()-1);
  weight.push_back(0.0);
  vecUtils.vshift(weight,0,weight,1,weight.size()-1);
  dels.push_back(0.0);
  vecUtils.vshift(dels,0,dels,1,dels.size()-1);
  

  MOOSTrace("find_config: vx=%f , vy=%f, vz=%f, heading=%f\n", vx,vy,vz, 180/M_PI*atan2(vy,vx));

      if (debug)   
       {
	 char filenm[100];
	 sprintf(filenm,"seg_vars_%d.m",nt);
	 FILE *fp = fopen(filenm, "w");
	 int i;	 

	 fprintf(fp,"vx = %.8e ; vy = %.8e; vz = %.8e\n", vx,vy,vz);

 	 for (i=1; i<=Nseg; i++)
	   {
		 fprintf(fp,"mass(%d) = %.8e ; \n ",i,mass[i]);
		 fprintf(fp,"mass1(%d) = %.8e ; \n ",i,mass1[i]);
		 fprintf(fp,"ext(%d) = %.8e ; \n ",i,ext[i]);
		 fprintf(fp,"tandrag(%d) = %.8e ; \n ",i,tandrag[i]);
		 fprintf(fp,"nordrag(%d) = %.8e ; \n ",i,nordrag[i]);
		 fprintf(fp,"weight(%d) = %.8e ; \n ",i,weight[i]);
		 fprintf(fp,"dia(%d) = %.8e ; \n ",i,dia[i]);
		 fprintf(fp,"dels(%d) = %.8e ; \n ",i,dels[i]);
	   } 
	 i=Nseg+1;
	 fprintf(fp,"mass(%d) = %.8e ; \n ",i,mass[i]);
	 fprintf(fp,"mass1(%d) = %.8e ; \n ",i,mass1[i]);
	 fprintf(fp,"ext(%d) = %.8e ; \n ",i,ext[i]);

	 fclose(fp);
	 }
     

  // Define racxx=rho*area*cx and racyy=rho*area*cy
  
  // racxx=(mass1-mass).*cx;
  // racyy=(mass1-mass).*cy;
  vector<double> racxx;
  vector<double> racyy;
  vecUtils.vsub(mass1,mass,racyy);
  vecUtils.vmul(racyy,cx,racxx);
  vecUtils.vmul(racyy,cy,racyy);
  
  vector<double> dely;
  vector<double> bigy;

  FILE *fpq;
  FILE *fpM;
  FILE *fpN;
  FILE *fpR;
  FILE *fpres;
  FILE *fpJ;
  FILE *fpyc;
  if (debug)
    { 
      char filenm[100];
      sprintf(filenm,"dq_%d.m",nt);
      fpq = fopen(filenm, "w");
      sprintf(filenm,"DM_M_%d.m",nt);
      fpM = fopen(filenm, "w");
      sprintf(filenm,"DN_N_%d.m",nt);
      fpN = fopen(filenm, "w");
      sprintf(filenm,"rmat_%d.m",nt);
      fpR = fopen(filenm, "w");
      sprintf(filenm,"res_%d.m",nt);
      fpres = fopen(filenm, "w");
      sprintf(filenm,"jac_%d.m",nt);
      fpJ = fopen(filenm, "w");
      sprintf(filenm,"yc_%d.m",nt);
      fpyc = fopen(filenm, "w");
    }


  while (ftol >= 1e-3 && icount<icmax )
    {  
      icount=icount+1;
      
      // Evaluate the residual at the tow point
      
      vector<double> residual;
      residual.clear();
      double tp1= y[indx(6,2,1)] - cos(y[indx(6,5,1)])*cos(y[indx(6,6,1)])*vx + sin(y[indx(6,5,1)])*cos(y[indx(6,6,1)])*vy + sin(y[indx(6,6,1)])*vz;
      double tp2= y[indx(6,3,1)] + cos(y[indx(6,5,1)])*sin(y[indx(6,6,1)])*vx - sin(y[indx(6,5,1)])*sin(y[indx(6,6,1)])*vy + cos(y[indx(6,6,1)])*vz;
      double tp3= y[indx(6,4,1)] - sin(y[indx(6,5,1)])*vx - cos(y[indx(6,5,1)])*vy;
      //resvect.push_back(0.0);
      resvect.clear();
      resvect.push_back(tp1);
      resvect.push_back(tp2);
      resvect.push_back(tp3);
      double towpoint_residual = pow(tp1,2)+pow(tp2,2)+pow(tp3,2);
      double Max_residual;

      // Evaluate the residuals all along the cable
      
      for (int n=1; n<=Nseg; n++)
	{
	  if (nt==1)
	    {
	      yavg.clear();
	      yavg.push_back(0.0);
	      for (int j=1; j<= 6; j++)
		{
		  yavg.push_back((y[indx(6,j,n+1)]+y[indx(6,j,n)])/2);
		  //		  MOOSTrace("nt=1, icount= %d, y(%d,%d) = %f, y(%d,%d) = %f\n", icount, j,n, y[indx(6,j,n)],j,n+1,y[indx(6,j,n+1)]);
		}

	      //   M=M6x6(yavg);
	      
	      vector<double> M;
	      M = vecUtils.zeros(6,6);
	      
	      M[indx(6,1,1)]= 1.0;
	      M[indx(6,2,2)]= 1.0;
	      M[indx(6,3,3)]= 1.0;
	      M[indx(6,4,4)]= 1.0;
	      
	      M[indx(6,2,5)]= yavg[4]*cos(yavg[6]);
	      M[indx(6,3,5)]=-yavg[4]*sin(yavg[6]);
	      M[indx(6,4,5)]= yavg[3]*sin(yavg[6])-yavg[2]*cos(yavg[6]);
	      M[indx(6,5,5)]=-yavg[1]*cos(yavg[6]);
	      
	      M[indx(6,2,6)]=-yavg[3];
	      M[indx(6,3,6)]= yavg[2];
	      M[indx(6,6,6)]= yavg[1];
	      
	      double cxavg=(cx[n+1]+cx[n])/2;
	      double cyavg=(cy[n+1]+cy[n])/2;
	      double gxavg=(gx[n+1]+gx[n])/2;
	      double gyavg=(gy[n+1]+gy[n])/2;
	      //MOOSTrace("cxavg= %f, cyavg= %f, gxavg= %f, gyavg= %f \n", cxavg, cyavg,gxavg,gyavg);
	      
	      //   q=q6x1c(yavg,rho,dia[n],tandrag[n],nordrag[n],weight[n],...
	      //            ext[n],cxavg,cyavg,gxavg,gyavg);
	      
	      double rext=sqrt(1.0+ext[n]*yavg[1]);
	      double cs_area=(pi/4)*pow(dia[n],2);
	      
	      double y2mct = yavg[2]-(cos(yavg[5])*cxavg-sin(yavg[5])*cyavg)*cos(yavg[6]);
	      double q1 = weight[n]*sin(yavg[6])+.5*rho*dia[n]*rext*pi*tandrag[n]*y2mct*fabs(y2mct);
	      
	      double y3mcn=yavg[3]+(cos(yavg[5])*cxavg-sin(yavg[5])*cyavg)*sin(yavg[6]);
	      double y4mcb=yavg[4]-(sin(yavg[5])*cxavg+cos(yavg[5])*cyavg);
	      double root=sqrt(pow(y3mcn,2)+pow(y4mcb,2));
	      double wz=-(sin(yavg[6])*yavg[2]+cos(yavg[6])*yavg[3]);
	      // MOOSTrace("dia[n] = %f, rext = %f, nordrag[n] = %f, y4mcb = %f, root = %f  \n",dia[n],rext,nordrag[n],y4mcb,root);
	      // MOOSTrace("cs_area = %f, wz = %f, yavg[5] = %f  \n",cs_area,wz,yavg[5]);

	      double q5= .5*rho*dia[n]*rext*nordrag[n]*y4mcb*root-rho*cs_area*wz*(sin(yavg[5])*gxavg+cos(yavg[5])*gyavg);
	      
	      double q6= weight[n]*cos(yavg[6])+.5*rho*dia[n]*rext*nordrag[n]*y3mcn*root-rho*cs_area*wz*sin(yavg[6])*(-cos(yavg[5])*gxavg+sin(yavg[5])*gyavg);
	      
	      vector<double> q;
	      q = vecUtils.zeros(1,6);
	      q[0]=q1;
	      q[4]=q5;
	      q[5]=q6;
	      // MOOSTrace("q1 = %f, q5 = %f, q6 = %f\n", q1, q5, q6);
	      
	      vector<double> ydif;
	      ydif.clear();             
	      for (int j = 1; j<=6; j++)
		  ydif.push_back((y[indx(6,j,n+1)]-y[indx(6,j,n)])/dels[n]);
	      vecUtils.mmul(M, 6, 6, ydif, 1, cable_resvect);
	      vecUtils.vsub(cable_resvect,q,cable_resvect);
	      //           cable_resvect=M*ydif-q;
	      for (int j=0; j<6; j++)
		{
		  //		  MOOSTrace("nt=1, icount = %d, n= %d: yavg[%d] = %f\n", icount, n, j, yavg[j]);
		  // MOOSTrace("nt=1, icount = %d, n= %d: ydif[%d] = %f\n", icount, n, j, ydif[j]);
		  // MOOSTrace("cable_resvect[%d] = %f\n", cable_resvect[j]);
		}	    
	    }
	  else
	    {
	      vector<double> Mn;
	      vector<double> ynsavg;

	      ynsavg.clear();
	      ynsavg.push_back(0.0);
	      for (int j=1; j<= 6; j++)
		ynsavg.push_back((y[indx(6,j,n+1)]+y[indx(6,j,n)])/2);
	      
	      //            ynsavg=(y[indx(6,:,n+1)+y[indx(6,:,n)])/2;
	      
	      vector<double> ynsdif;
	      ynsdif.clear();       
	      for (int j = 1; j<=6; j++)
		{
		  double gradn = (y[indx(6,j,n+1)] - y[indx(6,j,n)])/dels[n];
		  ynsdif.push_back(gradn);
		}
	    
	      // ynsdif=(y[indx(6,:,n+1)-y[indx(6,:,n)])/dels[n];
	      
	      //    Mn=M6x6(ynsavg);
	      
	      Mn = vecUtils.zeros(6,6);
	      
	      Mn[indx(6,1,1)]= 1.0;
	      Mn[indx(6,2,2)]= 1.0;
	      Mn[indx(6,3,3)]= 1.0;
	      Mn[indx(6,4,4)]= 1.0;
	      
	      Mn[indx(6,2,5)]= ynsavg[4]*cos(ynsavg[6]);
	      Mn[indx(6,3,5)]=-ynsavg[4]*sin(ynsavg[6]);
	      Mn[indx(6,4,5)]= ynsavg[3]*sin(ynsavg[6])-ynsavg[2]*cos(ynsavg[6]);
	      Mn[indx(6,5,5)]=-ynsavg[1]*cos(ynsavg[6]);
	     
	      Mn[indx(6,2,6)]=-ynsavg[3];
	      Mn[indx(6,3,6)]= ynsavg[2];
	      Mn[indx(6,6,6)]= ynsavg[1];
	      
	      
	      vector<double> Mo;
	      vector<double> yosavg;
	      
	      yosavg.clear();
	      yosavg.push_back(0.0);
	      for (int j=1; j<= 6; j++)
		yosavg.push_back((yold[indx(6,j,n+1)]+yold[indx(6,j,n)])/2);
	      
	      vector<double> yosdif;
	      yosdif.clear();      
	      for (int j = 1; j<=6; j++)
		{
		  double grado = (yold[indx(6,j,n+1)] - yold[indx(6,j,n)])/dels[n];
		  yosdif.push_back(grado);
		}
	      
	      //            yosavg=(yold(:,n+1)+yold(:,n))/2;
	      
	      //    Mo=M6x6(yosavg);
	      
	      Mo = vecUtils.zeros(6,6);
	      
	      Mo[indx(6,1,1)]= 1.0;
	      Mo[indx(6,2,2)]= 1.0;
	      Mo[indx(6,3,3)]= 1.0;
	      Mo[indx(6,4,4)]= 1.0;
	      
	      Mo[indx(6,2,5)]= yosavg[4]*cos(yosavg[6]);
	      Mo[indx(6,3,5)]=-yosavg[4]*sin(yosavg[6]);
	      Mo[indx(6,4,5)]= yosavg[3]*sin(yosavg[6])-yosavg[2]*cos(yosavg[6]);
	      Mo[indx(6,5,5)]=-yosavg[1]*cos(yosavg[6]);
	      
	      Mo[indx(6,2,6)]=-yosavg[3];
	      Mo[indx(6,3,6)]= yosavg[2];
	      Mo[indx(6,6,6)]= yosavg[1];
	      
	      double cxavg=(cx[n+1]+cx[n])/2;
	      double cyavg=(cy[n+1]+cy[n])/2;
	      double gxavg=(gx[n+1]+gx[n])/2;
	      double gyavg=(gy[n+1]+gy[n])/2;
	      
	      //    qn=q6x1c(ynsavg,rho,dia[n],tandrag[n],nordrag[n],weight[n],...
	      //             ext[n],cxavg,cyavg,gxavg,gyavg);
	      
	      double rext=sqrt(1.0+ext[n]*ynsavg[1]);
	      double cs_area=(pi/4)*pow(dia[n],2);
	      
	      double y2mct=ynsavg[2]-(cos(ynsavg[5])*cxavg-sin(ynsavg[5])*cyavg)*cos(ynsavg[6]);
	      double q1= weight[n]*sin(ynsavg[6])+.5*rho*dia[n]*rext*pi*tandrag[n]*y2mct*fabs(y2mct);
	      
	      double y3mcn=ynsavg[3]+(cos(ynsavg[5])*cxavg-sin(ynsavg[5])*cyavg)*sin(ynsavg[6]);
	      double y4mcb=ynsavg[4]-(sin(ynsavg[5])*cxavg+cos(ynsavg[5])*cyavg);
	      double root=sqrt(pow(y3mcn,2)+pow(y4mcb,2));
	      double wz=-(sin(ynsavg[6])*ynsavg[2]+cos(ynsavg[6])*ynsavg[3]);
	      
	      double q5 = .5*rho*dia[n]*rext*nordrag[n]*y4mcb*root-rho*cs_area*wz*(sin(ynsavg[5])*gxavg+cos(ynsavg[5])*gyavg);
	      
	      double q6 = weight[n]*cos(ynsavg[6])+.5*rho*dia[n]*rext*nordrag[n]*y3mcn*root-rho*cs_area*wz*sin(ynsavg[6])*(-cos(ynsavg[5])*gxavg+sin(ynsavg[5])*gyavg);
	      
	      vector<double> qn;
	      qn = vecUtils.zeros(1,6);
	      qn[0]=q1;
	      qn[4]=q5;
	      qn[5]=q6;
	      
	      // qn=[q1; 0.0; 0.0; 0.0; q5; q6];
	      
	      //    qo=q6x1c(yosavg,rho,dia[n],tandrag[n],nordrag[n],weight[n],...
	      //             ext[n],cxavg,cyavg,gxavg,gyavg);
	      
	      rext=sqrt(1.0+ext[n]*yosavg[1]);
	      cs_area=(pi/4)*pow(dia[n],2);
	      
	      y2mct=yosavg[2]-(cos(yosavg[5])*cxavg-sin(yosavg[5])*cyavg)*cos(yosavg[6]);
	      q1= weight[n]*sin(yosavg[6])+.5*rho*dia[n]*rext*pi*tandrag[n]*y2mct*fabs(y2mct);
	      
	      y3mcn=yosavg[3]+(cos(yosavg[5])*cxavg-sin(yosavg[5])*cyavg)*sin(yosavg[6]);
	      y4mcb=yosavg[4]-(sin(yosavg[5])*cxavg+cos(yosavg[5])*cyavg);
	      root=sqrt(pow(y3mcn,2)+pow(y4mcb,2));
	      wz=-(sin(yosavg[6])*yosavg[2]+cos(yosavg[6])*yosavg[3]);
	      
	      q5= .5*rho*dia[n]*rext*nordrag[n]*y4mcb*root-rho*cs_area*wz*(sin(yosavg[5])*gxavg+cos(yosavg[5])*gyavg);
	      
	      q6= weight[n]*cos(yosavg[6])+.5*rho*dia[n]*rext*nordrag[n]*y3mcn*root-rho*cs_area*wz*sin(yosavg[6])*(-cos(yosavg[5])*gxavg+sin(yosavg[5])*gyavg);
	      
	      
	      vector<double> qo;
	      qo = vecUtils.zeros(1,6);
	      qo[0]=q1;
	      qo[4]=q5;
	      qo[5]=q6;
	      
	      //            qo=[q1; 0.0; 0.0; 0.0; q5; q6];
	      
	      // yosdif=(yold(:,n+1)-yold(:,n))/dels[n];
	      // cable_resvect=.5*(Mn*ynsdif+Mo*yosdif-qn-qo);
	      vector<double> bufn;
	      vector<double> bufo;

	      vecUtils.mmul(Mn, 6, 6, ynsdif, 1, cable_resvect);
	      vecUtils.vsub(cable_resvect,qn,bufn);
	      vecUtils.mmul(Mo, 6, 6, yosdif, 1, cable_resvect);
	      vecUtils.vsub(cable_resvect,qo,bufo);
	      vecUtils.vadd(bufn,bufo,cable_resvect);
	      vecUtils.vsmul(cable_resvect,0.5,cable_resvect);
	      
	       if (debug )  
		 {
		   for (int j = 1; j <= 6; j++)
		     {
		       fprintf(fpR,"icnt=%d; n=%d  ; yold(%d,%d) = %.8e \n ",icount,n,j,n,yold[indx(6,j,n)]);
		       fprintf(fpR,"icnt=%d; n=%d  ; yold(%d,%d) = %.8e \n ",icount,n,j,n+1,yold[indx(6,j,n+1)]);
		       fprintf(fpR,"icnt=%d; n=%d  ; yosavg(%d) = %.8e  \n ",icount,n,j,yosavg[j]);
		       fprintf(fpR,"icnt=%d; n=%d  ; yosdif(%d) = %.8e  \n ",icount,n,j,yosdif[j-1]);
		       fprintf(fpR,"icnt=%d; n=%d  ; qo(%d) = %.8e  \n ",icount,n,j,qo[j-1]);
		       fprintf(fpR,"icnt=%d; n=%d  ; ynsavg(%d) = %.8e  \n ",icount,n,j,ynsavg[j]);
		       fprintf(fpR,"icnt=%d; n=%d  ; ynsdif(%d) = %.8e  \n ",icount,n,j,ynsdif[j-1]);
		       fprintf(fpR,"icnt=%d; n=%d  ; qn(%d) = %.8e  \n ",icount,n,j,qn[j-1]);
		       for (int i = 1; i <= 6; i++)
			 {
			   fprintf(fpR,"icnt=%d; n=%d; Mo(%d,%d) = %.8e ; \n ",icount,n,i,j,Mo[indx(6,i,j)]);
			   fprintf(fpR,"icnt=%d; n=%d; Mn(%d,%d) = %.8e ; \n ",icount,n,i,j,Mn[indx(6,i,j)]);
			 }
		     }
		 }
	       
     
	      //            ytavg=(y[indx(6,:,n)+yold(:,n))/2;
	      vector<double> ytavg;
	      
	      ytavg.clear();
	      ytavg.push_back(0.0);
	      for (int j=1; j<= 6; j++)
		ytavg.push_back((y[indx(6,j,n)]+yold[indx(6,j,n)])/2);
	      
	      //    N=N6x6c(ytavg,mass[n],mass1[n],ext[n],racxx[n],racyy[n]);
	      
	      vector<double> N;
	      N = vecUtils.zeros(6,6);
	      double opet=1.0+ext[n]*ytavg[1];
	      
	      N[indx(6,1,1)]= -ext[n]*mass[n]*ytavg[2]/opet;
	      N[indx(6,2,1)]=  ext[n];
	      N[indx(6,5,1)]= -ext[n]*mass1[n]*ytavg[4]/opet;
	      N[indx(6,6,1)]= -ext[n]*mass1[n]*ytavg[3]/opet;
	      
	      N[indx(6,1,2)]= mass[n];
	      N[indx(6,6,3)]= mass1[n];
	      N[indx(6,5,4)]= mass1[n];
	      
	      double ract= (cos(ytavg[5])*racxx[n]-sin(ytavg[5])*racyy[n])*cos(ytavg[6]);
	      double racn=-(cos(ytavg[5])*racxx[n]-sin(ytavg[5])*racyy[n])*sin(ytavg[6]);
	      double racb= (sin(ytavg[5])*racxx[n]+cos(ytavg[5])*racyy[n]);
	      
	      N[indx(6,1,5)]= (mass1[n]*ytavg[4]-racb)*cos(ytavg[6]);
	      N[indx(6,4,5)]=-opet*cos(ytavg[6]);
	      N[indx(6,5,5)]= mass1[n]*ytavg[3]*sin(ytavg[6])-(mass[n]*ytavg[2]+ract)*cos(ytavg[6]);
	      N[indx(6,6,5)]=-mass1[n]*ytavg[4]*sin(ytavg[6]);
	      
	      N[indx(6,1,6)]=-(mass1[n]*ytavg[3]-racn);
	      N[indx(6,3,6)]= opet;
	      N[indx(6,6,6)]= mass[n]*ytavg[2]+ract;
	      
	      //            ytdif=(y[indx(6,:,n)-yold(:,n))/deltim;
	      
	      vector<double> ytdif;
	      ytdif.clear();             
	      for (int j = 1; j<=6; j++)
		ytdif.push_back((y[indx(6,j,n)]-yold[indx(6,j,n)])/deltim);

	      // cable_resvect=cable_resvect-.5*N*ytdif;
	      
	       if (debug )  
		 {
		   for (int j = 1; j <= 6; j++)
		     {
		       fprintf(fpR,"icnt=%d; n=%d  ; ytavg(%d) = %.8e  \n ",icount,n,j,ytavg[j]);
		       fprintf(fpR,"icnt=%d; n=%d  ; ytdif(%d) = %.8e  \n ",icount,n,j,ytdif[j-1]);

		       for (int i = 1; i <= 6; i++)
			 {
			   fprintf(fpR,"icnt=%d; n=%d; N(%d,%d) = %.8e  \n ",icount,n,i,j,N[indx(6,i,j)]);
			 }
		     }
		 }


	      vecUtils.mmul(N, 6, 6, ytdif, 1, ytdif);
	      vecUtils.vsmul(ytdif,0.5,ytdif);
	      
	      vecUtils.vsub(cable_resvect,ytdif,cable_resvect);
	      
	      //ytavg=(y[indx(6,:,n+1)+yold(:,n+1))/2;
	      ytavg.clear();
	      ytavg.push_back(0.0);
	      for (int j=1; j<= 6; j++)
		ytavg.push_back((y[indx(6,j,n+1)]+yold[indx(6,j,n+1)])/2);
	      
	      
	      //    N=N6x6c(ytavg,mass[n+1],mass1[n+1],ext[n+1],racxx[n+1],racyy[n+1]);
	      
	      N = vecUtils.zeros(6,6);
	      opet=1.0+ext[n+1]*ytavg[1];
	      
	      N[indx(6,1,1)]= -ext[n+1]*mass[n+1]*ytavg[2]/opet;
	      N[indx(6,2,1)]=  ext[n+1];
	      N[indx(6,5,1)]= -ext[n+1]*mass1[n+1]*ytavg[4]/opet;
	      N[indx(6,6,1)]= -ext[n+1]*mass1[n+1]*ytavg[3]/opet;
	      
	      N[indx(6,1,2)]= mass[n+1];
	      N[indx(6,6,3)]= mass1[n+1];
	      N[indx(6,5,4)]= mass1[n+1];
	      
	      ract= (cos(ytavg[5])*racxx[n+1]-sin(ytavg[5])*racyy[n+1])*cos(ytavg[6]);
	      racn=-(cos(ytavg[5])*racxx[n+1]-sin(ytavg[5])*racyy[n+1])*sin(ytavg[6]);
	      racb= (sin(ytavg[5])*racxx[n+1]+cos(ytavg[5])*racyy[n+1]);
	      
	      N[indx(6,1,5)]= (mass1[n+1]*ytavg[4]-racb)*cos(ytavg[6]);
	      N[indx(6,4,5)]=-opet*cos(ytavg[6]);
	      N[indx(6,5,5)]= mass1[n+1]*ytavg[3]*sin(ytavg[6])-(mass[n+1]*ytavg[2]+ract)*cos(ytavg[6]);
	      N[indx(6,6,5)]=-mass1[n+1]*ytavg[4]*sin(ytavg[6]);
	      
	      N[indx(6,1,6)]=-(mass1[n+1]*ytavg[3]-racn);
	      N[indx(6,3,6)]= opet;
	      N[indx(6,6,6)]= mass[n+1]*ytavg[2]+ract;
	      
	      // ytdif=(y[indx(6,:,n+1)-yold(:,n+1))/deltim;
	      // cable_resvect=cable_resvect-.5*N*ytdif;
	      ytdif.clear();             
	      for (int j = 1; j<=6; j++)
		ytdif.push_back((y[indx(6,j,n+1)]-yold[indx(6,j,n+1)])/deltim);

	       if (debug )  
		 {
		   for (int j = 1; j <= 6; j++)
		     {
		       fprintf(fpR,"icnt=%d; n=%d+1; ytavg(%d) = %.8e  \n ",icount,n,j,ytavg[j]);
		       fprintf(fpR,"icnt=%d; n=%d+1; ytdif(%d) = %.8e  \n ",icount,n,j,ytdif[j-1]);

		       for (int i = 1; i <= 6; i++)
			 {
			   fprintf(fpR,"icnt=%d; n=%d+1; N(%d,%d) = %.8e  \n ",icount,n,i,j,N[indx(6,i,j)]);
			 }
		     }
		 }


	      vecUtils.mmul(N, 6, 6, ytdif, 1, ytdif);
	      vecUtils.vsmul(ytdif,0.5,ytdif);
	      
	      vecUtils.vsub(cable_resvect,ytdif,cable_resvect);

	    }
	  
	  //        residual[n]=sum(cable_resvect.^2);
	  vector<double> buf;
	  vecUtils.vmul(cable_resvect,cable_resvect,buf);
	  residual.push_back(vecUtils.sum(buf));
	  
	  //        n6=(n-1)*6;
	  //        resvect(1+n6+3:6+n6+3)=cable_resvect;
	  for (int j=0; j<6; j++)
	    {
	      resvect.push_back(cable_resvect[j]);
	      // MOOSTrace("nt= %d, n= %d, j= %d, cable_resvect= %f\n", nt, n, j, cable_resvect[j]);
	    }
	}
      
      
      double cable_residual=vecUtils.vmax(residual);
      // Max_residual=max(towpoint_residual,cable_residual);
      if (cable_residual > towpoint_residual)
	Max_residual = cable_residual;
      else
	Max_residual = towpoint_residual;
      
      //      MOOSTrace("nt=%d, icount = %d, cable, towpoint residuals = %f, %f \n", nt, icount, cable_residual, towpoint_residual);

      int N6=Nseg*6;
      if ( towbody )
	{
	  
	  // Evaluate the residual at the tow body
	  
	  vector<double>  body_resvect;
	  
	  if ( nt==1 )
	    {
	      //    body_resvect=P3x1c(y(:,Nseg+1),rho,vol,area,dragcoef,wgt,...
	      //                       cx[Nseg+1],cy[Nseg+1],gx[Nseg+1],gy[Nseg+1]);
	      
	      double y2mct=y[indx(6,2,Nseg+1)]-(cos(y[indx(6,5,Nseg+1)])*cx[Nseg+1]-sin(y[indx(6,5,Nseg+1)])*cy[Nseg+1])*cos(y[indx(6,6,Nseg+1)]);
	      double y3mcn=y[indx(6,3,Nseg+1)]+(cos(y[indx(6,5,Nseg+1)])*cx[Nseg+1]-sin(y[indx(6,5,Nseg+1)])*cy[Nseg+1])*sin(y[indx(6,6,Nseg+1)]);
	      double y4mcb=y[indx(6,4,Nseg+1)]-(sin(y[indx(6,5,Nseg+1)])*cx[Nseg+1]+cos(y[indx(6,5,Nseg+1)])*cy[Nseg+1]);
	      double wz=-(sin(y[indx(6,6,Nseg+1)])*y[indx(6,2,Nseg+1)]+cos(y[indx(6,6,Nseg+1)])*y[indx(6,3,Nseg+1)]);
	      
	      double root=sqrt(pow(y2mct,2)+pow(y3mcn,2)+pow(y4mcb,2));
	      double drag=.5*rho*area*dragcoef;
	      
	      double P1= drag*y2mct*root+y[indx(6,1,Nseg+1)]+wgt*sin(y[indx(6,6,Nseg+1)])-rho*vol*wz*cos(y[indx(6,6,Nseg+1)])*(cos(y[indx(6,5,Nseg+1)])*gx[Nseg+1]-sin(y[indx(6,5,Nseg+1)])*gy[Nseg+1]);
	      
	      double P2= drag*y3mcn*root+wgt*cos(y[indx(6,6,Nseg+1)])-rho*vol*wz*sin(y[indx(6,6,Nseg+1)])*(-cos(y[indx(6,5,Nseg+1)])*gx[Nseg+1]+sin(y[indx(6,5,Nseg+1)])*gy[Nseg+1]);
	      
	      double P3= drag*y4mcb*root-rho*vol*wz*(sin(y[indx(6,5,Nseg+1)])*gx[Nseg+1]+cos(y[indx(6,5,Nseg+1)])*gy[Nseg+1]);
	      
	      // body_resvect=[P1; P2; P3];
	      
	      body_resvect.push_back(P1);
	      body_resvect.push_back(P2);
	      body_resvect.push_back(P3);
	    }
	  else
	    {
	      //	      ytavg=(y[indx(6,:,Nseg+1)+yold(:,Nseg+1))/2;
	      vector<double> ytavg;
	      
	      ytavg.clear();
	      ytavg.push_back(0.0);
	      for (int j=1; j<= 6; j++)
		ytavg.push_back((y[indx(6,j,Nseg+1)]+yold[indx(6,j,Nseg+1)])/2);
	      
	      
	      //   P=P3x1c(ytavg,rho,vol,area,dragcoef,wgt,cx[Nseg+1],cy[Nseg+1],...
	      //            gx[Nseg+1],gy[Nseg+1]);
	      
	      double y2mct=ytavg[2]-(cos(ytavg[5])*cx[Nseg+1]-sin(ytavg[5])*cy[Nseg+1])*cos(ytavg[6]);
	      double y3mcn=ytavg[3]+(cos(ytavg[5])*cx[Nseg+1]-sin(ytavg[5])*cy[Nseg+1])*sin(ytavg[6]);
	      double y4mcb=ytavg[4]-(sin(ytavg[5])*cx[Nseg+1]+cos(ytavg[5])*cy[Nseg+1]);
	      double wz=-(sin(ytavg[6])*ytavg[2]+cos(ytavg[6])*ytavg[3]);
	      
	      double root=sqrt(pow(y2mct,2)+pow(y3mcn,2)+pow(y4mcb,2));
	      double drag=.5*rho*area*dragcoef;
	      
	      double P1= drag*y2mct*root+ytavg[1]+wgt*sin(ytavg[6])-rho*vol*wz*cos(ytavg[6])*(cos(ytavg[5])*gx[Nseg+1]-sin(ytavg[5])*gy[Nseg+1]);
	      
	      double P2= drag*y3mcn*root+wgt*cos(ytavg[6])-rho*vol*wz*sin(ytavg[6])*(-cos(ytavg[5])*gx[Nseg+1]+sin(ytavg[5])*gy[Nseg+1]);
	      
	      double P3= drag*y4mcb*root-rho*vol*wz*(sin(ytavg[5])*gx[Nseg+1]+cos(ytavg[5])*gy[Nseg+1]);
	      
	      
	      //            P=[P1; P2; P3];
	      vector<double> P;
	      P.push_back(P1);
	      P.push_back(P2);
	      P.push_back(P3);
	      //    A=A3x6(ytavg,tbmass,vol,rho);
	      vector<double> A;	      
	      A = vecUtils.zeros(3,6);
	      double tbmass1=tbmass+rho*vol;
	      
	      A[indx(3,1,2)]= tbmass1;
	      A[indx(3,1,5)]= tbmass1*ytavg[4]*cos(ytavg[6]);
	      A[indx(3,1,6)]=-tbmass1*ytavg[3];
	      
	      A[indx(3,2,3)]= tbmass1;
	      A[indx(3,2,5)]=-tbmass1*ytavg[4]*sin(ytavg[6]);
	      A[indx(3,2,6)]= tbmass1*ytavg[2];
	      
	      A[indx(3,3,4)]= tbmass1;
	      A[indx(3,3,5)]= tbmass1*(ytavg[3]*sin(ytavg[6])-ytavg[2]*cos(ytavg[6]));
	      
	      // ytdif=(y[indx(6,:,Nseg+1)-yold(:,Nseg+1))/deltim;
	      vector<double> ytdif;
	      ytdif.clear();             
	      for (int j = 1; j<=6; j++)
		ytdif.push_back((y[indx(6,j,Nseg+1)]-yold[indx(6,j,Nseg+1)])/deltim);
	      
	      // body_resvect=P+A*ytdif;
	      
	      vecUtils.mmul(A, 3, 6, ytdif, 1, ytdif);
	      vecUtils.vadd(P,ytdif,body_resvect);
	    }
	  
	  //  towbody_residual=sum(body_resvect.^2);
	  vector<double> buf;
	  vecUtils.vmul(body_resvect,body_resvect,buf);         
	  double towbody_residual = vecUtils.sum(buf);
	  
	  //        Max_residual=max(Max_residual,towbody_residual);
	  if (towbody_residual > Max_residual)
	    Max_residual = towbody_residual;		
	  
	  // Monitor convergence with output to screen
	  
	  MOOSTrace("nt = %d, icount =%d, Max_residual= %f\n",nt, icount,Max_residual);
	  //        resvect(1+N6+3:3+N6+3)=body_resvect;
	  for (int j=0; j < body_resvect.size(); j++)
	    resvect.push_back(body_resvect[j]);
	}
      else
	{
	  // Evaluate the residual at the free end
	  vector<double> free_resvect;
	  
	  if (nt==1)
	    {
	      //    free_resvect=Q3x1c(y(:,Nseg+1),rho,dia[Nseg],...
	      //                      nordrag[Nseg],weight[Nseg],...
	      //                      cx[Nseg+1],cy[Nseg+1],gx[Nseg+1],gy[Nseg+1]);
	      
	      double y3mcn=y[indx(6,3,Nseg+1)]+(cos(y[indx(6,5,Nseg+1)])*cx[Nseg+1]-sin(y[indx(6,5,Nseg+1)])*cy[Nseg+1])*sin(y[indx(6,6,Nseg+1)]);
	      double y4mcb=y[indx(6,4,Nseg+1)]-(sin(y[indx(6,5,Nseg+1)])*cx[Nseg+1]+cos(y[indx(6,5,Nseg+1)])*cy[Nseg+1]);
	      double root=sqrt(pow(y3mcn,2)+pow(y4mcb,2));
	      double cs_area=(pi/4)*pow(dia[Nseg],2);
	      double wz=-(sin(y[indx(6,6,Nseg+1)])*y[indx(6,2,Nseg+1)]+cos(y[indx(6,6,Nseg+1)])*y[indx(6,3,Nseg+1)]);
	      
	      double Cy= y[indx(6,1,Nseg+1)];
	      
	      double Q2= .5*rho*dia[Nseg]*nordrag[Nseg]*y4mcb*root-rho*cs_area*wz*(sin(y[indx(6,5,Nseg+1)])*gx[Nseg+1]+cos(y[indx(6,5,Nseg+1)])*gy[Nseg+1]);
	      
	      double Q3= weight[Nseg]*cos(y[indx(6,6,Nseg+1)])+.5*rho*dia[Nseg]*nordrag[Nseg]*y3mcn*root-rho*cs_area*wz*sin(y[indx(6,6,Nseg+1)])*(-cos(y[indx(6,5,Nseg+1)])*gx[Nseg+1]+sin(y[indx(6,5,Nseg+1)])*gy[Nseg+1]);
	      
	      free_resvect.clear();
	      free_resvect.push_back(Cy);
	      free_resvect.push_back(Q2);
	      free_resvect.push_back(Q3);
	      
	      //            free_resvect=[Cy; Q2; Q3];
	    }
	  else
	    {
	      // ytavg=(y[indx(6,:,Nseg+1)+yold(:,Nseg+1))/2;
	      vector<double> ytavg;
	      ytavg.clear();
	      ytavg.push_back(0.0);
	      for (int j=1; j<= 6; j++)
		ytavg.push_back((y[indx(6,j,Nseg+1)]+yold[indx(6,j,Nseg+1)])/2);
	      
	      //    Q=Q3x1c(ytavg,rho,dia[Nseg],nordrag[Nseg],weight[Nseg],...
	      //            cx[Nseg+1],cy[Nseg+1],gx[Nseg+1],gy[Nseg+1]);
	      
	      double y3mcn=ytavg[3]+(cos(ytavg[5])*cx[Nseg+1]-sin(ytavg[5])*cy[Nseg+1])*sin(ytavg[6]);
	      double y4mcb=ytavg[4]-(sin(ytavg[5])*cx[Nseg+1]+cos(ytavg[5])*cy[Nseg+1]);
	      double root=sqrt(pow(y3mcn,2)+pow(y4mcb,2));
	      double cs_area=(pi/4)*pow(dia[Nseg],2);
	      double wz=-(sin(ytavg[6])*ytavg[2]+cos(ytavg[6])*ytavg[3]);
	      
	      double Cy= ytavg[1];
	      
	      double Q2= .5*rho*dia[Nseg]*nordrag[Nseg]*y4mcb*root-rho*cs_area*wz*(sin(ytavg[5])*gx[Nseg+1]+cos(ytavg[5])*gy[Nseg+1]);
	      
	      double Q3= weight[Nseg]*cos(ytavg[6])+.5*rho*dia[Nseg]*nordrag[Nseg]*y3mcn*root-rho*cs_area*wz*sin(ytavg[6])*(-cos(ytavg[5])*gx[Nseg+1]+sin(ytavg[5])*gy[Nseg+1]);
	      
	      
	      // Q=[Cy; Q2; Q3];
	      vector<double> Q;
	      Q.clear();
	      Q.push_back(Cy);
	      Q.push_back(Q2);
	      Q.push_back(Q3);
	      
	      //    B=B3x6c(ytavg,mass[Nseg+1],mass1[Nseg+1],...
	      //            racxx[Nseg+1],racyy[Nseg+1]);
	      vector<double> B; 
	      B = vecUtils.zeros(3,6);
	      
	      double ract= (cos(ytavg[5])*racxx[Nseg+1]-sin(ytavg[5])*racyy[Nseg+1])*cos(ytavg[6]);
	      
	      B[indx(3,3,3)]= mass1[Nseg+1];
	      B[indx(3,2,4)]= mass1[Nseg+1];
	      
	      B[indx(3,2,5)]= mass1[Nseg+1]*ytavg[3]*sin(ytavg[6])-(mass[Nseg+1]*ytavg[2]+ract)*cos(ytavg[6]);
	      B[indx(3,3,5)]=-mass1[Nseg+1]*ytavg[4]*sin(ytavg[6]);
	      
	      B[indx(3,3,6)]= mass[Nseg+1]*ytavg[2]+ract;
	      
	      // ytdif=(y[indx(6,:,Nseg+1)-yold(:,Nseg+1))/deltim;
	      
	      vector<double> ytdif;
	      ytdif.clear();             
	      for (int j = 1; j<=6; j++)
		ytdif.push_back((y[indx(6,j,Nseg+1)]-yold[indx(6,j,Nseg+1)])/deltim);
	      
	      //            free_resvect=Q+B*ytdif;
	      vecUtils.mmul(B, 3, 6, ytdif, 1, ytdif);
	      vecUtils.vadd(Q,ytdif,free_resvect);
	    }
	  
	  //        freeend_residual=sum(free_resvect.^2);
	  
	  vector<double> buf;
	  vecUtils.vmul(free_resvect,free_resvect,buf);         
	  double freeend_residual = vecUtils.sum(buf);
	  
	  //        Max_residual=max(Max_residual,freeend_residual);
	  if (freeend_residual > Max_residual)
	    Max_residual = freeend_residual;		
	  
	  // Monitor convergence with output to screen
	  
	 	  MOOSTrace("free end: nt=%d, icount = %d, Max_residual= %f\n",nt,icount,Max_residual);
	  
	  //        resvect(1+N6+3:3+N6+3)=free_resvect;
	  for (int j=0; j < 3; j++)
	    {
	      //	      MOOSTrace("free_resvect[%d],= %f\n", j, free_resvect[j]);
	      resvect.push_back(free_resvect[j]);
	    }
	}
      
      // Build the Jacobian matrix
      Jacob =  vecUtils.zeros(6*(Nseg+1),6*(Nseg+1));
      Jacobo = vecUtils.zeros(6*(Nseg+1),6*(Nseg+1));
      
      // Define the tow point contribution
      
      
      // Jacob(1:3,1:6)=dtp3x6(y[indx(6,:,1),vx,vy,vz);
      vector<double> dtp;
      dtp = vecUtils.zeros(3,6);
      
      dtp[indx(3,1,2)]= 1.0;
      dtp[indx(3,1,5)]= sin(y[indx(6,5,1)])*cos(y[indx(6,6,1)])*vx + cos(y[indx(6,5,1)])*cos(y[indx(6,6,1)])*vy;
      dtp[indx(3,1,6)]= cos(y[indx(6,5,1)])*sin(y[indx(6,6,1)])*vx - sin(y[indx(6,5,1)])*sin(y[indx(6,6,1)])*vy + cos(y[indx(6,6,1)])*vz;
      
      dtp[indx(3,2,3)]= 1.0;
      dtp[indx(3,2,5)]=-sin(y[indx(6,5,1)])*sin(y[indx(6,6,1)])*vx - cos(y[indx(6,5,1)])*sin(y[indx(6,6,1)])*vy;
      dtp[indx(3,2,6)]= cos(y[indx(6,5,1)])*cos(y[indx(6,6,1)])*vx - sin(y[indx(6,5,1)])*cos(y[indx(6,6,1)])*vy - sin(y[indx(6,6,1)])*vz;
      
      dtp[indx(3,3,4)]= 1.0;
      dtp[indx(3,3,5)]=-cos(y[indx(6,5,1)])*vx + sin(y[indx(6,5,1)])*vy;
      for (int i=1; i<=3; i++)
	for (int j=1; j<=6;j++)
	  Jacob[indx(dimJacob,i,j)] =dtp[indx(3,i,j)];;
      
      // Define the contributions ,to the Jacobian, along the cable
      
      vector<double> M;
      vector<double> dM1;
      vector<double> dM2;
      vector<double> dM3;
      vector<double> dM4;
      vector<double> dM5;
      vector<double> dM6;
      vector<double> DM;
      vector<double> dq;
      
      vector<double> N;
      vector<double> dN1;
      vector<double> dN2;
      vector<double> dN3;
      vector<double> dN4;
      vector<double> dN5;
      vector<double> dN6;
      
      vector<double> dP;
      vector<double> dQ;
      
      vector<double> A;
      vector<double> dA1;
      vector<double> dA2;
      vector<double> dA3;
      vector<double> dA4;
      vector<double> dA5;
      vector<double> dA6;
      
      vector<double> B;
      vector<double> dB1;
      vector<double> dB2;
      vector<double> dB3;
      vector<double> dB4;
      vector<double> dB5;
      vector<double> dB6;
      
      
      vector<double> ysavg;
      vector<double> ysdif;
      vector<double> ytavg;
      vector<double> ytdif;
      
      vector<double> buf ;
      vector<double> buf1;
      vector<double> buf2;
      vector<double> buf3;
      vector<double> buf4;
      

      for (int n=1; n<=Nseg; n++)
	{
	  int n6=(n-1)*6;
	  
	  if ( nt==1 )
	    {
	      // ysavg=(y[indx(6,:,n+1)+y[indx(6,:,n)])/2;
	      ysavg.clear();
	      ysavg.push_back(0.0);
	      for (int j=1; j<= 6; j++)
		ysavg.push_back((y[indx(6,j,n+1)]+y[indx(6,j,n)])/2);
	      
	      // ysdif=(y[indx(6,:,n+1)-y[indx(6,:,n)])/dels[n];
	      
	      ysdif.clear();
	      for (int j=1; j<= 6; j++)
		ysdif.push_back((y[indx(6,j,n+1)]-y[indx(6,j,n)])/dels[n]);
	      
	      //    dM=dM6x6x6(ysavg);
	      //    DM=[dM(:,:,1)*ysdif dM(:,:,2)*ysdif dM(:,:,3)*ysdif ...
	      //        dM(:,:,4)*ysdif dM(:,:,5)*ysdif dM(:,:,6)*ysdif];
	      
	      dM1 = vecUtils.zeros(6,6);
	      dM2 = vecUtils.zeros(6,6);
	      dM3 = vecUtils.zeros(6,6);
	      dM4 = vecUtils.zeros(6,6);
	      dM5 = vecUtils.zeros(6,6);
	      dM6 = vecUtils.zeros(6,6);
	      
	      dM1[indx(6,5,5)]=-cos(ysavg[6]);
	      dM1[indx(6,6,6)]= 1.0;
	      
	      dM2[indx(6,4,5)]=-cos(ysavg[6]);
	      dM2[indx(6,3,6)]= 1.0;
	      
	      dM3[indx(6,4,5)]= sin(ysavg[6]);
	      dM3[indx(6,2,6)]=-1.0;
	      
	      dM4[indx(6,2,5)]= cos(ysavg[6]);
	      dM4[indx(6,3,5)]=-sin(ysavg[6]);
	      
	      dM6[indx(6,2,5)]=-ysavg[4]*sin(ysavg[6]);
	      dM6[indx(6,3,5)]=-ysavg[4]*cos(ysavg[6]);
	      dM6[indx(6,4,5)]= ysavg[3]*cos(ysavg[6])+ysavg[2]*sin(ysavg[6]);
	      dM6[indx(6,5,5)]= ysavg[1]*sin(ysavg[6]);
	      
	      // DM=[dM1*ysdif dM2*ysdif dM3*ysdif dM4*ysdif dM5*ysdif dM6*ysdif];
	      
	      DM.clear();
	      vecUtils.mmul(dM1,6,6,ysdif,1,buf);
	      for (int j=0;j<6;j++)
		DM.push_back(buf[j]); 
	      vecUtils.mmul(dM2,6,6,ysdif,1,buf);
	      for (int j=0;j<6;j++)
		DM.push_back(buf[j]); 
	      vecUtils.mmul(dM3,6,6,ysdif,1,buf);
	      for (int j=0;j<6;j++)
		DM.push_back(buf[j]);
	      vecUtils.mmul(dM4,6,6,ysdif,1,buf);
	      for (int j=0;j<6;j++)
		DM.push_back(buf[j]);
	      vecUtils.mmul(dM5,6,6,ysdif,1,buf);
	      for (int j=0;j<6;j++)
		DM.push_back(buf[j]); 
 	      vecUtils.mmul(dM6,6,6,ysdif,1,buf);
	      for (int j=0;j<6;j++)
		DM.push_back(buf[j]); 
	      
	      
	      //    M=M6x6(ysavg);
	      
	      M = vecUtils.zeros(6,6);
	      
	      M[indx(6,1,1)]= 1.0;
	      M[indx(6,2,2)]= 1.0;
	      M[indx(6,3,3)]= 1.0;
	      M[indx(6,4,4)]= 1.0;
	      
	      M[indx(6,2,5)]= ysavg[4]*cos(ysavg[6]);
	      M[indx(6,3,5)]=-ysavg[4]*sin(ysavg[6]);
	      M[indx(6,4,5)]= ysavg[3]*sin(ysavg[6])-ysavg[2]*cos(ysavg[6]);
	      M[indx(6,5,5)]=-ysavg[1]*cos(ysavg[6]);
	      
	      M[indx(6,2,6)]=-ysavg[3];
	      M[indx(6,3,6)]= ysavg[2];
	      M[indx(6,6,6)]= ysavg[1];
	      
	      double cxavg=(cx[n+1]+cx[n])/2;
	      double cyavg=(cy[n+1]+cy[n])/2;
	      double gxavg=(gx[n+1]+gx[n])/2;
	      double gyavg=(gy[n+1]+gy[n])/2;
	      
	      // dq = dq6x6c(ysavg,rho,dia[n],tandrag[n],nordrag[n],weight[n],...
	      //             ext[n],cxavg,cyavg,gxavg,gyavg);
	      
	      dq = vecUtils.zeros(6,6);
	      double rext=sqrt(1.0+ext[n]*ysavg[1]);
	      double cs_area=(pi/4)*pow(dia[n],2);
	      
	      double y2mct=ysavg[2]-(cos(ysavg[5])*cxavg-sin(ysavg[5])*cyavg)*cos(ysavg[6]);
	      
	      double d2d5=(sin(ysavg[5])*cxavg+cos(ysavg[5])*cyavg)*cos(ysavg[6]);
	      double d2d6=(cos(ysavg[5])*cxavg-sin(ysavg[5])*cyavg)*sin(ysavg[6]);
	      
	      dq[indx(6,1,1)]=.25*rho*dia[n]*ext[n]*pi*tandrag[n]*y2mct*fabs(y2mct)/rext;
	      dq[indx(6,1,2)]= rho*dia[n]*rext*pi*tandrag[n]*fabs(y2mct);
	      dq[indx(6,1,5)]= dq[indx(6,1,2)]*d2d5;
	      dq[indx(6,1,6)]= weight[n]*cos(ysavg[6])+dq[indx(6,1,2)]*d2d6;
	      
	      double y3mcn=ysavg[3]+(cos(ysavg[5])*cxavg-sin(ysavg[5])*cyavg)*sin(ysavg[6]);
	      double y4mcb=ysavg[4]-(sin(ysavg[5])*cxavg+cos(ysavg[5])*cyavg);
	      
	      double d3d5=-(sin(ysavg[5])*cxavg+cos(ysavg[5])*cyavg)*sin(ysavg[6]);
	      double d3d6= (cos(ysavg[5])*cxavg-sin(ysavg[5])*cyavg)*cos(ysavg[6]);
	      double d4d5=-(cos(ysavg[5])*cxavg-sin(ysavg[5])*cyavg);
	      
	      double root=sqrt(pow(y3mcn,2)+pow(y4mcb,2));
	      
	      dq[indx(6,5,1)]= .25*rho*dia[n]*ext[n]*nordrag[n]*y4mcb*root/rext;
	      dq[indx(6,6,1)]= .25*rho*dia[n]*ext[n]*nordrag[n]*y3mcn*root/rext;
	      
	      if (root > 0)
		{
		  dq[indx(6,5,3)]= .5*rho*dia[n]*rext*nordrag[n]*y4mcb*y3mcn/root;
		  dq[indx(6,5,4)]= .5*rho*dia[n]*rext*nordrag[n]*(2*pow(y4mcb,2)+pow(y3mcn,2))/root;
		  
		  dq[indx(6,6,3)]= .5*rho*dia[n]*rext*nordrag[n]*(2*pow(y3mcn,2)+pow(y4mcb,2))/root;
		  dq[indx(6,6,4)]= .5*rho*dia[n]*rext*nordrag[n]*y3mcn*y4mcb/root;
		}
	      
	      dq[indx(6,5,5)]= dq[indx(6,5,3)]*d3d5+dq[indx(6,5,4)]*d4d5;
	      dq[indx(6,5,6)]= dq[indx(6,5,3)]*d3d6;
	      dq[indx(6,6,5)]= dq[indx(6,6,3)]*d3d5+dq[indx(6,6,4)]*d4d5;
	      dq[indx(6,6,6)]=-weight[n]*sin(ysavg[6])+dq[indx(6,6,3)]*d3d6;
	      
	      double wz=-(sin(ysavg[6])*ysavg[2]+cos(ysavg[6])*ysavg[3]);
	      double dwzd2=-sin(ysavg[6]);
	      double dwzd3=-cos(ysavg[6]);
	      double dwzd6=-(cos(ysavg[6])*ysavg[2]-sin(ysavg[6])*ysavg[3]);
	      
	      dq[indx(6,5,2)]=-rho*cs_area*dwzd2*(sin(ysavg[5])*gxavg+cos(ysavg[5])*gyavg);
	      dq[indx(6,6,2)]=-rho*cs_area*dwzd2*sin(ysavg[6])*(-cos(ysavg[5])*gxavg+sin(ysavg[5])*gyavg);
	      dq[indx(6,5,3)]=dq[indx(6,5,3)]-rho*cs_area*dwzd3*(sin(ysavg[5])*gxavg+cos(ysavg[5])*gyavg);
	      dq[indx(6,6,3)]=dq[indx(6,6,3)]-rho*cs_area*dwzd3*sin(ysavg[6])*(-cos(ysavg[5])*gxavg+sin(ysavg[5])*gyavg);
	      
	      dq[indx(6,5,5)]=dq[indx(6,5,5)]-rho*cs_area*wz*(cos(ysavg[5])*gxavg-sin(ysavg[5])*gyavg);
	      dq[indx(6,6,5)]=dq[indx(6,6,5)]-rho*cs_area*wz*sin(ysavg[6])*(sin(ysavg[5])*gxavg+cos(ysavg[5])*gyavg);
	      dq[indx(6,5,6)]=dq[indx(6,5,6)]-rho*cs_area*dwzd6*(sin(ysavg[5])*gxavg+cos(ysavg[5])*gyavg);
	      dq[indx(6,6,6)]=dq[indx(6,6,6)]-rho*cs_area*dwzd6*sin(ysavg[6])*(-cos(ysavg[5])*gxavg+sin(ysavg[5])*gyavg);
	      dq[indx(6,6,6)]=dq[indx(6,6,6)]-rho*cs_area*wz*cos(ysavg[6])*(-cos(ysavg[5])*gxavg+sin(ysavg[5])*gyavg);
	      
	      vecUtils.vsub(DM,dq,buf1);
	      vecUtils.vsmul(buf1,0.5,buf1);
	      vecUtils.vsmul(M,1.0/dels[n],buf2);
	      vecUtils.vsub(buf1,buf2,buf3);
	      vecUtils.vadd(buf1,buf2,buf4);
	      
	      //            Jacob(1+n6+3:6+n6+3,1+n6: 6+n6)=(.5*DM)-(M/dels[n])-(.5*dq);
	      //            Jacob(1+n6+3:6+n6+3,7+n6:12+n6)=(.5*DM)+(M/dels[n])-(.5*dq);
	      for (int i=1; i<=6; i++)
		for (int j=1; j<=6; j++)
		  {
		    Jacob[indx(dimJacob,i+n6+3,j+n6)]=buf3[indx(6,i,j)];
		    Jacob[indx(dimJacob,i+n6+3,6+j+n6)]=buf4[indx(6,i,j)];
		  }
	    }
	  else
	    {
	      //            ysavg=(y[indx(6,:,n+1)+y[indx(6,:,n)])/2;
	      //            ysdif=(y[indx(6,:,n+1)-y[indx(6,:,n)])/dels[n];
	      
	      ysavg.clear();
	      ysavg.push_back(0.0);
	      for (int j=1; j<= 6; j++)
		ysavg.push_back((y[indx(6,j,n+1)]+y[indx(6,j,n)])/2);
	      
	      ysdif.clear();
	      for (int j=1; j<= 6; j++)
		ysdif.push_back((y[indx(6,j,n+1)]-y[indx(6,j,n)])/dels[n]);
	      
	      //    dM=dM6x6x6(ysavg);
	      //    DM=[dM(:,:,1)*ysdif dM(:,:,2)*ysdif dM(:,:,3)*ysdif ...
	      //        dM(:,:,4)*ysdif dM(:,:,5)*ysdif dM(:,:,6)*ysdif];
	      //MOOSTrace("dM\n");
	      dM1 = vecUtils.zeros(6,6);
	      dM2 = vecUtils.zeros(6,6);
	      dM3 = vecUtils.zeros(6,6);
	      dM4 = vecUtils.zeros(6,6);
	      dM5 = vecUtils.zeros(6,6);
	      dM6 = vecUtils.zeros(6,6);
	      
	      dM1[indx(6,5,5)]=-cos(ysavg[6]);
	      dM1[indx(6,6,6)]= 1.0;
	      
	      dM2[indx(6,4,5)]=-cos(ysavg[6]);
	      dM2[indx(6,3,6)]= 1.0;
	      
	      dM3[indx(6,4,5)]= sin(ysavg[6]);
	      dM3[indx(6,2,6)]=-1.0;
	      
	      dM4[indx(6,2,5)]= cos(ysavg[6]);
	      dM4[indx(6,3,5)]=-sin(ysavg[6]);
	      
	      dM6[indx(6,2,5)]=-ysavg[4]*sin(ysavg[6]);
	      dM6[indx(6,3,5)]=-ysavg[4]*cos(ysavg[6]);
	      dM6[indx(6,4,5)]= ysavg[3]*cos(ysavg[6])+ysavg[2]*sin(ysavg[6]);
	      dM6[indx(6,5,5)]= ysavg[1]*sin(ysavg[6]);
	      
	      // DM=[dM1*ysdif dM2*ysdif dM3*ysdif dM4*ysdif dM5*ysdif dM6*ysdif];
	      
	      vecUtils.mmul(dM1,6,6,ysdif,1,buf);
	      for (int j=0;j<6;j++)
		DM.push_back(buf[j]); 
	      vecUtils.mmul(dM2,6,6,ysdif,1,buf);
	      for (int j=0;j<6;j++)
		DM.push_back(buf[j]); 
	      vecUtils.mmul(dM3,6,6,ysdif,1,buf);
	      for (int j=0;j<6;j++)
		DM.push_back(buf[j]);
	      vecUtils.mmul(dM4,6,6,ysdif,1,buf);
	      for (int j=0;j<6;j++)
		DM.push_back(buf[j]);
	      vecUtils.mmul(dM5,6,6,ysdif,1,buf);
	      for (int j=0;j<6;j++)
		DM.push_back(buf[j]); 
 	      vecUtils.mmul(dM6,6,6,ysdif,1,buf);
	      for (int j=0;j<6;j++)
		DM.push_back(buf[j]); 
	      
	      //    M=M6x6(ysavg);
	      
	      //MOOSTrace("M\n");
	      M = vecUtils.zeros(6,6);
	      
	      M[indx(6,1,1)]= 1.0;
	      M[indx(6,2,2)]= 1.0;
	      M[indx(6,3,3)]= 1.0;
	      M[indx(6,4,4)]= 1.0;
	      
	      M[indx(6,2,5)]= ysavg[4]*cos(ysavg[6]);
	      M[indx(6,3,5)]=-ysavg[4]*sin(ysavg[6]);
	      M[indx(6,4,5)]= ysavg[3]*sin(ysavg[6])-ysavg[2]*cos(ysavg[6]);
	      M[indx(6,5,5)]=-ysavg[1]*cos(ysavg[6]);
	      
	      M[indx(6,2,6)]=-ysavg[3];
	      M[indx(6,3,6)]= ysavg[2];
	      M[indx(6,6,6)]= ysavg[1];
	      
      if (debug )  
       {
	 for (int j = 1; j <= 6; j++)
	   for (int i = 1; i <= 6; i++)
	     fprintf(fpM,"n=%d; DM(%d,%d) = %.8e ; M(%d,%d) = %.8e ; \n ",n,i,j,DM[indx(6,i,j)],i,j,M[indx(6,i,j)]);
       }

	      double cxavg=(cx[n+1]+cx[n])/2;
	      double cyavg=(cy[n+1]+cy[n])/2;
	      double gxavg=(gx[n+1]+gx[n])/2;
	      double gyavg=(gy[n+1]+gy[n])/2;
	      
	      // dq = dq6x6c(ysavg,rho,dia[n],tandrag[n],nordrag[n],weight[n],...
	      //             ext[n],cxavg,cyavg,gxavg,gyavg);
	      
	      vector<double> dq;
	      // MOOSTrace("dq\n");

	      dq = vecUtils.zeros(6,6);
	      double rext=sqrt(1.0+ext[n]*ysavg[1]);
	      double cs_area=(pi/4)*pow(dia[n],2);
	      
	      double y2mct=ysavg[2]-(cos(ysavg[5])*cxavg-sin(ysavg[5])*cyavg)*cos(ysavg[6]);
	      
	      double d2d5=(sin(ysavg[5])*cxavg+cos(ysavg[5])*cyavg)*cos(ysavg[6]);
	      double d2d6=(cos(ysavg[5])*cxavg-sin(ysavg[5])*cyavg)*sin(ysavg[6]);
	      
	      dq[indx(6,1,1)]=.25*rho*dia[n]*ext[n]*pi*tandrag[n]*y2mct*fabs(y2mct)/rext;
	      dq[indx(6,1,2)]= rho*dia[n]*rext*pi*tandrag[n]*fabs(y2mct);
	      dq[indx(6,1,5)]= dq[indx(6,1,2)]*d2d5;
	      dq[indx(6,1,6)]= weight[n]*cos(ysavg[6])+dq[indx(6,1,2)]*d2d6;
	      
	      double y3mcn=ysavg[3]+(cos(ysavg[5])*cxavg-sin(ysavg[5])*cyavg)*sin(ysavg[6]);
	      double y4mcb=ysavg[4]-(sin(ysavg[5])*cxavg+cos(ysavg[5])*cyavg);
	      
	      double d3d5=-(sin(ysavg[5])*cxavg+cos(ysavg[5])*cyavg)*sin(ysavg[6]);
	      double d3d6= (cos(ysavg[5])*cxavg-sin(ysavg[5])*cyavg)*cos(ysavg[6]);
	      double d4d5=-(cos(ysavg[5])*cxavg-sin(ysavg[5])*cyavg);
	      
	      double root=sqrt(pow(y3mcn,2)+pow(y4mcb,2));
	      
	      dq[indx(6,5,1)]= .25*rho*dia[n]*ext[n]*nordrag[n]*y4mcb*root/rext;
	      dq[indx(6,6,1)]= .25*rho*dia[n]*ext[n]*nordrag[n]*y3mcn*root/rext;
	      
	      if (root > 0)
		{
		  dq[indx(6,5,3)]= .5*rho*dia[n]*rext*nordrag[n]*y4mcb*y3mcn/root;
		  dq[indx(6,5,4)]= .5*rho*dia[n]*rext*nordrag[n]*(2*pow(y4mcb,2)+pow(y3mcn,2))/root;
		  
		  dq[indx(6,6,3)]= .5*rho*dia[n]*rext*nordrag[n]*(2*pow(y3mcn,2)+pow(y4mcb,2))/root;
		  dq[indx(6,6,4)]= .5*rho*dia[n]*rext*nordrag[n]*y3mcn*y4mcb/root;
		}
	      
	      dq[indx(6,5,5)]= dq[indx(6,5,3)]*d3d5+dq[indx(6,5,4)]*d4d5;
	      dq[indx(6,5,6)]= dq[indx(6,5,3)]*d3d6;
	      dq[indx(6,6,5)]= dq[indx(6,6,3)]*d3d5+dq[indx(6,6,4)]*d4d5;
	      dq[indx(6,6,6)]=-weight[n]*sin(ysavg[6])+dq[indx(6,6,3)]*d3d6;
	      
	      double wz=-(sin(ysavg[6])*ysavg[2]+cos(ysavg[6])*ysavg[3]);
	      double dwzd2=-sin(ysavg[6]);
	      double dwzd3=-cos(ysavg[6]);
	      double dwzd6=-(cos(ysavg[6])*ysavg[2]-sin(ysavg[6])*ysavg[3]);
	      
	      dq[indx(6,5,2)]=-rho*cs_area*dwzd2*(sin(ysavg[5])*gxavg+cos(ysavg[5])*gyavg);
	      dq[indx(6,6,2)]=-rho*cs_area*dwzd2*sin(ysavg[6])*(-cos(ysavg[5])*gxavg+sin(ysavg[5])*gyavg);
	      dq[indx(6,5,3)]=dq[indx(6,5,3)]-rho*cs_area*dwzd3*(sin(ysavg[5])*gxavg+cos(ysavg[5])*gyavg);
	      dq[indx(6,6,3)]=dq[indx(6,6,3)]-rho*cs_area*dwzd3*sin(ysavg[6])*(-cos(ysavg[5])*gxavg+sin(ysavg[5])*gyavg);
	      
	      dq[indx(6,5,5)]=dq[indx(6,5,5)]-rho*cs_area*wz*(cos(ysavg[5])*gxavg-sin(ysavg[5])*gyavg);
	      dq[indx(6,6,5)]=dq[indx(6,6,5)]-rho*cs_area*wz*sin(ysavg[6])*(sin(ysavg[5])*gxavg+cos(ysavg[5])*gyavg);
	      dq[indx(6,5,6)]=dq[indx(6,5,6)]-rho*cs_area*dwzd6*(sin(ysavg[5])*gxavg+cos(ysavg[5])*gyavg);
	      dq[indx(6,6,6)]=dq[indx(6,6,6)]-rho*cs_area*dwzd6*sin(ysavg[6])*(-cos(ysavg[5])*gxavg+sin(ysavg[5])*gyavg);
	      dq[indx(6,6,6)]=dq[indx(6,6,6)]-rho*cs_area*wz*cos(ysavg[6])*(-cos(ysavg[5])*gxavg+sin(ysavg[5])*gyavg);
	      
      if (debug )  
       {
	 for (int j = 1; j <= 6; j++)
	   for (int i = 1; i <= 6; i++)
	     fprintf(fpq,"n=%d; dq(%d,%d) = %.8e  \n ",n,i,j,dq[indx(6,i,j)]);
       }


	      //            Jack1=(.25*DM)-(.5*M/dels[n])-(.25*dq);
	      //            Jack2=(.25*DM)+(.5*M/dels[n])-(.25*dq);
	      
	      vecUtils.vsub(DM,dq,buf1);
	      vecUtils.vsmul(buf1,0.25,buf1);
	      vecUtils.vsmul(M,0.5/dels[n],buf2);
	      
	      vector<double> Jack1;
	      vector<double> Jack2;
	      vecUtils.vsub(buf1,buf2,Jack1);
	      vecUtils.vadd(buf1,buf2,Jack2);
	      
	      //	      ysavg=(yold(:,n+1)+yold(:,n))/2;
	      //	      ysdif=(yold(:,n+1)-yold(:,n))/dels[n];
	      
	      ysavg.clear();
	      ysavg.push_back(0.0);
	      for (int j=1; j<= 6; j++)
		ysavg.push_back((yold[indx(6,j,n+1)]+yold[indx(6,j,n)])/2);
	      
	      ysdif.clear();
	      for (int j=1; j<= 6; j++)
		ysdif.push_back((yold[indx(6,j,n+1)]-yold[indx(6,j,n)])/dels[n]);
	      
	      //    dM=dM6x6x6(ysavg);
	      //    DM=[dM(:,:,1)*ysdif dM(:,:,2)*ysdif dM(:,:,3)*ysdif ...
	      //        dM(:,:,4)*ysdif dM(:,:,5)*ysdif dM(:,:,6)*ysdif];
	      // MOOSTrace("dM\n");

	      dM1 = vecUtils.zeros(6,6);
	      dM2 = vecUtils.zeros(6,6);
	      dM3 = vecUtils.zeros(6,6);
	      dM4 = vecUtils.zeros(6,6);
	      dM5 = vecUtils.zeros(6,6);
	      dM6 = vecUtils.zeros(6,6);
	      
	      dM1[indx(6,5,5)]=-cos(ysavg[6]);
	      dM1[indx(6,6,6)]= 1.0;
	      
	      dM2[indx(6,4,5)]=-cos(ysavg[6]);
	      dM2[indx(6,3,6)]= 1.0;
	      
	      dM3[indx(6,4,5)]= sin(ysavg[6]);
	      dM3[indx(6,2,6)]=-1.0;
	      
	      dM4[indx(6,2,5)]= cos(ysavg[6]);
	      dM4[indx(6,3,5)]=-sin(ysavg[6]);
	      
	      dM6[indx(6,2,5)]=-ysavg[4]*sin(ysavg[6]);
	      dM6[indx(6,3,5)]=-ysavg[4]*cos(ysavg[6]);
	      dM6[indx(6,4,5)]= ysavg[3]*cos(ysavg[6])+ysavg[2]*sin(ysavg[6]);
	      dM6[indx(6,5,5)]= ysavg[1]*sin(ysavg[6]);
	      
	      // DM=[dM1*ysdif dM2*ysdif dM3*ysdif dM4*ysdif dM5*ysdif dM6*ysdif];
	      
	      DM.clear();
	      
	      vecUtils.mmul(dM1,6,6,ysdif,1,buf);
	      for (int j=0;j<6;j++)
		DM.push_back(buf[j]); 
	      vecUtils.mmul(dM2,6,6,ysdif,1,buf);
	      for (int j=0;j<6;j++)
		DM.push_back(buf[j]); 
	      vecUtils.mmul(dM3,6,6,ysdif,1,buf);
	      for (int j=0;j<6;j++)
		DM.push_back(buf[j]);
	      vecUtils.mmul(dM4,6,6,ysdif,1,buf);
	      for (int j=0;j<6;j++)
		DM.push_back(buf[j]);
	      vecUtils.mmul(dM5,6,6,ysdif,1,buf);
	      for (int j=0;j<6;j++)
		DM.push_back(buf[j]); 
 	      vecUtils.mmul(dM6,6,6,ysdif,1,buf);
	      for (int j=0;j<6;j++)
		DM.push_back(buf[j]); 
	      
	      //    M=M6x6(ysavg);
	      
	      // MOOSTrace("M\n");
	      M = vecUtils.zeros(6,6);
	      
	      M[indx(6,1,1)]= 1.0;
	      M[indx(6,2,2)]= 1.0;
	      M[indx(6,3,3)]= 1.0;
	      M[indx(6,4,4)]= 1.0;
	      
	      M[indx(6,2,5)]= ysavg[4]*cos(ysavg[6]);
	      M[indx(6,3,5)]=-ysavg[4]*sin(ysavg[6]);
	      M[indx(6,4,5)]= ysavg[3]*sin(ysavg[6])-ysavg[2]*cos(ysavg[6]);
	      M[indx(6,5,5)]=-ysavg[1]*cos(ysavg[6]);
	      
	      M[indx(6,2,6)]=-ysavg[3];
	      M[indx(6,3,6)]= ysavg[2];
	      M[indx(6,6,6)]= ysavg[1];
	      

      if (debug )  
       {
	 for (int j = 1; j <= 6; j++)
	   for (int i = 1; i <= 6; i++)
	     fprintf(fpM,"n=%d old; DM(%d,%d) = %.8e ; M(%d,%d) = %.8e ; \n ",n,i,j,DM[indx(6,i,j)],i,j,M[indx(6,i,j)]);
       }


	      // dq = dq6x6c(ysavg,rho,dia[n],tandrag[n],nordrag[n],weight[n],...
	      //             ext[n],cxavg,cyavg,gxavg,gyavg);

	      // MOOSTrace("dq\n");
	      
	      dq = vecUtils.zeros(6,6);
	      rext=sqrt(1.0+ext[n]*ysavg[1]);
	      cs_area=(pi/4)*pow(dia[n],2);
	      
	      y2mct=ysavg[2]-(cos(ysavg[5])*cxavg-sin(ysavg[5])*cyavg)*cos(ysavg[6]);
	      
	      d2d5=(sin(ysavg[5])*cxavg+cos(ysavg[5])*cyavg)*cos(ysavg[6]);
	      d2d6=(cos(ysavg[5])*cxavg-sin(ysavg[5])*cyavg)*sin(ysavg[6]);
	      
	      dq[indx(6,1,1)]=.25*rho*dia[n]*ext[n]*pi*tandrag[n]*y2mct*fabs(y2mct)/rext;
	      dq[indx(6,1,2)]= rho*dia[n]*rext*pi*tandrag[n]*fabs(y2mct);
	      dq[indx(6,1,5)]= dq[indx(6,1,2)]*d2d5;
	      dq[indx(6,1,6)]= weight[n]*cos(ysavg[6])+dq[indx(6,1,2)]*d2d6;
	      
	      y3mcn=ysavg[3]+(cos(ysavg[5])*cxavg-sin(ysavg[5])*cyavg)*sin(ysavg[6]);
	      y4mcb=ysavg[4]-(sin(ysavg[5])*cxavg+cos(ysavg[5])*cyavg);
	      
	      d3d5=-(sin(ysavg[5])*cxavg+cos(ysavg[5])*cyavg)*sin(ysavg[6]);
	      d3d6= (cos(ysavg[5])*cxavg-sin(ysavg[5])*cyavg)*cos(ysavg[6]);
	      d4d5=-(cos(ysavg[5])*cxavg-sin(ysavg[5])*cyavg);
	      
	      root=sqrt(pow(y3mcn,2)+pow(y4mcb,2));
	      
	      dq[indx(6,5,1)]= .25*rho*dia[n]*ext[n]*nordrag[n]*y4mcb*root/rext;
	      dq[indx(6,6,1)]= .25*rho*dia[n]*ext[n]*nordrag[n]*y3mcn*root/rext;
	      
	      if (root > 0)
		{
		  dq[indx(6,5,3)]= .5*rho*dia[n]*rext*nordrag[n]*y4mcb*y3mcn/root;
		  dq[indx(6,5,4)]= .5*rho*dia[n]*rext*nordrag[n]*(2*pow(y4mcb,2)+pow(y3mcn,2))/root;
		  
		  dq[indx(6,6,3)]= .5*rho*dia[n]*rext*nordrag[n]*(2*pow(y3mcn,2)+pow(y4mcb,2))/root;
		  dq[indx(6,6,4)]= .5*rho*dia[n]*rext*nordrag[n]*y3mcn*y4mcb/root;
		}
	      
	      dq[indx(6,5,5)]= dq[indx(6,5,3)]*d3d5+dq[indx(6,5,4)]*d4d5;
	      dq[indx(6,5,6)]= dq[indx(6,5,3)]*d3d6;
	      dq[indx(6,6,5)]= dq[indx(6,6,3)]*d3d5+dq[indx(6,6,4)]*d4d5;
	      dq[indx(6,6,6)]=-weight[n]*sin(ysavg[6])+dq[indx(6,6,3)]*d3d6;
	      
	      wz=-(sin(ysavg[6])*ysavg[2]+cos(ysavg[6])*ysavg[3]);
	      dwzd2=-sin(ysavg[6]);
	      dwzd3=-cos(ysavg[6]);
	      dwzd6=-(cos(ysavg[6])*ysavg[2]-sin(ysavg[6])*ysavg[3]);
	      
	      dq[indx(6,5,2)]=-rho*cs_area*dwzd2*(sin(ysavg[5])*gxavg+cos(ysavg[5])*gyavg);
	      dq[indx(6,6,2)]=-rho*cs_area*dwzd2*sin(ysavg[6])*(-cos(ysavg[5])*gxavg+sin(ysavg[5])*gyavg);
	      dq[indx(6,5,3)]=dq[indx(6,5,3)]-rho*cs_area*dwzd3*(sin(ysavg[5])*gxavg+cos(ysavg[5])*gyavg);
	      dq[indx(6,6,3)]=dq[indx(6,6,3)]-rho*cs_area*dwzd3*sin(ysavg[6])*(-cos(ysavg[5])*gxavg+sin(ysavg[5])*gyavg);
	      
	      dq[indx(6,5,5)]=dq[indx(6,5,5)]-rho*cs_area*wz*(cos(ysavg[5])*gxavg-sin(ysavg[5])*gyavg);
	      dq[indx(6,6,5)]=dq[indx(6,6,5)]-rho*cs_area*wz*sin(ysavg[6])*(sin(ysavg[5])*gxavg+cos(ysavg[5])*gyavg);
	      dq[indx(6,5,6)]=dq[indx(6,5,6)]-rho*cs_area*dwzd6*(sin(ysavg[5])*gxavg+cos(ysavg[5])*gyavg);
	      dq[indx(6,6,6)]=dq[indx(6,6,6)]-rho*cs_area*dwzd6*sin(ysavg[6])*(-cos(ysavg[5])*gxavg+sin(ysavg[5])*gyavg);
	      dq[indx(6,6,6)]=dq[indx(6,6,6)]-rho*cs_area*wz*cos(ysavg[6])*(-cos(ysavg[5])*gxavg+sin(ysavg[5])*gyavg);
	      
      if (debug )  
       {
	 for (int j = 1; j <= 6; j++)
	   for (int i = 1; i <= 6; i++)
	     fprintf(fpq,"n=%d old; dq(%d,%d) = %.8e  \n ",n,i,j,dq[indx(6,i,j)]);
       }



	      //            Jack5=(.25*DM)-(.5*M/dels[n])-(.25*dq);
	      //            Jack6=(.25*DM)+(.5*M/dels[n])-(.25*dq);
	      
	      
	      vecUtils.vsub(DM,dq,buf1);
	      vecUtils.vsmul(buf1,0.25,buf1);
	      
	      vecUtils.vsmul(M,0.5/dels[n],buf2);
	      
	      vector<double> Jack5;
	      vector<double> Jack6;
	      vecUtils.vsub(buf1,buf2,Jack5);
	      vecUtils.vadd(buf1,buf2,Jack6);
	      
	      
	      
	      //	      ytavg=(y[indx(6,:,n)+yold(:,n))/2;
	      
	      ytavg.clear();
	      ytavg.push_back(0.0);
	      for (int j=1; j<= 6; j++)
		ytavg.push_back((y[indx(6,j,n)]+yold[indx(6,j,n)])/2);
	      
	      
	      //    N=N6x6c(ytavg,mass[n],mass1[n],ext[n],racxx[n],racyy[n]);
	      // MOOSTrace("N\n");
	      
	      N = vecUtils.zeros(6,6);
	      double opet=1.0+ext[n]*ytavg[1];
	      
	      N[indx(6,1,1)]= -ext[n]*mass[n]*ytavg[2]/opet;
	      N[indx(6,2,1)]=  ext[n];
	      N[indx(6,5,1)]= -ext[n]*mass1[n]*ytavg[4]/opet;
	      N[indx(6,6,1)]= -ext[n]*mass1[n]*ytavg[3]/opet;
	      
	      N[indx(6,1,2)]= mass[n];
	      N[indx(6,6,3)]= mass1[n];
	      N[indx(6,5,4)]= mass1[n];
	      
	      double ract= (cos(ytavg[5])*racxx[n]-sin(ytavg[5])*racyy[n])*cos(ytavg[6]);
	      double racn=-(cos(ytavg[5])*racxx[n]-sin(ytavg[5])*racyy[n])*sin(ytavg[6]);
	      double racb= (sin(ytavg[5])*racxx[n]+cos(ytavg[5])*racyy[n]);
	      
	      N[indx(6,1,5)]= (mass1[n]*ytavg[4]-racb)*cos(ytavg[6]);
	      N[indx(6,4,5)]=-opet*cos(ytavg[6]);
	      N[indx(6,5,5)]= mass1[n]*ytavg[3]*sin(ytavg[6])-(mass[n]*ytavg[2]+ract)*cos(ytavg[6]);
	      N[indx(6,6,5)]=-mass1[n]*ytavg[4]*sin(ytavg[6]);
	      
	      N[indx(6,1,6)]=-(mass1[n]*ytavg[3]-racn);
	      N[indx(6,3,6)]= opet;
	      N[indx(6,6,6)]= mass[n]*ytavg[2]+ract;
	      
	      //  dN=dN6x6x6c(ytavg,mass[n],mass1[n],ext[n],racxx[n],racyy[n]);
	      
	      // MOOSTrace("dN\n");
	      dN1 = vecUtils.zeros(6,6);
	      dN2 = vecUtils.zeros(6,6);
	      dN3 = vecUtils.zeros(6,6);
	      dN4 = vecUtils.zeros(6,6);
	      dN5 = vecUtils.zeros(6,6);
	      dN6 = vecUtils.zeros(6,6);
	      
	      opet=1.0+ext[n]*ytavg[1];
	      
	      dN1[indx(6,1,1)]= mass[n]*pow(ext[n],2)*ytavg[2]/pow(opet,2);
	      dN2[indx(6,1,1)]=-mass[n]*ext[n]/opet;
	      
	      dN1[indx(6,5,1)]= mass1[n]*pow(ext[n],2)*ytavg[4]/pow(opet,2);
	      dN4[indx(6,5,1)]=-mass1[n]*ext[n]/opet;
	      dN1[indx(6,6,1)]= mass1[n]*pow(ext[n],2)*ytavg[3]/pow(opet,2);
	      dN3[indx(6,6,1)]=-mass1[n]*ext[n]/opet;
	      dN1[indx(6,3,6)]= ext[n];
	      
	      ract=    (cos(ytavg[5])*racxx[n]-sin(ytavg[5])*racyy[n])*cos(ytavg[6]);
	      double dractd5=-(sin(ytavg[5])*racxx[n]+cos(ytavg[5])*racyy[n])*cos(ytavg[6]);
	      double dractd6=-(cos(ytavg[5])*racxx[n]-sin(ytavg[5])*racyy[n])*sin(ytavg[6]);
	      racn=   -(cos(ytavg[5])*racxx[n]-sin(ytavg[5])*racyy[n])*sin(ytavg[6]);
	      double dracnd5= (sin(ytavg[5])*racxx[n]+cos(ytavg[5])*racyy[n])*sin(ytavg[6]);
	      double dracnd6=-(cos(ytavg[5])*racxx[n]-sin(ytavg[5])*racyy[n])*cos(ytavg[6]);
	      racb=    (sin(ytavg[5])*racxx[n]+cos(ytavg[5])*racyy[n]);
	      double dracbd5= (cos(ytavg[5])*racxx[n]-sin(ytavg[5])*racyy[n]);
	      
	      dN2[indx(6,5,5)]=-mass[n]*cos(ytavg[6]);
	      dN2[indx(6,6,6)]= mass[n];
	      
	      dN3[indx(6,5,5)]= mass1[n]*sin(ytavg[6]);
	      dN3[indx(6,1,6)]=-mass1[n];
	      
	      dN4[indx(6,1,5)]= mass1[n]*cos(ytavg[6]);
	      dN4[indx(6,6,5)]=-mass1[n]*sin(ytavg[6]);
	      
	      dN5[indx(6,1,5)]=-dracbd5*cos(ytavg[6]);
	      dN5[indx(6,1,6)]= dracnd5;
	      dN5[indx(6,5,5)]=-dractd5*cos(ytavg[6]);
	      dN5[indx(6,6,6)]= dractd5;
	      
	      dN6[indx(6,1,5)]=-(mass1[n]*ytavg[4]-racb)*sin(ytavg[6]);
	      dN6[indx(6,1,6)]= dracnd6;
	      dN1[indx(6,4,5)]=-ext[n]*cos(ytavg[6]);
	      dN6[indx(6,4,5)]= opet*sin(ytavg[6]);
	      dN6[indx(6,5,5)]= mass1[n]*ytavg[3]*cos(ytavg[6])+(mass[n]*ytavg[2]+ract)*sin(ytavg[6])-dractd6*cos(ytavg[6]);
	      dN6[indx(6,6,5)]=-mass1[n]*ytavg[4]*cos(ytavg[6]);
	      dN6[indx(6,6,6)]= dractd6;
	      
	      
	      //            ytdif=(y[indx(6,:,n)-yold(:,n))/deltim;
	      ytdif.clear();
	      for (int j=1; j<= 6; j++)
		ytdif.push_back((y[indx(6,j,n)]-yold[indx(6,j,n)])/deltim);
	      
	      //    DN=[dN[indx(6,:,:,1)*ytdif dN[indx(6,:,:,2)*ytdif dN[indx(6,:,:,3)*ytdif ...
	      //        dN[indx(6,:,:,4)*ytdif dN[indx(6,:,:,5)*ytdif dN[indx(6,:,:,6)*ytdif];
	      
	      //            DN=[dN1*ytdif dN2*ytdif dN3*ytdif dN4*ytdif dN5*ytdif dN6*ytdif];
	      vector<double> DN;    
	      DN.clear();
	      
	      vecUtils.mmul(dN1,6,6,ytdif,1,buf);
	      for (int j=0;j<6;j++)
		DN.push_back(buf[j]); 
	      vecUtils.mmul(dN2,6,6,ytdif,1,buf);
	      for (int j=0;j<6;j++)
		DN.push_back(buf[j]); 
	      vecUtils.mmul(dN3,6,6,ytdif,1,buf);
	      for (int j=0;j<6;j++)
		DN.push_back(buf[j]);
	      vecUtils.mmul(dN4,6,6,ytdif,1,buf);
	      for (int j=0;j<6;j++)
		DN.push_back(buf[j]);
	      vecUtils.mmul(dN5,6,6,ytdif,1,buf);
	      for (int j=0;j<6;j++)
		DN.push_back(buf[j]); 
 	      vecUtils.mmul(dN6,6,6,ytdif,1,buf);
	      for (int j=0;j<6;j++)
		DN.push_back(buf[j]); 
	      
      if (debug )  
       {
	 for (int j = 1; j <= 6; j++)
	   for (int i = 1; i <= 6; i++)
	     fprintf(fpN,"n=%d   ; DN(%d,%d) = %.8e ; N(%d,%d) = %.8e ; \n ",n,i,j,DN[indx(6,i,j)],i,j,N[indx(6,i,j)]);
       }



	      // Jack3=(.25*DN)+(.5*N/deltim);
	      // Jack7=(.25*DN)-(.5*N/deltim);
	      
	      vecUtils.vsmul(DN,0.25,buf1);
	      
	      vecUtils.vsmul(N,0.5/deltim,buf2);
	      
	      vector<double> Jack3;
	      vector<double> Jack7;
	      vecUtils.vadd(buf1,buf2,Jack3);
	      vecUtils.vsub(buf1,buf2,Jack7);
	      
	      
	      //            ytavg=(y[indx(6,:,n+1)+yold(:,n+1))/2;
	      ytavg.clear();
	      ytavg.push_back(0.0);
	      for (int j=1; j<= 6; j++)
		ytavg.push_back((y[indx(6,j,n+1)]+yold[indx(6,j,n+1)])/2);
	      
	      
	      
	      //    N=N6x6c(ytavg,mass[n+1],mass1[n+1],ext[n+1],racxx[n+1],racyy[n+1]);
	      
	      // MOOSTrace("N\n");
	      N = vecUtils.zeros(6,6);
	      opet=1.0+ext[n+1]*ytavg[1];
	      
	      N[indx(6,1,1)]= -ext[n+1]*mass[n+1]*ytavg[2]/opet;
	      N[indx(6,2,1)]=  ext[n+1];
	      N[indx(6,5,1)]= -ext[n+1]*mass1[n+1]*ytavg[4]/opet;
	      N[indx(6,6,1)]= -ext[n+1]*mass1[n+1]*ytavg[3]/opet;
	      
	      N[indx(6,1,2)]= mass[n+1];
	      N[indx(6,6,3)]= mass1[n+1];
	      N[indx(6,5,4)]= mass1[n+1];
	      
	      ract= (cos(ytavg[5])*racxx[n+1]-sin(ytavg[5])*racyy[n+1])*cos(ytavg[6]);
	      racn=-(cos(ytavg[5])*racxx[n+1]-sin(ytavg[5])*racyy[n+1])*sin(ytavg[6]);
	      racb= (sin(ytavg[5])*racxx[n+1]+cos(ytavg[5])*racyy[n+1]);
	      
	      N[indx(6,1,5)]= (mass1[n+1]*ytavg[4]-racb)*cos(ytavg[6]);
	      N[indx(6,4,5)]=-opet*cos(ytavg[6]);
	      N[indx(6,5,5)]= mass1[n+1]*ytavg[3]*sin(ytavg[6])-(mass[n+1]*ytavg[2]+ract)*cos(ytavg[6]);
	      N[indx(6,6,5)]=-mass1[n+1]*ytavg[4]*sin(ytavg[6]);
	      
	      N[indx(6,1,6)]=-(mass1[n+1]*ytavg[3]-racn);
	      N[indx(6,3,6)]= opet;
	      N[indx(6,6,6)]= mass[n+1]*ytavg[2]+ract;
	      
	      //  dN=dN6x6x6c(ytavg,mass[n+1],mass1[n+1],ext[n+1],racxx[n+1],racyy[n+1]);
	      // MOOSTrace("dNx\n");
	      
	      dN1 = vecUtils.zeros(6,6);
	      dN2 = vecUtils.zeros(6,6);
	      dN3 = vecUtils.zeros(6,6);
	      dN4 = vecUtils.zeros(6,6);
	      dN5 = vecUtils.zeros(6,6);
	      dN6 = vecUtils.zeros(6,6);
	      
	      opet=1.0+ext[n+1]*ytavg[1];
	      
	      dN1[indx(6,1,1)]= mass[n+1]*pow(ext[n+1],2)*ytavg[2]/pow(opet,2);
	      dN2[indx(6,1,1)]=-mass[n+1]*ext[n+1]/opet;
	      
	      dN1[indx(6,5,1)]= mass1[n+1]*pow(ext[n+1],2)*ytavg[4]/pow(opet,2);
	      dN4[indx(6,5,1)]=-mass1[n+1]*ext[n+1]/opet;
	      dN1[indx(6,6,1)]= mass1[n+1]*pow(ext[n+1],2)*ytavg[3]/pow(opet,2);
	      dN3[indx(6,6,1)]=-mass1[n+1]*ext[n+1]/opet;
	      dN1[indx(6,3,6)]= ext[n+1];
	      
	      ract=    (cos(ytavg[5])*racxx[n+1]-sin(ytavg[5])*racyy[n+1])*cos(ytavg[6]);
	      dractd5=-(sin(ytavg[5])*racxx[n+1]+cos(ytavg[5])*racyy[n+1])*cos(ytavg[6]);
	      dractd6=-(cos(ytavg[5])*racxx[n+1]-sin(ytavg[5])*racyy[n+1])*sin(ytavg[6]);
	      racn=   -(cos(ytavg[5])*racxx[n+1]-sin(ytavg[5])*racyy[n+1])*sin(ytavg[6]);
	      dracnd5= (sin(ytavg[5])*racxx[n+1]+cos(ytavg[5])*racyy[n+1])*sin(ytavg[6]);
	      dracnd6=-(cos(ytavg[5])*racxx[n+1]-sin(ytavg[5])*racyy[n+1])*cos(ytavg[6]);
	      racb=    (sin(ytavg[5])*racxx[n+1]+cos(ytavg[5])*racyy[n+1]);
	      dracbd5= (cos(ytavg[5])*racxx[n+1]-sin(ytavg[5])*racyy[n+1]);
	      
	      dN2[indx(6,5,5)]=-mass[n+1]*cos(ytavg[6]);
	      dN2[indx(6,6,6)]= mass[n+1];
	      
	      dN3[indx(6,5,5)]= mass1[n+1]*sin(ytavg[6]);
	      dN3[indx(6,1,6)]=-mass1[n+1];
	      
	      dN4[indx(6,1,5)]= mass1[n+1]*cos(ytavg[6]);
	      dN4[indx(6,6,5)]=-mass1[n+1]*sin(ytavg[6]);
	      
	      dN5[indx(6,1,5)]=-dracbd5*cos(ytavg[6]);
	      dN5[indx(6,1,6)]= dracnd5;
	      dN5[indx(6,5,5)]=-dractd5*cos(ytavg[6]);
	      dN5[indx(6,6,6)]= dractd5;
	      
	      dN6[indx(6,1,5)]=-(mass1[n+1]*ytavg[4]-racb)*sin(ytavg[6]);
	      dN6[indx(6,1,6)]= dracnd6;
	      dN1[indx(6,4,5)]=-ext[n+1]*cos(ytavg[6]);
	      dN6[indx(6,4,5)]= opet*sin(ytavg[6]);
	      dN6[indx(6,5,5)]= mass1[n+1]*ytavg[3]*cos(ytavg[6])+(mass[n+1]*ytavg[2]+ract)*sin(ytavg[6])-dractd6*cos(ytavg[6]);
	      dN6[indx(6,6,5)]=-mass1[n+1]*ytavg[4]*cos(ytavg[6]);
	      dN6[indx(6,6,6)]= dractd6;
	      
	      
	      //            ytdif=(y[indx(6,:,n+1)-yold(:,n+1))/deltim;
	      
	      ytdif.clear();
	      for (int j=1; j<= 6; j++)
		ytdif.push_back((y[indx(6,j,n+1)]-yold[indx(6,j,n+1)])/deltim);
	      
	      //    DN=[dN[indx(6,:,:,1)*ytdif dN[indx(6,:,:,2)*ytdif dN[indx(6,:,:,3)*ytdif ...
	      //        dN[indx(6,:,:,4)*ytdif dN[indx(6,:,:,5)*ytdif dN[indx(6,:,:,6)*ytdif];
	      
	      //            DN=[dN1*ytdif dN2*ytdif dN3*ytdif dN4*ytdif dN5*ytdif dN6*ytdif];
	      
	      DN.clear();
	      
	      vecUtils.mmul(dN1,6,6,ytdif,1,buf);
	      for (int j=0;j<6;j++)
		DN.push_back(buf[j]); 
	      vecUtils.mmul(dN2,6,6,ytdif,1,buf);
	      for (int j=0;j<6;j++)
		DN.push_back(buf[j]); 
	      vecUtils.mmul(dN3,6,6,ytdif,1,buf);
	      for (int j=0;j<6;j++)
		DN.push_back(buf[j]);
	      vecUtils.mmul(dN4,6,6,ytdif,1,buf);
	      for (int j=0;j<6;j++)
		DN.push_back(buf[j]);
	      vecUtils.mmul(dN5,6,6,ytdif,1,buf);
	      for (int j=0;j<6;j++)
		DN.push_back(buf[j]); 
 	      vecUtils.mmul(dN6,6,6,ytdif,1,buf);
	      for (int j=0;j<6;j++)
		DN.push_back(buf[j]); 
	      
      if (debug )  
       {
	 for (int j = 1; j <= 6; j++)
	   for (int i = 1; i <= 6; i++)
	     fprintf(fpN,"n=%d+1 ; DN(%d,%d) = %.8e ; N(%d,%d) = %.8e ; \n ",n,i,j,DN[indx(6,i,j)],i,j,N[indx(6,i,j)]);
       }

	      // Jack4=(.25*DN)+(.5*N/deltim);
	      // Jack8=(.25*DN)-(.5*N/deltim);
	      
	      vecUtils.vsmul(DN,0.25,buf1);
	      
	      vecUtils.vsmul(N,0.5/deltim,buf2);
	      
	      vector<double> Jack4;
	      vector<double> Jack8;
	      vecUtils.vadd(buf1,buf2,Jack4);
	      vecUtils.vsub(buf1,buf2,Jack8);
	      
	      // Jacob(1+n6+3:6+n6+3,1+n6: 6+n6)=Jack1-Jack3;
	      // Jacob(1+n6+3:6+n6+3,7+n6:12+n6)=Jack2-Jack4;
	      // Jacobo(1+n6+3:6+n6+3,1+n6: 6+n6)=Jack5-Jack7;
	      // Jacobo(1+n6+3:6+n6+3,7+n6:12+n6)=Jack6-Jack8;
	      for (int i=1; i<=6; i++)
		for (int j=1; j<=6; j++)
		  {
		    Jacob[indx(dimJacob,i+n6+3,j+n6)]=Jack1[indx(6,i,j)]-Jack3[indx(6,i,j)];
		    Jacob[indx(dimJacob,i+n6+3,6+j+n6)]=Jack2[indx(6,i,j)]-Jack4[indx(6,i,j)];
		    Jacobo[indx(dimJacob,i+n6+3,j+n6)]=Jack5[indx(6,i,j)]-Jack7[indx(6,i,j)];
		    Jacobo[indx(dimJacob,i+n6+3,6+j+n6)]=Jack6[indx(6,i,j)]-Jack8[indx(6,i,j)];
		  }
	    }
	}
      //      	      MOOSTrace("Cable done\n");

      // Define the free end or tow body contribution to the Jacobian
      
      N6=Nseg*6;
      if (towbody)
	{
	  if (nt==1)
	    {
	      //    dP=dP3x6c(y(:,Nseg+1),rho,vol,area,dragcoef,wgt,...
	      //              cx[Nseg+1],cy[Nseg+1],gx[Nseg+1],gy[Nseg+1]);
	      
	      dP = vecUtils.zeros(3,6);
	      double drag=.5*rho*area*dragcoef;
	      
	      dP[indx(3,1,1)]= 1.0;
	      
	      double y2mct=y[indx(6,2,Nseg+1)]-(cos(y[indx(6,5,Nseg+1)])*cx[Nseg+1]-sin(y[indx(6,5,Nseg+1)])*cy[Nseg+1])*cos(y[indx(6,6,Nseg+1)]);
	      double y3mcn=y[indx(6,3,Nseg+1)]+(cos(y[indx(6,5,Nseg+1)])*cx[Nseg+1]-sin(y[indx(6,5,Nseg+1)])*cy[Nseg+1])*sin(y[indx(6,6,Nseg+1)]);
	      double y4mcb=y[indx(6,4,Nseg+1)]-(sin(y[indx(6,5,Nseg+1)])*cx[Nseg+1]+cos(y[indx(6,5,Nseg+1)])*cy[Nseg+1]);
	      
	      double d2d5= (sin(y[indx(6,5,Nseg+1)])*cx[Nseg+1]+cos(y[indx(6,5,Nseg+1)])*cy[Nseg+1])*cos(y[indx(6,6,Nseg+1)]);
	      double d2d6= (cos(y[indx(6,5,Nseg+1)])*cx[Nseg+1]-sin(y[indx(6,5,Nseg+1)])*cy[Nseg+1])*sin(y[indx(6,6,Nseg+1)]);
	      double d3d5=-(sin(y[indx(6,5,Nseg+1)])*cx[Nseg+1]+cos(y[indx(6,5,Nseg+1)])*cy[Nseg+1])*sin(y[indx(6,6,Nseg+1)]);
	      double d3d6= (cos(y[indx(6,5,Nseg+1)])*cx[Nseg+1]-sin(y[indx(6,5,Nseg+1)])*cy[Nseg+1])*cos(y[indx(6,6,Nseg+1)]);
	      double d4d5=-(cos(y[indx(6,5,Nseg+1)])*cx[Nseg+1]-sin(y[indx(6,5,Nseg+1)])*cy[Nseg+1]);
	      
	      double root=sqrt(pow(y2mct,2)+pow(y3mcn,2)+pow(y4mcb,2));
	      
	      if (root > 0)
		{
		  dP[indx(3,1,2)]= drag*(2*pow(y2mct,2)+pow(y3mcn,2)+pow(y4mcb,2))/root;
		  dP[indx(3,2,2)]= drag*y3mcn*y2mct/root;
		  dP[indx(3,3,2)]= drag*y4mcb*y2mct/root;
		  
		  dP[indx(3,1,3)]= drag*y2mct*y3mcn/root;
		  dP[indx(3,2,3)]= drag*(2*pow(y3mcn,2)+pow(y2mct,2)+pow(y4mcb,2))/root;
		  dP[indx(3,3,3)]= drag*y4mcb*y3mcn/root;
		  
		  dP[indx(3,1,4)]= drag*y2mct*y4mcb/root;
		  dP[indx(3,2,4)]= drag*y3mcn*y4mcb/root;
		  dP[indx(3,3,4)]= drag*(2*pow(y4mcb,2)+pow(y2mct,2)+pow(y3mcn,2))/root;
		}
	      
	      dP[indx(3,1,5)]= dP[indx(3,1,2)]*d2d5+dP[indx(3,1,3)]*d3d5+dP[indx(3,1,4)]*d4d5;
	      dP[indx(3,1,6)]= wgt*cos(y[indx(6,6,Nseg+1)])+dP[indx(3,1,2)]*d2d6+dP[indx(3,1,3)]*d3d6;
	      dP[indx(3,2,5)]= dP[indx(3,2,2)]*d2d5+dP[indx(3,2,3)]*d3d5+dP[indx(3,2,4)]*d4d5;
	      dP[indx(3,2,6)]=-wgt*sin(y[indx(6,6,Nseg+1)])+dP[indx(3,2,2)]*d2d6+dP[indx(3,2,3)]*d3d6;
	      dP[indx(3,3,5)]= dP[indx(3,3,2)]*d2d5+dP[indx(3,3,3)]*d3d5+dP[indx(3,3,4)]*d4d5;
	      dP[indx(3,3,6)]= dP[indx(3,3,2)]*d2d6+dP[indx(3,3,3)]*d3d6;
	      
	      double wz=-(sin(y[indx(6,6,Nseg+1)])*y[indx(6,2,Nseg+1)]+cos(y[indx(6,6,Nseg+1)])*y[indx(6,3,Nseg+1)]);
	      double dwzd2=-sin(y[indx(6,6,Nseg+1)]);
	      double dwzd3=-cos(y[indx(6,6,Nseg+1)]);
	      double dwzd6=-(cos(y[indx(6,6,Nseg+1)])*y[indx(6,2,Nseg+1)]-sin(y[indx(6,6,Nseg+1)])*y[indx(6,3,Nseg+1)]);
	      
	      dP[indx(3,1,2)]=dP[indx(3,1,2)]-rho*vol*dwzd2*cos(y[indx(6,6,Nseg+1)])*(cos(y[indx(6,5,Nseg+1)])*gx[Nseg+1]-sin(y[indx(6,5,Nseg+1)])*gy[Nseg+1]);
	      dP[indx(3,1,3)]=dP[indx(3,1,3)]-rho*vol*dwzd3*cos(y[indx(6,6,Nseg+1)])*(cos(y[indx(6,5,Nseg+1)])*gx[Nseg+1]-sin(y[indx(6,5,Nseg+1)])*gy[Nseg+1]);
	      
	      dP[indx(3,2,2)]=dP[indx(3,2,2)]-rho*vol*dwzd2*sin(y[indx(6,6,Nseg+1)])*(-cos(y[indx(6,5,Nseg+1)])*gx[Nseg+1]+sin(y[indx(6,5,Nseg+1)])*gy[Nseg+1]);
	      dP[indx(3,3,2)]=dP[indx(3,3,2)]-rho*vol*dwzd2*(sin(y[indx(6,5,Nseg+1)])*gx[Nseg+1]+cos(y[indx(6,5,Nseg+1)])*gy[Nseg+1]);
	      dP[indx(3,2,3)]=dP[indx(3,2,3)]-rho*vol*dwzd3*sin(y[indx(6,6,Nseg+1)])*(-cos(y[indx(6,5,Nseg+1)])*gx[Nseg+1]+sin(y[indx(6,5,Nseg+1)])*gy[Nseg+1]);
	      dP[indx(3,3,3)]=dP[indx(3,3,3)]-rho*vol*dwzd3*(sin(y[indx(6,5,Nseg+1)])*gx[Nseg+1]+cos(y[indx(6,5,Nseg+1)])*gy[Nseg+1]);
	      
	      dP[indx(3,1,5)]=dP[indx(3,1,5)]+rho*vol*wz*cos(y[indx(6,6,Nseg+1)])*(sin(y[indx(6,5,Nseg+1)])*gx[Nseg+1]+cos(y[indx(6,5,Nseg+1)])*gy[Nseg+1]);
	      dP[indx(3,1,6)]=dP[indx(3,1,6)]-rho*vol*dwzd6*cos(y[indx(6,6,Nseg+1)])*(cos(y[indx(6,5,Nseg+1)])*gx[Nseg+1]-sin(y[indx(6,5,Nseg+1)])*gy[Nseg+1]);
	      dP[indx(3,1,6)]=dP[indx(3,1,6)]+rho*vol*wz*sin(y[indx(6,6,Nseg+1)])*(cos(y[indx(6,5,Nseg+1)])*gx[Nseg+1]-sin(y[indx(6,5,Nseg+1)])*gy[Nseg+1]);
	      
	      dP[indx(3,2,5)]=dP[indx(3,2,5)]-rho*vol*wz*sin(y[indx(6,6,Nseg+1)])*(sin(y[indx(6,5,Nseg+1)])*gx[Nseg+1]+cos(y[indx(6,5,Nseg+1)])*gy[Nseg+1]);
	      dP[indx(3,3,5)]=dP[indx(3,3,5)]-rho*vol*wz*(cos(y[indx(6,5,Nseg+1)])*gx[Nseg+1]-sin(y[indx(6,5,Nseg+1)])*gy[Nseg+1]);
	      dP[indx(3,2,6)]=dP[indx(3,2,6)]-rho*vol*dwzd6*sin(y[indx(6,6,Nseg+1)])*(-cos(y[indx(6,5,Nseg+1)])*gx[Nseg+1]+sin(y[indx(6,5,Nseg+1)])*gy[Nseg+1]);
	      dP[indx(3,2,6)]=dP[indx(3,2,6)]-rho*vol*wz*cos(y[indx(6,6,Nseg+1)])*(-cos(y[indx(6,5,Nseg+1)])*gx[Nseg+1]+sin(y[indx(6,5,Nseg+1)])*gy[Nseg+1]);
	      dP[indx(3,3,6)]=dP[indx(3,3,6)]-rho*vol*dwzd6*(sin(y[indx(6,5,Nseg+1)])*gx[Nseg+1]+cos(y[indx(6,5,Nseg+1)])*gy[Nseg+1]);
	      
	      // Jacob(1+N6+3:3+N6+3,1+N6:6+N6)=dP;
	      for (int i=1; i<=3; i++)
		for (int j=1; j<=6; j++)
		  {
		    Jacob[indx(dimJacob,i+N6+3,j+N6)]=dP[indx(3,i,j)];
		  }
	    }
	  
	  else
	    {
	      // ytavg=(y[indx(6,:,Nseg+1)+yold(:,Nseg+1))/2;
	      ytavg.clear();
	      ytavg.push_back(0.0);
	      for (int j=1; j<= 6; j++)
		ytavg.push_back((y[indx(6,j,Nseg+1)]+yold[indx(6,j,Nseg+1)])/2);
	      
	      
            //    A=A3x6(ytavg,tbmass,vol,rho);
	      
	      A = vecUtils.zeros(3,6);
	      double tbmass1=tbmass+rho*vol;
             
	      A[indx(3,1,2)]= tbmass1;
	      A[indx(3,1,5)]= tbmass1*ytavg[4]*cos(ytavg[6]);
	      A[indx(3,1,6)]=-tbmass1*ytavg[3];
	      
	      A[indx(3,2,3)]= tbmass1;
	      A[indx(3,2,5)]=-tbmass1*ytavg[4]*sin(ytavg[6]);
	      A[indx(3,2,6)]= tbmass1*ytavg[2];
	      
	      A[indx(3,3,4)]= tbmass1;
	      A[indx(3,3,5)]= tbmass1*(ytavg[3]*sin(ytavg[6])-ytavg[2]*cos(ytavg[6]));
	      
	      //    dP=dP3x6c(ytavg,rho,vol,area,dragcoef,wgt,...
	      //             cx[Nseg+1],cy[Nseg+1],gx[Nseg+1],gy[Nseg+1]);
	      
	      dP = vecUtils.zeros(3,6);
	      double drag=.5*rho*area*dragcoef;
	      
	      dP[indx(3,1,1)]= 1.0;
	      
	      double y2mct=ytavg[2]-(cos(ytavg[5])*cx[Nseg+1]-sin(ytavg[5])*cy[Nseg+1])*cos(ytavg[6]);
	      double y3mcn=ytavg[3]+(cos(ytavg[5])*cx[Nseg+1]-sin(ytavg[5])*cy[Nseg+1])*sin(ytavg[6]);
	      double y4mcb=ytavg[4]-(sin(ytavg[5])*cx[Nseg+1]+cos(ytavg[5])*cy[Nseg+1]);
	      
	      double d2d5=(sin(ytavg[5])*cx[Nseg+1]+cos(ytavg[5])*cy[Nseg+1])*cos(ytavg[6]);
	      double d2d6=(cos(ytavg[5])*cx[Nseg+1]-sin(ytavg[5])*cy[Nseg+1])*sin(ytavg[6]);
	      double d3d5=-(sin(ytavg[5])*cx[Nseg+1]+cos(ytavg[5])*cy[Nseg+1])*sin(ytavg[6]);
	      double d3d6= (cos(ytavg[5])*cx[Nseg+1]-sin(ytavg[5])*cy[Nseg+1])*cos(ytavg[6]);
	      double d4d5=-(cos(ytavg[5])*cx[Nseg+1]-sin(ytavg[5])*cy[Nseg+1]);
	      
	      double root=sqrt(pow(y2mct,2)+pow(y3mcn,2)+pow(y4mcb,2));
	      
	      if (root > 0)
		{
		  dP[indx(3,1,2)]= drag*(2*pow(y2mct,2)+pow(y3mcn,2)+pow(y4mcb,2))/root;
		  dP[indx(3,2,2)]= drag*y3mcn*y2mct/root;
		  dP[indx(3,3,2)]= drag*y4mcb*y2mct/root;
		  
		  dP[indx(3,1,3)]= drag*y2mct*y3mcn/root;
		  dP[indx(3,2,3)]= drag*(2*pow(y3mcn,2)+pow(y2mct,2)+pow(y4mcb,2))/root;
		  dP[indx(3,3,3)]= drag*y4mcb*y3mcn/root;
		  
		  dP[indx(3,1,4)]= drag*y2mct*y4mcb/root;
		  dP[indx(3,2,4)]= drag*y3mcn*y4mcb/root;
		  dP[indx(3,3,4)]= drag*(2*pow(y4mcb,2)+pow(y2mct,2)+pow(y3mcn,2))/root;
		}
	      
	      dP[indx(3,1,5)]= dP[indx(3,1,2)]*d2d5+dP[indx(3,1,3)]*d3d5+dP[indx(3,1,4)]*d4d5;
	      dP[indx(3,1,6)]= wgt*cos(y[indx(6,6,Nseg+1)])+dP[indx(3,1,2)]*d2d6+dP[indx(3,1,3)]*d3d6;
	      dP[indx(3,2,5)]= dP[indx(3,2,2)]*d2d5+dP[indx(3,2,3)]*d3d5+dP[indx(3,2,4)]*d4d5;
	      dP[indx(3,2,6)]=-wgt*sin(y[indx(6,6,Nseg+1)])+dP[indx(3,2,2)]*d2d6+dP[indx(3,2,3)]*d3d6;
	      dP[indx(3,3,5)]= dP[indx(3,3,2)]*d2d5+dP[indx(3,3,3)]*d3d5+dP[indx(3,3,4)]*d4d5;
	      dP[indx(3,3,6)]= dP[indx(3,3,2)]*d2d6+dP[indx(3,3,3)]*d3d6;
	      
	      double wz=-(sin(ytavg[6])*ytavg[2]+cos(ytavg[6])*ytavg[3]);
	      double dwzd2=-sin(ytavg[6]);
	      double dwzd3=-cos(ytavg[6]);
	      double dwzd6=-(cos(ytavg[6])*ytavg[2]-sin(ytavg[6])*ytavg[3]);
	      
	      dP[indx(3,1,2)]=dP[indx(3,1,2)]-rho*vol*dwzd2*cos(ytavg[6])*(cos(ytavg[5])*gx[Nseg+1]-sin(ytavg[5])*gy[Nseg+1]);
	      dP[indx(3,1,3)]=dP[indx(3,1,3)]-rho*vol*dwzd3*cos(ytavg[6])*(cos(ytavg[5])*gx[Nseg+1]-sin(ytavg[5])*gy[Nseg+1]);
	      
	      dP[indx(3,2,2)]=dP[indx(3,2,2)]-rho*vol*dwzd2*sin(ytavg[6])*(-cos(ytavg[5])*gx[Nseg+1]+sin(ytavg[5])*gy[Nseg+1]);
	      dP[indx(3,3,2)]=dP[indx(3,3,2)]-rho*vol*dwzd2*(sin(ytavg[5])*gx[Nseg+1]+cos(ytavg[5])*gy[Nseg+1]);
	      dP[indx(3,2,3)]=dP[indx(3,2,3)]-rho*vol*dwzd3*sin(ytavg[6])*(-cos(ytavg[5])*gx[Nseg+1]+sin(ytavg[5])*gy[Nseg+1]);
	      dP[indx(3,3,3)]=dP[indx(3,3,3)]-rho*vol*dwzd3*(sin(ytavg[5])*gx[Nseg+1]+cos(ytavg[5])*gy[Nseg+1]);
	      
	      dP[indx(3,1,5)]=dP[indx(3,1,5)]+rho*vol*wz*cos(ytavg[6])*(sin(ytavg[5])*gx[Nseg+1]+cos(ytavg[5])*gy[Nseg+1]);
	      dP[indx(3,1,6)]=dP[indx(3,1,6)]-rho*vol*dwzd6*cos(ytavg[6])*(cos(ytavg[5])*gx[Nseg+1]-sin(ytavg[5])*gy[Nseg+1]);
	      dP[indx(3,1,6)]=dP[indx(3,1,6)]+rho*vol*wz*sin(ytavg[6])*(cos(ytavg[5])*gx[Nseg+1]-sin(ytavg[5])*gy[Nseg+1]);
	      
	      dP[indx(3,2,5)]=dP[indx(3,2,5)]-rho*vol*wz*sin(ytavg[6])*(sin(ytavg[5])*gx[Nseg+1]+cos(ytavg[5])*gy[Nseg+1]);
	      dP[indx(3,3,5)]=dP[indx(3,3,5)]-rho*vol*wz*(cos(ytavg[5])*gx[Nseg+1]-sin(ytavg[5])*gy[Nseg+1]);
	      dP[indx(3,2,6)]=dP[indx(3,2,6)]-rho*vol*dwzd6*sin(ytavg[6])*(-cos(ytavg[5])*gx[Nseg+1]+sin(ytavg[5])*gy[Nseg+1]);
	      dP[indx(3,2,6)]=dP[indx(3,2,6)]-rho*vol*wz*cos(ytavg[6])*(-cos(ytavg[5])*gx[Nseg+1]+sin(ytavg[5])*gy[Nseg+1]);
	      dP[indx(3,3,6)]=dP[indx(3,3,6)]-rho*vol*dwzd6*(sin(ytavg[5])*gx[Nseg+1]+cos(ytavg[5])*gy[Nseg+1]);
	      
	      
	      //   dA=dA3x6x6(ytavg,tbmass,vol,rho);
	      
	      dA1 = vecUtils.zeros(3,6);
	      dA2 = vecUtils.zeros(3,6);
	      dA3 = vecUtils.zeros(3,6);
	      dA4 = vecUtils.zeros(3,6);
	      dA5 = vecUtils.zeros(3,6);
	      dA6 = vecUtils.zeros(3,6);
	      
	      tbmass1=tbmass+rho*vol;
	      
	      dA3[indx(3,1,6)]=-tbmass1;
	      dA4[indx(3,1,5)]= tbmass1*cos(ytavg[6]);
	      dA6[indx(3,1,5)]=-tbmass1*ytavg[4]*sin(ytavg[6]);
	      
	      dA2[indx(3,2,6)]= tbmass1;
	      dA4[indx(3,2,5)]=-tbmass1*sin(ytavg[6]);
	      dA6[indx(3,2,5)]=-tbmass1*ytavg[4]*cos(ytavg[6]);
	      
	      dA2[indx(3,3,5)]=-tbmass1*cos(ytavg[6]);
	      dA3[indx(3,3,5)]= tbmass1*sin(ytavg[6]);
	      dA6[indx(3,3,5)]= tbmass1*(ytavg[3]*cos(ytavg[6])+ytavg[2]*sin(ytavg[6]));
	      
	      //            ytdif=(y[indx(6,:,Nseg+1)-yold(:,Nseg+1))/deltim;
	      
	      ytdif.clear();
	      for (int j=1; j<= 6; j++)
		ytdif.push_back((y[indx(6,j,Nseg+1)]-yold[indx(6,j,Nseg+1)])/deltim);
	      
	      //    DA=[dA[indx(3,:,:,1)*ytdif dA[indx(3,:,:,2)*ytdif dA[indx(3,:,:,3)*ytdif ...
	      //        dA[indx(3,:,:,4)*ytdif dA[indx(3,:,:,5)*ytdif dA[indx(3,:,:,6)*ytdif];
	      
	      // DA=[dA1*ytdif dA2*ytdif dA3*ytdif dA4*ytdif dA5*ytdif dA6*ytdif];
	      vector<double> DA;
	      DA.clear();
	      
	      vecUtils.mmul(dA1,3,6,ytdif,1,buf);
	      for (int j=0;j<6;j++)
		DA.push_back(buf[j]); 
	      vecUtils.mmul(dA2,3,6,ytdif,1,buf);
	      for (int j=0;j<6;j++)
		DA.push_back(buf[j]); 
	      vecUtils.mmul(dA3,3,6,ytdif,1,buf);
	      for (int j=0;j<6;j++)
		DA.push_back(buf[j]);
	      vecUtils.mmul(dA4,3,6,ytdif,1,buf);
	      for (int j=0;j<6;j++)
		DA.push_back(buf[j]);
	      vecUtils.mmul(dA5,3,6,ytdif,1,buf);
	      for (int j=0;j<6;j++)
		DA.push_back(buf[j]); 
 	      vecUtils.mmul(dA6,3,6,ytdif,1,buf);
	      for (int j=0;j<6;j++)
		DA.push_back(buf[j]); 
	      
	      vecUtils.vadd(DA,dP,buf1);
	      vecUtils.vsmul(buf1,0.5,buf1);
	      
	      vecUtils.vsmul(A,1.0/deltim,buf2);
	      vecUtils.vadd(buf1,buf2,buf3);
	      vecUtils.vsub(buf1,buf2,buf4);
	      
	      
	      //            Jacob(1+N6+3:3+N6+3,1+N6:6+N6)=(.5*DA)+(A/deltim)+(.5*dP);
	      //            Jacobo(1+N6+3:3+N6+3,1+N6:6+N6)=(.5*DA)-(A/deltim)+(.5*dP);
	      for (int i=1; i<=3; i++)
		for (int j=1; j<=6; j++)
		  {
		    Jacob[indx(dimJacob,i+N6+3,j+N6)]=buf3[indx(3,i,j)];
		    Jacobo[indx(dimJacob,i+N6+3,j+N6)]=buf4[indx(3,i,j)];
		  }
	    }
	}
      
      else
	{
	  if (nt==1)
	    {
	      
	      //    dQ=dQ3x6c(y(:,Nseg+1),rho,dia[Nseg],nordrag[Nseg],weight[Nseg],...
	      //              cx[Nseg+1],cy[Nseg+1],gx[Nseg+1],gy[Nseg+1]);
	      
	      dQ = vecUtils.zeros(3,6);
	      
	      double cs_area=(pi/4)*pow(dia[Nseg],2);
	      dQ[indx(3,1,1)]= 1.0;
	      
	      double y3mcn=y[indx(6,3,Nseg+1)]+(cos(y[indx(6,5,Nseg+1)])*cx[Nseg+1]-sin(y[indx(6,5,Nseg+1)])*cy[Nseg+1])*sin(y[indx(6,6,Nseg+1)]);
	      double y4mcb=y[indx(6,4,Nseg+1)]-(sin(y[indx(6,5,Nseg+1)])*cx[Nseg+1]+cos(y[indx(6,5,Nseg+1)])*cy[Nseg+1]);
	      
	      double d3d5=-(sin(y[indx(6,5,Nseg+1)])*cx[Nseg+1]+cos(y[indx(6,5,Nseg+1)])*cy[Nseg+1])*sin(y[indx(6,6,Nseg+1)]);
	      double d3d6= (cos(y[indx(6,5,Nseg+1)])*cx[Nseg+1]-sin(y[indx(6,5,Nseg+1)])*cy[Nseg+1])*cos(y[indx(6,6,Nseg+1)]);
	      double d4d5=-(cos(y[indx(6,5,Nseg+1)])*cx[Nseg+1]-sin(y[indx(6,5,Nseg+1)])*cy[Nseg+1]);
	      
	      double root=sqrt(pow(y3mcn,2)+pow(y4mcb,2));
	      if (root > 0)
		{
                dQ[indx(3,2,3)]= .5*rho*dia[Nseg]*nordrag[Nseg]*y4mcb*y3mcn/root;
                dQ[indx(3,2,4)]= .5*rho*dia[Nseg]*nordrag[Nseg]*(2*pow(y4mcb,2)+pow(y3mcn,2))/root;
                
                dQ[indx(3,3,3)]= .5*rho*dia[Nseg]*nordrag[Nseg]*(2*pow(y3mcn,2)+pow(y4mcb,2))/root;
                dQ[indx(3,3,4)]= .5*rho*dia[Nseg]*nordrag[Nseg]*y3mcn*y4mcb/root;
		}
	      
            dQ[indx(3,3,6)]=-weight[Nseg]*sin(y[indx(6,6,Nseg+1)])+dQ[indx(3,3,3)]*d3d6;
            dQ[indx(3,3,5)]=dQ[indx(3,3,3)]*d3d5+dQ[indx(3,3,4)]*d4d5;
	    
            dQ[indx(3,2,5)]=dQ[indx(3,2,3)]*d3d5+dQ[indx(3,2,4)]*d4d5;
            dQ[indx(3,2,6)]=dQ[indx(3,2,3)]*d3d6;
            
            double wz=-(sin(y[indx(6,6,Nseg+1)])*y[indx(6,2,Nseg+1)]+cos(y[indx(6,6,Nseg+1)])*y[indx(6,3,Nseg+1)]);
            double dwzd2=-sin(y[indx(6,6,Nseg+1)]);
            double dwzd3=-cos(y[indx(6,6,Nseg+1)]);
            double dwzd6=-(cos(y[indx(6,6,Nseg+1)])*y[indx(6,2,Nseg+1)]-sin(y[indx(6,6,Nseg+1)])*y[indx(6,3,Nseg+1)]);
            
            dQ[indx(3,2,2)]=-rho*cs_area*dwzd2*(sin(y[indx(6,5,Nseg+1)])*gx[Nseg+1]+cos(y[indx(6,5,Nseg+1)])*gy[Nseg+1]);
            dQ[indx(3,3,2)]=-rho*cs_area*dwzd2*sin(y[indx(6,6,Nseg+1)])*(-cos(y[indx(6,5,Nseg+1)])*gx[Nseg+1]+sin(y[indx(6,5,Nseg+1)])*gy[Nseg+1]);
            dQ[indx(3,2,3)]=dQ[indx(3,2,3)]-rho*cs_area*dwzd3*(sin(y[indx(6,5,Nseg+1)])*gx[Nseg+1]+cos(y[indx(6,5,Nseg+1)])*gy[Nseg+1]);
            dQ[indx(3,3,3)]=dQ[indx(3,3,3)]-rho*cs_area*dwzd3*sin(y[indx(6,6,Nseg+1)])*(-cos(y[indx(6,5,Nseg+1)])*gx[Nseg+1]+sin(y[indx(6,5,Nseg+1)])*gy[Nseg+1]);
            
            dQ[indx(3,2,5)]=dQ[indx(3,2,5)]-rho*cs_area*wz*(cos(y[indx(6,5,Nseg+1)])*gx[Nseg+1]-sin(y[indx(6,5,Nseg+1)])*gy[Nseg+1]);
            dQ[indx(3,3,5)]=dQ[indx(3,3,5)]-rho*cs_area*wz*sin(y[indx(6,6,Nseg+1)])*(sin(y[indx(6,5,Nseg+1)])*gx[Nseg+1]+cos(y[indx(6,5,Nseg+1)])*gy[Nseg+1]);
            dQ[indx(3,2,6)]=dQ[indx(3,2,6)]-rho*cs_area*dwzd6*(sin(y[indx(6,5,Nseg+1)])*gx[Nseg+1]+cos(y[indx(6,5,Nseg+1)])*gy[Nseg+1]);
            dQ[indx(3,3,6)]=dQ[indx(3,3,6)]-rho*cs_area*dwzd6*sin(y[indx(6,6,Nseg+1)])*(-cos(y[indx(6,5,Nseg+1)])*gx[Nseg+1]+sin(y[indx(6,5,Nseg+1)])*gy[Nseg+1]);
            dQ[indx(3,3,6)]=dQ[indx(3,3,6)]-rho*cs_area*wz*cos(y[indx(6,6,Nseg+1)])*(-cos(y[indx(6,5,Nseg+1)])*gx[Nseg+1]+sin(y[indx(6,5,Nseg+1)])*gy[Nseg+1]);
	    
	    //            Jacob(1+N6+3:3+N6+3,1+N6:6+N6)=dQ;
	    for (int i=1; i<=3; i++)
	      for (int j=1; j<=6; j++)
		{
		  Jacob[indx(dimJacob,i+N6+3,j+N6)]=dQ[indx(3,i,j)];
		}
	    }
	  else
	    {
	      //            ytavg=(y[indx(6,:,Nseg+1)]+yold(:,Nseg+1))/2;
	      ytavg.clear();
	      ytavg.push_back(0.0);
	      for (int j=1; j<= 6; j++)
		ytavg.push_back((y[indx(6,j,Nseg+1)]+yold[indx(6,j,Nseg+1)])/2);
	      
	      //    B=B3x6c(ytavg,mass[Nseg+1],mass1[Nseg+1],...
	      //             racxx[Nseg+1],racyy[Nseg+1]);
	      // MOOSTrace("tow point B\n");
	      
	      B = vecUtils.zeros(3,6);
	      
	      double ract= (cos(ytavg[5])*racxx[Nseg+1]-sin(ytavg[5])*racyy[Nseg+1])*cos(ytavg[6]);
	      
	      B[indx(3,3,3)]= mass1[Nseg+1];
	      B[indx(3,2,4)]= mass1[Nseg+1];
	      
	      B[indx(3,2,5)]= mass1[Nseg+1]*ytavg[3]*sin(ytavg[6])-(mass[Nseg+1]*ytavg[2]+ract)*cos(ytavg[6]);
	      B[indx(3,3,5)]=-mass1[Nseg+1]*ytavg[4]*sin(ytavg[6]);
	      
	      B[indx(3,3,6)]= mass[Nseg+1]*ytavg[2]+ract;
	      
	      //    dQ=dQ3x6c(ytavg,rho,dia[Nseg],nordrag[Nseg],weight[Nseg],...
	      //              cx[Nseg+1],cy[Nseg+1],gx[Nseg+1],gy[Nseg+1]);
	      
	      // MOOSTrace("tow point dQ\n");
	      dQ = vecUtils.zeros(3,6);
	      
	      double cs_area=(pi/4)*pow(dia[Nseg],2);
	      
	      dQ[indx(3,1,1)]= 1.0;
	      
	      double y3mcn=ytavg[3]+(cos(ytavg[5])*cx[Nseg+1]-sin(ytavg[5])*cy[Nseg+1])*sin(ytavg[6]);
	      double y4mcb=ytavg[4]-(sin(ytavg[5])*cx[Nseg+1]+cos(ytavg[5])*cy[Nseg+1]);
	      
	      double d3d5=-(sin(ytavg[5])*cx[Nseg+1]+cos(ytavg[5])*cy[Nseg+1])*sin(ytavg[6]);
	      double d3d6= (cos(ytavg[5])*cx[Nseg+1]-sin(ytavg[5])*cy[Nseg+1])*cos(ytavg[6]);
	      double d4d5=-(cos(ytavg[5])*cx[Nseg+1]-sin(ytavg[5])*cy[Nseg+1]);
	      
	      double root=sqrt(pow(y3mcn,2)+pow(y4mcb,2));
	      if (root > 0)
		{
		  dQ[indx(3,2,3)]= .5*rho*dia[Nseg]*nordrag[Nseg]*y4mcb*y3mcn/root;
		  dQ[indx(3,2,4)]= .5*rho*dia[Nseg]*nordrag[Nseg]*(2*pow(y4mcb,2)+pow(y3mcn,2))/root;
		  
		  dQ[indx(3,3,3)]= .5*rho*dia[Nseg]*nordrag[Nseg]*(2*pow(y3mcn,2)+pow(y4mcb,2))/root;
		  dQ[indx(3,3,4)]= .5*rho*dia[Nseg]*nordrag[Nseg]*y3mcn*y4mcb/root;
		}
	      
	      dQ[indx(3,3,6)]=-weight[Nseg]*sin(ytavg[6])+dQ[indx(3,3,3)]*d3d6;
	      dQ[indx(3,3,5)]=dQ[indx(3,3,3)]*d3d5+dQ[indx(3,3,4)]*d4d5;
	      
	      dQ[indx(3,2,5)]=dQ[indx(3,2,3)]*d3d5+dQ[indx(3,2,4)]*d4d5;
	      dQ[indx(3,2,6)]=dQ[indx(3,2,3)]*d3d6;
	      
	      double wz=-(sin(ytavg[6])*ytavg[2]+cos(ytavg[6])*ytavg[3]);
	      double dwzd2=-sin(ytavg[6]);
	      double dwzd3=-cos(ytavg[6]);
	      double dwzd6=-(cos(ytavg[6])*ytavg[2]-sin(ytavg[6])*ytavg[3]);
             
	      dQ[indx(3,2,2)]=-rho*cs_area*dwzd2*(sin(ytavg[5])*gx[Nseg+1]+cos(ytavg[5])*gy[Nseg+1]);
	      dQ[indx(3,3,2)]=-rho*cs_area*dwzd2*sin(ytavg[6])*(-cos(ytavg[5])*gx[Nseg+1]+sin(ytavg[5])*gy[Nseg+1]);
	      dQ[indx(3,2,3)]=dQ[indx(3,2,3)]-rho*cs_area*dwzd3*(sin(ytavg[5])*gx[Nseg+1]+cos(ytavg[5])*gy[Nseg+1]);
	      dQ[indx(3,3,3)]=dQ[indx(3,3,3)]-rho*cs_area*dwzd3*sin(ytavg[6])*(-cos(ytavg[5])*gx[Nseg+1]+sin(ytavg[5])*gy[Nseg+1]);
	      
	      dQ[indx(3,2,5)]=dQ[indx(3,2,5)]-rho*cs_area*wz*(cos(ytavg[5])*gx[Nseg+1]-sin(ytavg[5])*gy[Nseg+1]);
	      dQ[indx(3,3,5)]=dQ[indx(3,3,5)]-rho*cs_area*wz*sin(ytavg[6])*(sin(ytavg[5])*gx[Nseg+1]+cos(ytavg[5])*gy[Nseg+1]);
	      dQ[indx(3,2,6)]=dQ[indx(3,2,6)]-rho*cs_area*dwzd6*(sin(ytavg[5])*gx[Nseg+1]+cos(ytavg[5])*gy[Nseg+1]);
	      dQ[indx(3,3,6)]=dQ[indx(3,3,6)]-rho*cs_area*dwzd6*sin(ytavg[6])*(-cos(ytavg[5])*gx[Nseg+1]+sin(ytavg[5])*gy[Nseg+1]);
	      dQ[indx(3,3,6)]=dQ[indx(3,3,6)]-rho*cs_area*wz*cos(ytavg[6])*(-cos(ytavg[5])*gx[Nseg+1]+sin(ytavg[5])*gy[Nseg+1]);
	      
	      
	      //    dB=dB3x6x6c(ytavg,mass[Nseg+1],mass1[Nseg+1],...
	      //               racxx[Nseg+1],racyy[Nseg+1]);
	      
	      // MOOSTrace("tow point dBx\n");
	      dB1 = vecUtils.zeros(3,6);
	      dB2 = vecUtils.zeros(3,6);
	      dB3 = vecUtils.zeros(3,6);
	      dB4 = vecUtils.zeros(3,6);
	      dB5 = vecUtils.zeros(3,6);
	      dB6 = vecUtils.zeros(3,6);
	      
	      ract=    (cos(ytavg[5])*racxx[Nseg+1]-sin(ytavg[5])*racyy[Nseg+1])*cos(ytavg[6]);
	      double dractd5=-(sin(ytavg[5])*racxx[Nseg+1]+cos(ytavg[5])*racyy[Nseg+1])*cos(ytavg[6]);
	      double dractd6=-(cos(ytavg[5])*racxx[Nseg+1]-sin(ytavg[5])*racyy[Nseg+1])*sin(ytavg[6]);
	      
	      dB2[indx(3,2,5)]=-mass[Nseg+1]*cos(ytavg[6]);
	      dB2[indx(3,3,6)]= mass[Nseg+1];
	      
	      dB3[indx(3,2,5)]= mass1[Nseg+1]*sin(ytavg[6]);
	      
	      dB4[indx(3,3,5)]=-mass1[Nseg+1]*sin(ytavg[6]);
	      
	      dB5[indx(3,2,5)]=-dractd5*cos(ytavg[6]);
	      dB5[indx(3,3,6)]= dractd5;
	      
	      dB6[indx(3,2,5)]= mass1[Nseg+1]*ytavg[3]*cos(ytavg[6])+(mass[Nseg+1]*ytavg[2]+ract)*sin(ytavg[6])-dractd6*cos(ytavg[6]);
	      dB6[indx(3,3,5)]=-mass1[Nseg+1]*ytavg[4]*cos(ytavg[6]);
	      dB6[indx(3,3,6)]= dractd6;
	      // MOOSTrace("tow point dBx done \n");
	      
	      
	      //   ytdif=(y[indx(6,:,Nseg+1)]-yold(:,Nseg+1))/deltim;
	      ytdif.clear();
	      for (int j=1; j<= 6; j++)
		ytdif.push_back((y[indx(6,j,Nseg+1)]-yold[indx(6,j,Nseg+1)])/deltim);
	      
	      
	      //            DB=[dB1*ytdif dB2*ytdif dB3*ytdif dB4*ytdif dB5*ytdif dB6*ytdif];
	      vector<double> DB;
	      DB.clear();
	      
	      vecUtils.mmul(dB1,3,6,ytdif,1,buf);
	      for (int j=0;j<3;j++)
		DB.push_back(buf[j]); 
	      vecUtils.mmul(dB2,3,6,ytdif,1,buf);
	      for (int j=0;j<3;j++)
		DB.push_back(buf[j]); 
	      vecUtils.mmul(dB3,3,6,ytdif,1,buf);
	      for (int j=0;j<3;j++)
		DB.push_back(buf[j]);
	      vecUtils.mmul(dB4,3,6,ytdif,1,buf);
	      for (int j=0;j<3;j++)
		DB.push_back(buf[j]);
	      vecUtils.mmul(dB5,3,6,ytdif,1,buf);
	      for (int j=0;j<3;j++)
		DB.push_back(buf[j]); 
	      vecUtils.mmul(dB6,3,6,ytdif,1,buf);
	      for (int j=0;j<3;j++)
		DB.push_back(buf[j]); 
	      
	      vecUtils.vadd(DB,dQ,buf1);
	      vecUtils.vsmul(buf1,0.5,buf1);
	      
	      vecUtils.vsmul(B,1.0/deltim,buf2);
	      vecUtils.vadd(buf1,buf2,buf3);
	      vecUtils.vsub(buf1,buf2,buf4);
	      
	      // Jacob(1+N6+3:3+N6+3,1+N6:6+N6)=(.5*DB)+(B/deltim)+(.5*dQ);
	      // Jacobo(1+N6+3:3+N6+3,1+N6:6+N6)=(.5*DB)-(B/deltim)+(.5*dQ);
	      for (int i=1; i<=3; i++)
		for (int j=1; j<=6; j++)
		  {
		    Jacob[indx(dimJacob,i+N6+3,j+N6)]=buf3[indx(3,i,j)];
		    Jacobo[indx(dimJacob,i+N6+3,j+N6)]=buf4[indx(3,i,j)];
		  }
	    }
	}
      // MOOSTrace("tow point done \n");

      // Solve Eq. [5] in Milinazzo, Wilkie and Latchman.
     
      //    resvectt=-resvect';
      vector<double> resvectt;;
     vecUtils.vneg(resvect,resvectt);

     // dely=Jacob\resvectt;

     vecUtils.vmov(resvectt,dely);
     MOOSTrace("dely.size = %d\n",dely.size());
     MOOSTrace("resvect.size = %d\n",resvectt.size());
     double get_back = vecUtils.bsolve(Jacob,dimJacob,bw,dely,1);

     vecUtils.vmov(y,bigy);
     vecUtils.vadd(y,dely,y);
     //    notzero=find(abs(bigy) >= 10^-7);
     //		   ftol=max(abs( dely(notzero)./bigy[notzero) ));
     
     // find delta
     ftol = 0;

     int ntol,jtol;
     for (int n=1; n<=Nseg+1; n++)
       for (int j=1; j<=6; j++)
	 if (fabs(bigy[indx(6,j,n)]) > 1e-7)
	 {
	   double xx = fabs(dely[indx(6,j,n)]/bigy[indx(6,j,n)]);
	   if (xx > ftol)
	     {
	       ftol = xx;
	       ntol=n;
	       jtol=j;
	     }
	 } 

      if (debug)  
       {
	 for (int i=1; i<=dimJacob; i++)
	   {
	     int nl = (i+2)/6;
	     int jl = (i-4)%6 +1;
	     fprintf(fpres,"cnt=%d ;resvect(%d,%d) = %.8e ; \n ",icount,nl,jl,resvect[i-1]);
	   } 
	 }
     

      if (debug )  
       {
	 for (int j = 1; j <= Nseg+1; j++)
	   for (int i = 1; i <= 6; i++)
	     fprintf(fpyc,"cnt=%d ;y(%d,%d) = %.8e ; yold(%d,%d) = %.8e ; \n ",icount,i,j,y[indx(6,i,j)],i,j,yold[indx(6,i,j)]);
       }

      if (debug)  
       {
	 for (int i=1; i<=dimJacob; i++)
	   {
	     int j1=max(i-bw,1);
	     int j2=min(i+bw,dimJacob);
	     
	     for (int j=j1; j<=j2; j++)
	       {
		 fprintf(fpJ,"cnt=%d ;Jacob(%d,%d) = %.8e ; \n ",icount,i,j, Jacob[indx(dimJacob,i,j)]);
	       }
	   } 
       }
     MOOSTrace("nt= %d, icount= %d, ftol(%d,%d)= %f\n", nt, icount, jtol,ntol,ftol);
     // **** End of while on (ftol >= 10^-3) ****
     
    }

  if (debug)
    {
	 fclose(fpJ);
	 fclose(fpyc);
	 fclose(fpres);
	 fclose(fpR);
	 fclose(fpM);
	 fclose(fpN);
	 fclose(fpq);
    }

  // Store the refined value of y in ynew for return
   //  InvJacob=inv(Jacob);

  if (!vecUtils.bminv(Jacob,dimJacob,bw,InvJacob))
    count += 1;

  double maxJacob = 0.0;
  double maxInvJacob = 0.0;
  
  for (int i=1; i<= dimJacob; i++)
    {
      double sum=0.0;
      double suminv=0.0;
      
      for (int j=1; j<= dimJacob; j++)
	{
	  sum    +=  fabs(Jacob[indx(dimJacob,i,j)]);
	  suminv +=  fabs(InvJacob[indx(dimJacob,i,j)]);
	}
      maxJacob = max(maxJacob,sum);
      maxInvJacob = max(maxInvJacob,suminv);
    }
  
  double get_back=maxJacob*maxInvJacob;
    
  MOOSTrace("nt=%d - Jacob Condition number = %f\n",nt,get_back);     
 
   if (get_back>1e7 || ftol > 1e-3 )
      {
	count += 1;
      }

	  //    if (cond_check == 1)
	  //   if (sum(sum(isnan(Jacob)))~=0 || sum(sum(isinf(Jacob)))~=0)
	  //       disp('Jacob contains infinite or NAN values');
	  //       get_back = 10^20;
	  //   else
	  //       get_back = cond(Jacob);
	  //   end

      if (debug)  
       {
	 char filenm[100];
	 sprintf(filenm,"nt_%d_inv.m",nt);
	 FILE *fp = fopen(filenm, "w");

	 for (int i=1; i<=dimJacob; i++)
	   {
	     int j1=max(i-2*bw,1);
	     int j2=min(i+2*bw,dimJacob);

	   for (int j=j1; j<=j2; j++)
	     {
		 fprintf(fp,"InvJacob(%d,%d) = %.8e ; \n ; ",i,j, InvJacob[indx(dimJacob,i,j)]);
	     }
	 } 
	 fclose(fp);
       }   
      /*
      */

    //ynew=y;
  vecUtils.vmov(y,ynew);
  return(true);
}  
// >>>>>>>>>>>>>>>>>> End Array dynamic model <<<<<<<<<<<<<<<<<

