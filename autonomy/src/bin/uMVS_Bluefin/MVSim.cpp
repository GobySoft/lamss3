///////////////////////////////////////////////////////////////////////////
//
//   MOOS - Mission Oriented Operating Suite 
//  
//   A suit of Applications and Libraries for Mobile Robotics Research 
//   Copyright (C) 2001-2005 Massachusetts Institute of Technology and 
//   Oxford University. 
//	
//   This software was written by Paul Newman at MIT 2001-2002 and Oxford 
//   University 2003-2005. email: pnewman@robots.ox.ac.uk. 
//	  
//   This file is part of a  MOOS Utility Component. 
//		
//   This program is free software; you can redistribute it and/or 
//   modify it under the terms of the GNU General Public License as 
//   published by the Free Software Foundation; either version 2 of the 
//   License, or (at your option) any later version. 
//		  
//   This program is distributed in the hope that it will be useful, 
//   but WITHOUT ANY WARRANTY; without even the implied warranty of 
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
//   General Public License for more details. 
//			
//   You should have received a copy of the GNU General Public License 
//   along with this program; if not, write to the Free Software 
//   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 
//   02111-1307, USA. 
//
//////////////////////////    END_GPL    //////////////////////////////////

// SimpleAUVSim.cpp: implementation of the CMVSim class.
//
//////////////////////////////////////////////////////////////////////
#include <MOOSLIB/MOOSLib.h>
#include <MOOSGenLib/MOOSGenLib.h>
#include "AcousticResponder.h"
#include "MVSim.h"
#include "math.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cctype>
#include "MBUtils.h"
using namespace std;

#include "AcousticIntersection.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMVSim::CMVSim()
{
    m_dfSimulatorTime = 0.0;
    m_dfLastMailed = 0.0;
    m_dfUpdateRate = 5.0;
    m_dfOldSimulatorTime = 0;

    m_dfStartTime = MOOSTime();

    m_bUseCurrent = false;
    m_bInitialised = false;
    m_bRealTime = true;
    
    SetAppFreq(10);
    SetCommsFreq(20);

    gps_update = false;
    drift_x =0;
    drift_y = 0;
    east_current_est = 0;
    north_current_est = 0;
    previous_gps = MOOSTime();
    dvl_lock = true;

    buoyancy_requested = false;
    trim_requested = false;
    buoyancy_delay = 5;
    max_trim_delay = 10;
    last_report = 0;
    report_interval = 5; 
    // pitch tolerance in degrees
    pitch_tolerance = 5;
}

CMVSim::~CMVSim()
{
    
}

//this will be called for us automatically
bool CMVSim::OnConnectToServer()
{
	return DoRegistrations();
}

bool CMVSim::DoRegistrations()
{
	SIM_ENTITY_LIST::iterator p;

	for(p = m_Entities.begin();p!=m_Entities.end();p++)
	{
		CSimEntity* pEntity = *p;

		CSixDOFAUV* pAUV = dynamic_cast<CSixDOFAUV*> (pEntity);

		if(pAUV)
		{
		  m_Comms.Register(pAUV->m_sInputPrefix+"DESIRED_ELEVATOR",0.0);
		  m_Comms.Register(pAUV->m_sInputPrefix+"DESIRED_RUDDER",0.0);
		  m_Comms.Register(pAUV->m_sInputPrefix+"DESIRED_THRUST",0.0);
		  m_Comms.Register(pAUV->m_sInputPrefix+"CABLE_TENSION",0.0);
		  m_Comms.Register(pAUV->m_sInputPrefix+"ARRAY_SHAPE_X",0.0);
		  m_Comms.Register(pAUV->m_sInputPrefix+"ARRAY_SHAPE_Y",0.0);
		  m_Comms.Register(pAUV->m_sInputPrefix+"ARRAY_SHAPE_Z",0.0);
// 		  m_Comms.Register(pAUV->m_sInputPrefix+"TRIM_KG",1.0);
 		  m_Comms.Register("TRIM_CONTROL",1.0);
 		  m_Comms.Register("BUOYANCY_CONTROL",1.0);
		}
	}

    m_Comms.Register("SIM_RESET",0.02);
    m_Comms.Notify("SIMULATION_MODE","TRUE");

    m_Comms.Register("GPS_UPDATE_RECEIVED",0);
    m_Comms.Register("GPS_X",0);
    m_Comms.Register("GPS_Y",0);

	// current estimate for navigation
	// publish default
	
    m_Comms.Register("CURRENT_X",0);
    m_Comms.Register("CURRENT_Y",0);

    return true;
}

