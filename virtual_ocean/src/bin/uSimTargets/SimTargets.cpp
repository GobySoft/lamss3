//SimTargets.cpp    Henrik Schmidt, MIT, Oct 2008

/*This program, SimTargets, is used in conjuction the array an acoustic simulators. 
It is a multitarget generalization of pTargetSim.
SimTargets listens for messages
published by pNaFCon (or an adhoc message from the iRemote key binding 
feature) that gives a Target_State of x,y,depth,heading,speed,time, 
and a Prosecute_State equal to prosecute, abort or false. If the state is prosecute,
and Target_State values are given, SimTargets will publish TARGET_XPOS and 
TARGET_YPOS, the current x and y positions of the target. 
*/
#include "SimTargets.h"

//***************************************************************************
//*  CSimTargets::CSimTargets()                                                
//*                                                                         *
//*  The constructor for the CBearings class. Variables can be initialized  *
//*  here.                                                                  *
//*                                                                         *
//***************************************************************************

using namespace std;

CSimTargets::CSimTargets()
{
  source_control = false;    
  Targets.clear();
  target_number = 0;
  target_init = false;
  got_x = false;
  got_y = false;

  t_num = 0;
  rangeCrit = 0;
  noRangeCrit = false;
}

//***************************************************************************
//*  CSimTargets::~CSimTargets()                                                *
//*                                                                         *
//*  The destructor for the CSimTargets class. Dynamic memory can be freed    *
//*  here.                                                                  *
//***************************************************************************

CSimTargets::~CSimTargets()
{
    //DOES ANYTHING NEED TO HAPPEN IN THIS DESTRUCTOR?    
}

//***************************************************************************
//*  CSimTargets::OnConnectToServer()                                          *
//*                                                                         *
//*  Called by the MOOSApp base class upon connecting to the MOOS database. *
//*  Register for variables here.                                           *
//*                                                                         *
//***************************************************************************

bool CSimTargets::OnConnectToServer()
{
  source_control = false;

  m_Comms.Register ("TARGET_CONTROL", 0);
  m_Comms.Register ("TARGET_CUE", 0);
  m_Comms.Register ("TARGET_FREQ", 0);
  m_Comms.Register ("TARGET_BW", 0);
  m_Comms.Register ("TARGET_SPL", 0);
  m_Comms.Register ("NAV_X", 1);
  m_Comms.Register ("NAV_Y", 1);
  return true;
}

//***************************************************************************
//*  CSimTargets::OnNewMail()                                                 *
//*                                                                         *
//*  Called by the MOOSApp base class when a variable this app is subscribed*
//*  to changes.                                                            *
//*                                                                         *
//***************************************************************************/

bool CSimTargets::OnNewMail(MOOSMSG_LIST &NewMail)
{
 CMOOSMsg Msg;
 
  MOOSMSG_LIST::reverse_iterator p;
  for(p = NewMail.rbegin(); p != NewMail.rend(); p++) {
    CMOOSMsg &Msg = *p;
    

    if(Msg.m_sKey == "TARGET_CUE")
      {	
	if (!source_control)
	  {
	    //MOOSTrace("Got TARGET_CUE message");
	    string val;
	    val = Msg.m_sVal;

	    parse_tgt_stat(val, 
		   target_number,
		   target_x_start,
		   target_y_start,
		   target_depth,
		   target_heading,
		   target_speed,
		   target_utc,
		   target_freq,
		   target_bw,
		   target_spl,
		   target_str,
		   tone_freq,
		   tone_dbl);


	    //	    target_number+=1;
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
	      MOOSTrace("Target ID  ignored\n");

	    last_time = target_utc;
	    target_set = true;
	    m_Comms.Notify("TARGET_RESET","TRUE");
	  }
      }
    else if(Msg.m_sKey == "T_STATE")
      {	
	if (source_control)
	  {
	    MOOSTrace("Got T_STATE message");
	    string val;
	    val = Msg.m_sVal;

	    parse_tgt_stat(val, 
		   target_number,
		   target_x_start,
		   target_y_start,
		   target_depth,
		   target_heading,
		   target_speed,
		   target_utc,
		   target_freq,
		   target_bw,
		   target_spl,
		   target_str,
		   tone_freq,
		   tone_dbl);


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
	      MOOSTrace("Target ID  ignored\n");

	    last_time = target_utc;
	    target_set = true;
	    m_Comms.Notify("TARGET_RESET","TRUE");
	  }
      }
     else if(Msg.m_sKey == "NAV_X")
      {	
	auv_x = Msg.m_dfVal;
	got_x =true;
      }
    else if(Msg.m_sKey == "NAV_Y")
      {	
	auv_y = Msg.m_dfVal;
	got_y = true;
      }
    else if(Msg.m_sKey == "TARGET_CONTROL")
      {	
	string source = Msg.m_sVal;
	source_control = MOOSStrCmp(source,"ON");
	if (source_control)
	  {
	    m_Comms.Register ("T_STATE", 0);
	  }
      }
    else
      MOOSTrace("Unrecognized command: [%s]\n",Msg.m_sKey.c_str());  
  }
    return true;
}