bool CMVSim::OnNewMail(MOOSMSG_LIST &NewMail)
{
    //someone has changed one of the variable we were intersted in!
    MOOSMSG_LIST::iterator p;

    for(p=NewMail.begin();p!=NewMail.end();p++)
    {
        CMOOSMsg & rMsg = *p;
	CSixDOFAUV * pAUV = GetSubscriber(rMsg.m_sKey);

		if(pAUV)
		{
			//we now know which AUV wants this control data..
			if(rMsg.m_sKey.find("DESIRED_ELEVATOR")!=string::npos)
			{
				pAUV->m_dfDesiredElevator = MOOSDeg2Rad(rMsg.m_dfVal);
				continue;
			}
			else if(rMsg.m_sKey.find("DESIRED_RUDDER")!=string::npos)
			{
				pAUV->m_dfDesiredRudder = MOOSDeg2Rad(rMsg.m_dfVal);
				continue;
			}
			else if(rMsg.m_sKey.find("DESIRED_THRUST")!=string::npos)
			{
				pAUV->m_dfDesiredThrust = rMsg.m_dfVal;
				continue;
			}
			else if(rMsg.m_sKey.find("CABLE_TENSION")!=string::npos)
			{
				pAUV->m_dfCableTension = rMsg.m_dfVal;
				continue;
			}
			else if(rMsg.m_sKey.find("ARRAY_SHAPE_X")!=string::npos)
			{
			        pAUV->StoreArrayX(rMsg.m_sVal);
				continue;
			}
			else if(rMsg.m_sKey.find("ARRAY_SHAPE_Y")!=string::npos)
			{
			        pAUV->StoreArrayY(rMsg.m_sVal);
				continue;
			}
			else if(rMsg.m_sKey.find("ARRAY_SHAPE_Z")!=string::npos)
			{
			        pAUV->StoreArrayZ(rMsg.m_sVal);
				continue;
			}
// 			else if(rMsg.m_sKey.find("TRIM_KG")!=string::npos)
// 			{
// 				pAUV->m_dfTrim = rMsg.m_dfVal;
// 				continue;
// 			}
		}
	
		if(MOOSStrCmp(rMsg.m_sKey,"BUOYANCY_CONTROL"))
		  {
		    if (MOOSStrCmp(rMsg.m_sVal,"true"))
		      {
			pAUV->m_dfTrim = 0.0;
			buoyancy_request_time = MOOSTime();
			std::string buoyancy_status="status=1,error=0,progressing,buoyancy=0.0";
			m_Comms.Notify("BUOYANCY_REPORT",buoyancy_status);
			buoyancy_requested = true;
			last_report = buoyancy_request_time;

		      }
		    continue;
		  }
		if(MOOSStrCmp(rMsg.m_sKey,"TRIM_CONTROL"))
		  {
		    if (MOOSStrCmp(rMsg.m_sVal,"true"))
		      {
			trim_request_time = MOOSTime();
			std::string trim_status="status=1,error=0,progressing,trim_pitch=" 
			  + doubleToString(pitch_degrees) + ",trim_roll=0.0";
			m_Comms.Notify("TRIM_REPORT",trim_status);
			trim_requested = true;
			last_report = trim_request_time;
		      }
		    continue;
		  }
		if(MOOSStrCmp(rMsg.m_sKey,"SIM_RESET"))
		  {
		    Initialise();
		  }
                if(rMsg.m_sKey == "CURRENT_X")
		  {
		    east_current_est = rMsg.m_dfVal;
		  }
                if(rMsg.m_sKey == "CURRENT_Y")
		  {
		    north_current_est = rMsg.m_dfVal;
		  }
                if(rMsg.m_sKey == "GPS_X")
		  {
		    gps_x = rMsg.m_dfVal;
		    //nav_x_true = nav_x;
		  }
                if(rMsg.m_sKey == "GPS_Y")
		  {
		    gps_y = rMsg.m_dfVal;
		    //nav_y_true = nav_y;
		  }
                if(rMsg.m_sKey == "GPS_UPDATE_RECEIVED")
		  {
		    double dummy;
		    if (gps_update = MOOSValFromString(dummy,rMsg.m_sVal,"Timestamp"))
		      {
			double nav_lon,nav_lat;
			MOOSValFromString(nav_lat,rMsg.m_sVal,"Latitude");
			MOOSValFromString(nav_lon,rMsg.m_sVal,"Longitude");
			previous_gps = dummy;
			drift_x = 0;
			drift_y = 0;
		      }
		  }
    }


    return true;
}