//***************************************************************************
//*  CSimTargets::Iterate()                                                  *
//*                                                                         *
//*  Called by the MOOSApp base class at a fixed rate specified in the      *
//*  .moos file. Calls to this method trigger processing of data that is    *
//*  ready to be processed.                                                  *
//****************************************************************************/
bool CSimTargets::Iterate()
{
  //MOOSTrace("Pros is %i \n", prosecuting);
  //MOOSTrace("Targ is %i \n", target_set);

      if (target_init && got_x && got_y)
	{
	  target_utc = MOOSTime();
	  t_num += 1;
	  target_number = t_num;
	  updateTargetList();
	  postTargetList();
	  
	  last_time = target_utc;
	  target_set = true;
	  m_Comms.Notify("TARGET_RESET","TRUE");
	  target_init = false;
	}

      if(target_set == true)
	{
	  //MOOSTrace("Targ Set is True\n");
	  //Calculate target position and range
	  
	  for (int i=0; i< Targets.size(); i++)
	    {
	      time_now = MOOSTime();
	      elapsed = time_now - Targets[i].target_utc;
	      target_x = Targets[i].target_x+(sin(Targets[i].target_hdg*M_PI/180.0)*elapsed*Targets[i].target_spd);
	      target_y = Targets[i].target_y+(cos(Targets[i].target_hdg*M_PI/180.0)*elapsed*Targets[i].target_spd);
	      source_range = sqrt( (auv_x-target_x)*(auv_x-target_x) + (auv_y-target_y)*(auv_y-target_y) );
	      MOOSTrace("Target %s Range = %f\n", Targets[i].target_id().c_str(), source_range);
	      MOOSTrace("TargetX = %f    TargetY = %f\n", target_x, target_y);
	      string tstr = Targets[i].target_id();
	      // 2010-12-14 Stephanie Kemna:
	      //   adding bool noRangeCrit in case you don't want a critical
	      //   range (multiple vehicles, one pSimTargets)
	      if (source_range < rangeCrit || noRangeCrit)
		{
		  Targets[i].target_on = true;
		  m_Comms.Notify(tstr+"_SIM_STATE",Targets[i].target_state());
		}
	      else
		{
		  Targets[i].target_on = false;
		  m_Comms.Notify(tstr+"_SIM_STATE",Targets[i].target_state());
		  cancelTarget(tstr);
		  postTargetList();
		}

	    }
	  
	}
}
//***************************************************************************
//*  CSimTargets::OnStartUp()                                                 *
//*                                                                         *
//*  Called by the MOOSApp base class when tis application is started       *
//*                                                                         *
//***************************************************************************