// who of teh AUVs want to rx this data (look at prefixes)
CSixDOFAUV * CMVSim::GetSubscriber(const std::string & sName)
{
    //index to entities...
	SIM_ENTITY_LIST::iterator p;

	for(p = m_Entities.begin();p!=m_Entities.end();p++)
	{
		CSimEntity* pEntity = *p;

		CSixDOFAUV* pAUV = dynamic_cast<CSixDOFAUV*> (pEntity);

		if(pAUV)
		{
			std::string sPrefix = pAUV->m_sInputPrefix;

			if(MOOSStrCmp(sName.substr(0,sPrefix.length()),sPrefix))
			{
				return pAUV;
			}
		}
	}

	return NULL;
}

bool CMVSim::Iterate()
{
    //have we initialised?
    if(!m_bInitialised)
    {
        Initialise();
    }

    double current_time = MOOSTime();

    // buoyancy and trim control
    if (buoyancy_requested)
      {
	if (current_time-buoyancy_request_time >= buoyancy_delay) 
	  {
	    std::string buoyancy_status="status=2,error=0,completed,buoyancy=0.0";
	    m_Comms.Notify("BUOYANCY_REPORT",buoyancy_status);
	    buoyancy_requested = false;
	  }
	else if (current_time - last_report >= report_interval)
	  {
	    std::string buoyancy_status="status=1,error=0,progressing,buoyancy=0.0";
	    m_Comms.Notify("BUOYANCY_REPORT",buoyancy_status);
	    last_report = current_time; 
	  }
      }
    if (trim_requested)
      {
	if ((fabs(pitch_degrees) <= pitch_tolerance 
	     && current_time-trim_request_time >= buoyancy_delay) 
	    || current_time-trim_request_time >= max_trim_delay) 
	  {
	    std::string trim_status="status=2,error=0,completed,trim_pitch=" 
	      + doubleToString(pitch_degrees) + ",trim_roll=0.0";
	    m_Comms.Notify("TRIM_REPORT",trim_status);
	    trim_requested = false;
	  }
	else if (current_time - last_report >= report_interval) 
	  {
	    std::string trim_status="status=1,error=0,progressing,trim_pitch=" 
	      + doubleToString(pitch_degrees) + ",trim_roll=0.0";
	    m_Comms.Notify("TRIM_REPORT",trim_status);
	    last_report = current_time; 
	  }
      }
    // update nav at gps and start drift
    // H. Schmidt 110212
    if (gps_update)
      {
	GPSUpdate();
	gps_update = false;
      }
    else
      {
	drift_x = (current_time - previous_gps)*east_current_est;
	drift_y = (current_time - previous_gps)*north_current_est;
      }

    ///////////////////////////////////////
    //          sort timing out
    ///////////////////////////////////////
	
    double dfDT  = 0;

	//here we decide what the increment in time is...
	if(m_bRealTime)
	{
		//we will go at real world speed.
	  dfDT     = MOOSTime()-m_dfSimulatorTime;
        

	  if(dfDT==0)
	    {
	      return true;
	    }
	}
	else
	{
        //coarse DT is 0.1 seconds..good as any...
		dfDT = 0.1;
		SetAppFreq(int(1/dfDT));
	}
    

    //the epoch ends at:
	double dfEndTime	=   m_dfSimulatorTime +dfDT;	


    //but we want a fine granularity within this epoch of
    //better than 10 ms.
    //and within this we shall perform linear interpolation.
	double dfSmallDT    =   dfDT;
	int nDiv            =   1;

    do
	{

		dfSmallDT = dfDT/(nDiv++);

	}while(dfSmallDT>0.01);



    /////////////////////////////////////////////////
    //          now propagate models.....
    /////////////////////////////////////////////////
    
	while(m_dfSimulatorTime<=dfEndTime)
	{

        //index to entities...
		SIM_ENTITY_LIST::iterator p;


		//move all entities - Newton(1:3) still rule even in the
        //new millenium 
		for(p = m_Entities.begin();p!=m_Entities.end();p++)
		{
			CSimEntity* pEntity = *p;
			pEntity->Iterate(m_dfSimulatorTime,dfSmallDT);
		}

        //MOOSTrace("\n\nAbout To Solve Acoustics\n");
		//look for intersection with signals
		for(p = m_Entities.begin();p!=m_Entities.end();p++)
		{
			CSimEntity* pEntity = *p;

			pEntity->SolveAcoustics(m_dfSimulatorTime,dfSmallDT);

		}

        //move just a wee bit forward in time...
        m_dfSimulatorTime+=dfSmallDT;
	
    }//completed epoch...



    // housekeeping  - remove old pings in the water column..
	m_Environment.RemoveOldSignals(m_dfSimulatorTime);



    //////////////////////////////////////
    // Tell the world what has happened //
    //////////////////////////////////////
    
	//MOOSTrace("Posting @ %f\n",MOOSTime());
	PostResults();



    return true;
}





bool CMVSim::NeedToMail()
{
    return m_dfSimulatorTime-m_dfLastMailed>1.0/m_dfUpdateRate;

}


bool CMVSim::OnStartUp()
{
    Initialise();

    DoRegistrations();

    return true;
}

bool CMVSim::Initialise()
{
    Clean();
    SetAppFreq(50);
    m_dfLastMailed = 0.0;

    if(m_bRealTime)
      {
	m_dfStartTime = MOOSTime();
	m_dfSimulatorTime = m_dfStartTime;
      }
    else
      {
	m_dfAppStartTime=0.0;
	m_dfStartTime = 0.0;
	m_dfSimulatorTime = 0.0;
      }

    m_dfOldSimulatorTime = m_dfSimulatorTime;
    m_Environment.SetStartTime(m_dfSimulatorTime);
    
    // Now load in terrain data
    string sTerrainFile= "terrain.dat";
    m_MissionReader.GetValue("TerrainFile",sTerrainFile);
    
    if(!m_Environment.Initialise(sTerrainFile.c_str()))
      {
        MOOSTrace("warning environment failed to build\n");
      }
    
    // load general parameters from process config block
    STRING_LIST sParams;
    m_MissionReader.GetConfiguration(GetAppName(),sParams);
    m_Params.Load(sParams);
    
    // load some others in
    m_MissionReader.GetConfigurationParam("TideHeight",m_Environment.m_dfTideHeight);
    m_MissionReader.GetValue("MagneticOffset",m_Environment.m_dfMagneticOffset);

    string dvl_stat;
    if (m_MissionReader.GetConfigurationParam("dvl_lock",dvl_stat))
      dvl_lock = MOOSStrCmp(dvl_stat,"true");

    double new_tol;
    if (m_MissionReader.GetConfigurationParam("trim_tolerance",new_tol))
      {
	pitch_tolerance = new_tol;
	MOOSTrace("Pitch tolerance: %f degrees\n", pitch_tolerance);
      }
    if (m_MissionReader.GetConfigurationParam("max_trim_delay",new_tol))
      {
	max_trim_delay = new_tol;
	MOOSTrace("Max trim delay %f seconds\n", max_trim_delay);
      }
    int  nCurrentElements=0;

    // populate with vehicles and beacons
    STRING_LIST::iterator p;
    for(p = sParams.begin();p!=sParams.end();p++)
      {
	std::string sTmp = *p;
	std::string sTok,sVal;
	CMOOSFileReader::GetTokenValPair(sTmp,sTok,sVal);
	if(MOOSStrCmp(sTok,"ADD_AUV"))
	  {
	    MakeAUV(sVal);
	  }
	else if(MOOSStrCmp(sTok,"ADD_TRANSPONDER"))
	  {
	    MakeBeacon(sVal);
	  }
	else if(MOOSStrCmp(sTok,"CURRENT_DEPTH"))
          {
	    nCurrentElements++;
          }
	else if(MOOSStrCmp(sTok,"CURRENT_SPEED"))
          {
	    nCurrentElements++;
          }
	else if(MOOSStrCmp(sTok,"CURRENT_HEADING"))
          {
	    nCurrentElements++;
          }
      }
    
    if (nCurrentElements==3)
      {
	m_bUseCurrent=true;
        MOOSTrace("Using current profile in MOOS file.\n");
      }
	
    // open log file
    OpenLogFile();
    
    // log all entities' initial state
    LogStartConditions();
    
    m_bInitialised = true;
    
    return true;
}