bool CSimTargets::OnStartUp()
{   
  prosecuting = false;	
  target_set = false;
  
  if (!m_MissionReader.GetConfigurationParam("rangeCrit",rangeCrit))
    {
      MOOSDebugWrite("No rangeCrit in mission file - setting noRangeCrit to true, target always alive once launched\n");
      noRangeCrit = true;
    }
  
  string tgt_flag;
  if (!m_MissionReader.GetConfigurationParam("target_launch",tgt_flag))
      return true;
  else if ( !MOOSStrCmp(tgt_flag,"true") )
    return true;
  else
    {
      target_init = true;

      if (!m_MissionReader.GetConfigurationParam("tgt_x",target_x_start) )
	{
	  MOOSDebugWrite("No tgt_x in mission file - FAIL\n");
	  return false;
	}
      
      if (!m_MissionReader.GetConfigurationParam("tgt_y",target_y_start) )
	{
	  MOOSDebugWrite("No tgt_y in mission file - FAIL\n");
	  return false;
	}
  
      if (!m_MissionReader.GetConfigurationParam("tgt_speed",target_speed) )
	{
	  MOOSDebugWrite("No tgt_speed in mission file - FAIL\n");
	  return false;
	}

      if (!m_MissionReader.GetConfigurationParam("tgt_heading",target_heading) )
	{
	  MOOSDebugWrite("No tgt_heading in mission file - FAIL\n");
	  return false;
	}

      if (!m_MissionReader.GetConfigurationParam("tgt_depth",target_depth) )
	{
	  MOOSDebugWrite("No tgt_depth in mission file - FAIL\n");
	  return false;
	}

      if (!m_MissionReader.GetConfigurationParam("tgt_freq",target_freq) )
	{
	  MOOSDebugWrite("No tgt_freq in mission file - FAIL\n");
	  return false;
	}

      if (!m_MissionReader.GetConfigurationParam("tgt_bw",target_bw) )
	{
	  MOOSDebugWrite("No tgt_bw in mission file - FAIL\n");
	  return false;
	}

      if (!m_MissionReader.GetConfigurationParam("tgt_spl",target_spl) )
	{
	  MOOSDebugWrite("No tgt_spl in mission file - FAIL\n");
	  return false;
	}


      double dfVal;

      if (!m_MissionReader.GetConfigurationParam("number_tones",dfVal) )
	{
	  MOOSDebugWrite("No number_tones in mission file - FAIL\n");
	  return false;
	}
      int number_tones = int(dfVal);

      for (int i = 1; i<=number_tones; i++)
	{
	  if (!m_MissionReader.GetConfigurationParam("tone_freq_"+intToString(i),dfVal) )
	    {
	      MOOSTrace("No tone_freq_%d in mission file - FAIL\n",i);
	      return false;
	    }
	  else
	    tone_freq.push_back(dfVal);
	  
	  if (!m_MissionReader.GetConfigurationParam("tone_dbl_" + intToString(i),dfVal) )
	    {
	      MOOSTrace("No tone_dbl_%d in mission file - FAIL\n");
	      return false;
	    }
	  else
	    tone_dbl.push_back(dfVal);
	}

    }
  
  return true;
}


//-----------------------------------------------------------------
// Procedure: updateTargetList
//   Purpose: Maintain a list of known unique target names, and the
//            times they were last written to.
//   HS 032108 Modified to full target nav info and UTC of report to allow inheritance
//             and extrapolation of unreported varibles in NaFCon messages 

void CSimTargets::updateTargetList()
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
	    MOOSTrace("Existing Target %d, Time = %f, Speed = %f, Heading = %f \n",
		      i+1,target_utc,target_speed,target_heading);
	    Targets[i].printTargetValues();
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
    Targets[Targets.size()-1].printTargetValues();

    MOOSTrace("New Target %d, Time = %f, Speed = %f, Heading = %f \n",
              Targets.size(),target_utc,target_speed,target_heading);
}

void CSimTargets::cancelTarget(string target_name)
{
    target_name = toupper(stripBlankEnds(target_name));

    double moos_time = MOOSTime();
    MOOSTrace("Entering cancelTarget, target %s\n",target_name.c_str());
    
    for(int j=0; j<Targets.size(); j++) 
      {
        if(Targets[j].target_id() == target_name)
	  { 
	    MOOSTrace("Removing target %s\n",target_name.c_str());
	    for (int i=j; i<Targets.size()-1; i++)
	      {
		MOOSTrace("Shifting target %s\n", Targets[i+1].target_id().c_str());
		Targets[i] = Targets[i+1];
	      }	      
	    MOOSTrace("Popping target %s\n", Targets[Targets.size()-1].target_id().c_str());
	    Targets.pop_back();

	    return;
	  }
      }
}



//-----------------------------------------------------------------
// Procedure: postTargetList
//   Purpose: 

void CSimTargets::postTargetList()
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

    m_Comms.Notify("TARGET_SIM_LIST", target_list_string);


}