void CMVSim::GPSUpdate()
{
  //index to entities...
  SIM_ENTITY_LIST::iterator p;
  
  for(p = m_Entities.begin();p!=m_Entities.end();p++)
    {
      CSimEntity* pEntity = *p;
      
      CSixDOFAUV* pAUV = dynamic_cast<CSixDOFAUV*> (pEntity);
      pAUV->SetX(gps_x);
      pAUV->SetY(gps_y);
    }	

  return;
}

void CMVSim::PostResults()
{
    if(NeedToMail())
    {
		//index to entities...
		SIM_ENTITY_LIST::iterator p;
		
		for(p = m_Entities.begin();p!=m_Entities.end();p++)
		{
			CSimEntity* pEntity = *p;
			
			CSixDOFAUV* pAUV = dynamic_cast<CSixDOFAUV*> (pEntity);
			
			if(pAUV==NULL)
				continue;
			
			double dfX = pAUV->GetX();
			if(m_Params.m_bAddNoise)
				dfX+=MOOSWhiteNoise(m_Params.m_dfXYStd);
			
			double dfY = pAUV->GetY();
			if(m_Params.m_bAddNoise)
				dfY+=MOOSWhiteNoise(m_Params.m_dfXYStd);
			
			double dfZ = pAUV->GetZ();
			if(m_Params.m_bAddNoise)
				dfZ+=MOOSWhiteNoise(m_Params.m_dfZStd);
			
			double dfYaw = pAUV->GetYaw();
			if(m_Params.m_bAddNoise)
			{
				dfYaw+=MOOSWhiteNoise(m_Params.m_dfYawStd);
				dfYaw+=m_Params.m_dfYawBias;
			}

			double dfYawRate = pAUV->GetYawRate();
			
			double dfDepth = pAUV->GetDepth();
			if(m_Params.m_bAddNoise)
				dfDepth+=MOOSWhiteNoise(m_Params.m_dfZStd);

			double dfHeading = pAUV->GetHeading();
			if(m_Params.m_bAddNoise)
				dfHeading = -MOOSRad2Deg(dfYaw);
						
			double dfBodyVelY = pAUV->GetBodyVelY();
			if(m_Params.m_bAddNoise)
				dfBodyVelY+=MOOSWhiteNoise(m_Params.m_dfXYVelStd);
			
			double dfBodyVelX = pAUV->GetBodyVelX();
			if(m_Params.m_bAddNoise)
				dfBodyVelX+=MOOSWhiteNoise(m_Params.m_dfXYVelStd);

			double dfSpeed = pAUV->GetSpeed();
			if(m_Params.m_bAddNoise)
				dfSpeed=hypot(dfBodyVelY,dfBodyVelX);

			double dfPitch     = pAUV->GetPitch();
			pitch_degrees = dfPitch*180.0/M_PI;
			double dfPitchRate = pAUV->GetPitchRate();
			double dfRudder    = pAUV->GetRudder();
			double dfElevator  = pAUV->GetElevator();
			double dfTowPosX   = pAUV->GetTowPosX();
			double dfTowPosY   = pAUV->GetTowPosY();
			double dfTowPosZ   = pAUV->GetTowPosZ();
			double dfTowVelX   = pAUV->GetTowVelX();
			double dfTowVelY   = pAUV->GetTowVelY();
			double dfTowVelZ   = pAUV->GetTowVelZ();
			double dfPos1X     = pAUV->GetPos1X();
			double dfPos1Y     = pAUV->GetPos1Y();
			double dfPos1Z     = pAUV->GetPos1Z();
			double dfPos2X     = pAUV->GetPos2X();
			double dfPos2Y     = pAUV->GetPos2Y();
			double dfPos2Z     = pAUV->GetPos2Z();
			double dfPos3X     = pAUV->GetPos3X();
			double dfPos3Y     = pAUV->GetPos3Y();
			double dfPos3Z     = pAUV->GetPos3Z();
			double dfPos4X     = pAUV->GetPos4X();
			double dfPos4Y     = pAUV->GetPos4Y();
			double dfPos4Z     = pAUV->GetPos4Z();
			
			std::string sP = pAUV->m_sOutputPrefix;

			// HS 091009 for gps and lbl simulator
			double nav_x_true = dfX + drift_x;
			double nav_y_true = dfY + drift_y;

			m_Comms.Notify("TRUE_X",nav_x_true);
			m_Comms.Notify("TRUE_Y",nav_y_true);

			// by default the dvl detect current drift
			if (dvl_lock)
			  {
			    m_Comms.Notify(sP+"X",nav_x_true);
			    m_Comms.Notify(sP+"Y",nav_y_true);
			    m_Comms.Notify(sP+"TOW_POS_X",dfTowPosX+drift_x);
			    m_Comms.Notify(sP+"TOW_POS_Y",dfTowPosY+drift_y);
			  }
			else
			  {
			    m_Comms.Notify(sP+"X",dfX);
			    m_Comms.Notify(sP+"Y",dfY);
			    m_Comms.Notify(sP+"TOW_POS_X",dfTowPosX);
			    m_Comms.Notify(sP+"TOW_POS_Y",dfTowPosY);
			  }

			m_Comms.Notify(sP+"Z",dfZ);
			m_Comms.Notify(sP+"YAW",dfYaw);
			m_Comms.Notify(sP+"YAW_RATE",dfYawRate);
			m_Comms.Notify(sP+"DEPTH",dfDepth);
			m_Comms.Notify(sP+"SPEED",dfSpeed);
			m_Comms.Notify(sP+"HEADING",dfHeading);
//			m_Comms.Notify("NAV_HEADING",dfHeading);
			m_Comms.Notify(sP+"BODY_VEL_Y",dfBodyVelY);
			m_Comms.Notify(sP+"BODY_VEL_X",dfBodyVelX);
			m_Comms.Notify(sP+"PITCH",dfPitch);
			m_Comms.Notify(sP+"PITCH_RATE",dfPitchRate);
			// Roll of zero added tu pub list
			m_Comms.Notify(sP+"ROLL",0.0);
			m_Comms.Notify(sP+"RUDDER",dfRudder);
			m_Comms.Notify(sP+"ELEVATOR",dfElevator);
			m_Comms.Notify(sP+"TOW_POS_Z",dfTowPosZ);
			m_Comms.Notify(sP+"TOW_VEL_X",dfTowVelX);
			m_Comms.Notify(sP+"TOW_VEL_Y",dfTowVelY);
			m_Comms.Notify(sP+"TOW_VEL_Z",dfTowVelZ);
			
      /*
      m_Comms.Notify(sP+"ARRAY_HEAD_LEFT_X",dfPos1X);
			m_Comms.Notify(sP+"ARRAY_HEAD_LEFT_Y",dfPos1Y);
			m_Comms.Notify(sP+"ARRAY_HEAD_LEFT_Z",dfPos1Z);
			m_Comms.Notify(sP+"ARRAY_TAIL_LEFT_X",dfPos2X);
			m_Comms.Notify(sP+"ARRAY_TAIL_LEFT_Y",dfPos2Y);
			m_Comms.Notify(sP+"ARRAY_TAIL_LEFT_Z",dfPos2Z);
			m_Comms.Notify(sP+"ARRAY_HEAD_RIGHT_X",dfPos3X);
			m_Comms.Notify(sP+"ARRAY_HEAD_RIGHT_Y",dfPos3Y);
			m_Comms.Notify(sP+"ARRAY_HEAD_RIGHT_Z",dfPos3Z);
			m_Comms.Notify(sP+"ARRAY_TAIL_RIGHT_X",dfPos4X);
			m_Comms.Notify(sP+"ARRAY_TAIL_RIGHT_Y",dfPos4Y);
			m_Comms.Notify(sP+"ARRAY_TAIL_RIGHT_Z",dfPos4Z);
			*/	

			if(pAUV->GetAltitude()>=0)
			{
//				m_Comms.Notify(sP+"ALTITUDE",pAUV->GetAltitude());
			}		
		}    

		m_Comms.Notify("TIDE_HEIGHT",m_Environment.GetTideHeight());
		m_dfLastMailed = m_dfSimulatorTime;
	}

		//finally get all Msg that have appeared in Environments
		//out box, this allows objects from anywhere to send notifications
		MOOSMSG_LIST::iterator q;
		for(q = m_Environment.m_MailOut.begin();q!=m_Environment.m_MailOut.end();q++)
		{
		        //MOOSTrace("Sending Env :%s\n",q->GetString().c_str());
		        m_Comms.Post(*q);
		}

		if(!m_Environment.m_MailOut.empty())
		{
                        //MOOSTrace("Sent %d Mesg from Environment\n",m_Environment.m_MailOut.size());
			m_Environment.m_MailOut.clear();
		}


		//log all entities
		SIM_ENTITY_LIST::iterator p;
		for(p = m_Entities.begin();p!=m_Entities.end();p++)
		{
			(*p)->LogState(m_dfSimulatorTime);
		}
}

bool CMVSim::Clean()
{
    SIM_ENTITY_LIST::iterator p;

    //move all entities
    for(p = m_Entities.begin();p!=m_Entities.end();p++)
    {
	CSimEntity* pEntity = *p;
	delete pEntity;
    }
    m_Entities.clear();    
    m_Environment.Clean();

    return true;
}

/// Simply make an AUV object
bool CMVSim::MakeAUV(std::string sConfig)
{
	//ADD_AUV=  pose=[3x1]{7,3,4,5},name = AUV1,InputPrefix=AUV1,OutputPrefix=AUV1
	CSixDOFAUV * pAUV = new CSixDOFAUV; 
	try
	{
		// What's it called?
		std::string sName;
		if(!MOOSValFromString(sName,sConfig,"name"))
			throw CMOOSException(MOOSFormat("AUV has no name! %s\n",MOOSHERE));
				
		pAUV->SetName(sName);

		// Give it a parameter block
		pAUV->SetParams(&m_Params);
				
	        // Load vehicle-specific parameters
	        STRING_LIST sParams;
	        m_MissionReader.GetConfiguration(GetAppName(),sParams);
	        pAUV->Load(sName,sParams);

		// Initialize vehicle state 
		pAUV->Initialise();

		//where is it?
		std::vector<double> T; int nR,nC;
		if(MOOSValFromString(T,nR,nC,sConfig,"pose"))
		{
			if(nR !=4)
			{
				throw CMOOSException(MOOSFormat("pose must be a 4 vector %s\n",MOOSHERE));
			}
			pAUV->SetX(T[0]);
			pAUV->SetY(T[1]);
			pAUV->SetZ(T[2]);
			pAUV->SetYaw(-M_PI*T[3]/180.0);
			
		}
		
		//how will control be sent to it and positions be emmited?
		std::string sInputPrefix="SIM"; //this would mean it Rxs SIM_DESIRED_RUDDER
		std::string sOutputPrefix="SIM";//and publishes SIM_X etc...
		
		if(!MOOSValFromString(sInputPrefix,sConfig,"InputPrefix"))
		{
			throw CMOOSException(MOOSFormat("No Input prefix defined - assuming  %s\n",sInputPrefix.c_str()));
		}
		if(!MOOSValFromString(sOutputPrefix,sConfig,"OutputPrefix"))
		{
			throw CMOOSException(MOOSFormat("No OutputPrefix  defined - assuming  %s\n",sOutputPrefix.c_str()));
		}
		
		pAUV->SetInputPrefix(sInputPrefix);
		pAUV->SetOutputPrefix(sOutputPrefix);
		
		//tell it about the world
		pAUV->SetEnvironment(&m_Environment);

        //what channel does it reply to interrogations on?
        std::string  sChannelResponder = "Ch2";
		if(!MOOSValFromString(sChannelResponder,sConfig,"ResponderChannel"))
		{
			MOOSTrace("No ResponderChannel  defined - assuming no Responder required \n");

            //turn it off - other parameters don't matter
            pAUV->ConfigureResponder(false,ACOUSTIC_CHAN_CIF,ACOUSTIC_CHAN_CIF,-1);

		}
        else
        {
            AcousticChannel eCh = CAcousticSignal::ChannelFromString(sChannelResponder);
                        
            //what is its TAT on interrogations?
            double dfTAT = 0.125;
            if(!MOOSValFromString(dfTAT,sConfig,"TAT"))
            {
                throw CMOOSException(MOOSFormat("No TAT defined - assuming  %f\n",dfTAT));
            }
                        
            //turn it on..
            pAUV->ConfigureResponder(true,ACOUSTIC_CHAN_CIF,eCh,dfTAT);
        }
		//store it
		m_Entities.push_front(pAUV);
	}
	catch(CMOOSException e)
	{
		delete pAUV;
		return MOOSFail(e.m_sReason);
	}

    return true;
}

bool CMVSim::MakeBeacon(std::string sConfig)
{
	//"ADD_TRANSPONDER = Name = B1,pose=[3x1]{23, 45, 0},TAT = 0.125,RX = CH8 | CH9 | C10,TX = CIF

	CAcousticBeacon * pBeacon = new CAcousticBeacon;
	try
	{

		std::vector<double> T; int nR,nC;
		if(MOOSValFromString(T,nR,nC,sConfig,"pose"))
		{
			if(nR !=3)
			{
				return MOOSFail("pose must be a 3 vector %s\n",MOOSHERE);
			}
			pBeacon->m_Pos_e<<T[0]
				<<T[1]
				<<T[2]
				<<0
				<<0
				<<0;
		}
		
		//whats it called
		std::string sName;
		if(!MOOSValFromString(sName,sConfig,"name"))
			throw CMOOSException(MOOSFormat("Beacon has no name! %s %s\n",sConfig.c_str(),MOOSHERE));
		pBeacon->SetName(sName);
		
		std::string sTx;
		if(!MOOSValFromString(sTx,sConfig,"Tx"))
			throw CMOOSException(MOOSFormat("Beacon has no Tx field %s %s\n",sConfig.c_str(),MOOSHERE));
		
		
		ACOUSTIC_NODE_LIST AcousticNodes;
		pBeacon->GetAcousticNodes(AcousticNodes);
		
		AcousticChannel eChan = CAcousticSignal::ChannelFromString(sTx);
		
		if( eChan==ACOUSTIC_CHAN_ERROR ||
			!AcousticNodes.front()->SetTxChan(eChan))
		{
			throw CMOOSException(MOOSFormat("error setting Tx Channel on beacon %s\n",sName.c_str()));
		}
		
		std::string sRx;
		if(!MOOSValFromString(sTx,sConfig,"Rx"))
			throw CMOOSException(MOOSFormat("Beacon has no Rx field %s %s\n",sConfig.c_str(),MOOSHERE));
		
		
		
		while(!sRx.empty())
		{
			string sChan = MOOSChomp(sRx,"|");
			
			AcousticChannel eChan = CAcousticSignal::ChannelFromString(sChan);
			
			if( eChan==ACOUSTIC_CHAN_ERROR ||
				!AcousticNodes.front()->SetRxChan(eChan,true))
			{ 
				MOOSTrace("error setting Rx Channel on beacon\n");
			}
		}//for all channels
		
		
		////////////////////////////
		//      set up TAT
		////////////////////////////
		
		double dfTAT;
		if(!MOOSValFromString(dfTAT,sConfig,"TAT"))
			throw CMOOSException(MOOSFormat("Beacon has no TAT field %s %s\n",sConfig.c_str(),MOOSHERE));
		
		
		CAcousticResponder * pResponder = dynamic_cast<CAcousticResponder*>(AcousticNodes.front());
		if(pResponder!=NULL)
		{
			pResponder->SetTAT(dfTAT);
		}
		
		//final set up of beacon...
		pBeacon->SetEnvironment(&m_Environment);
		
		pBeacon->SetParams(&m_Params);
		
		m_Entities.push_front(pBeacon);
	}
	catch (CMOOSException e)
	{
		delete pBeacon;
		return MOOSFail(e.m_sReason);
	}

	return true;
}            
	


bool CMVSim::OpenLogFile()
{
	//set up log file...
	if(!m_Params.m_sLogFileName.empty())
	{
		//open and close ..truncates file
		ofstream os(m_Params.m_sLogFileName.c_str());
		
		os.close();
	}
	return true;
}

bool CMVSim::LogStartConditions()
{
	SIM_ENTITY_LIST::iterator p;

	if(!m_Params.m_sLogFileName.empty())
	{
		//open and close ..truncates file
		ofstream os(m_Params.m_sLogFileName.c_str(),ios::app);

		//start time...
		os.setf(ios::fixed,ios::floatfield);
		os<<setprecision(3);
		os<<"StartTime,T="<<setprecision(3)<<m_dfSimulatorTime<<endl;

		//entity states
		for(p = m_Entities.begin();p!=m_Entities.end();p++)
		{
			(*p)->CSimEntity::LogState(m_dfSimulatorTime);
		}


	}

	return true;
}
