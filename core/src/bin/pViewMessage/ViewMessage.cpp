/*****************************************************************/
/*    NAME: Henrik Schmidt                                       */
/*    ORGN: MIT Cambridge MA                                     */
/*    FILE: ViewMessage.cpp                                 */
/*    DATE: Aug. 2, 2008                                         */
/*                                                               */
/* This program is free software; you can redistribute it and/or */
/* modify it under the terms of the GNU General Public License   */
/* as published by the Free Software Foundation; either version  */
/* 2 of the License, or (at your option) any later version.      */
/*                                                               */
/* This program is distributed in the hope that it will be       */
/* useful, but WITHOUT ANY WARRANTY; without even the implied    */
/* warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR       */
/* PURPOSE. See the GNU General Public License for more details. */
/*                                                               */
/* You should have received a copy of the GNU General Public     */
/* License along with this program; if not, write to the Free    */
/* Software Foundation, Inc., 59 Temple Place - Suite 330,       */
/* Boston, MA 02111-1307, USA.                                   */
/*****************************************************************/

#include "ViewMessage.h"

#include <iostream>
#include <sstream>
#include <math.h>

using namespace std;


CViewMessage::CViewMessage()
{
  new_point = false;
  new_poly = false;
  new_segl = false;
  x.clear();
  y.clear();
  ownship = "unknown";
  mission_state = "unknown";
  old_state = "unknown";
  m_pnt_x.clear();
  m_pnt_y.clear();
  m_pnt_valid.clear();
  m_pnt_type.clear();
  m_poly_max = 9;
}

CViewMessage::~CViewMessage()
{
}

bool CViewMessage::OnStartUp()
{
  
  //Initialize m_Geodesy from lat lon origin in .moos file
  string sVal;
  double dfLatOrigin;
  double dfLongOrigin;
  
  if(m_MissionReader.GetValue("LatOrigin",sVal))
    {
      dfLatOrigin = atof(sVal.c_str());
      //MOOSTrace("  LatOrigin  = %10.5f deg.\n",dfLatOrigin);
    }
  else
    {
      MOOSTrace("LatOrigin not specified in mission file - FAIL\n");
      return false;
    }
  
  if(m_MissionReader.GetValue("LongOrigin",sVal))
    {
      dfLongOrigin = atof(sVal.c_str());
      //MOOSTrace("  LongOrigin = %10.5f deg.\n",dfLongOrigin);
    }
  else
    {
      MOOSTrace("LongOrigin not specified in mission file - FAIL\n");
      return false;
    }
  if(!m_Geodesy.Initialise(dfLatOrigin,dfLongOrigin))
    {
      MOOSTrace("Geodesy Init inside pTargetSim failed - FAIL\n");
      return false;
    }
  //Done initializaing m_Geodesy
  
  return(true);
}

bool CViewMessage::OnConnectToServer()
{
  m_Comms.Register( "VIEW_POINT",0);
  m_Comms.Register( "VIEW_POLYGON",0);
  m_Comms.Register( "VIEW_SEGLIST",0);
  m_Comms.Register( "VEHICLE_NAME",0);
  m_Comms.Register( "MISSION_STATE",0);
 
  return(true);
}

bool CViewMessage::Iterate()
{
  // Now generate message format
  string view_msg = "";
  if (new_point)
    {
      for (int i=0; i < m_pnt_type.size(); i++)
	{
	  if (i > 0)
	    view_msg = view_msg + ",";

	  if (m_pnt_valid[i])
	      pt_active = "true";
	  else
	      pt_active = "false";

	  view_msg = view_msg + "pt_active_" + intToString(i+1) + "=" + pt_active +","
	    + "pt_type_" + intToString(i+1) + "=" + m_pnt_type[i] + ","
	    + "pt_x_" + intToString(i+1) + "=" + doubleToString(m_pnt_x[i]) + ","
	    + "pt_y_" + intToString(i+1) + "=" + doubleToString(m_pnt_y[i]);
	}
      MOOSTrace("point message created:" + view_msg + "\n\n");
      m_Comms.Notify("VIEW_POINT_OUT",view_msg);
      
      new_point = false;
    }
  
  if (new_poly)
    {
      view_msg = "active=" + poly_active + ","
	+ "mission=" + mission_state + ","
	+ "edge_size=" + doubleToString(edge_size,0) + ","           
	+ "vertex_size=" + doubleToString(vertex_size,0);
      for (int i=0;i<x.size();i++)
	{
	  view_msg += ",x_" + intToString(i+1) + "=" + doubleToString(x[i],0) + ","
	    + "y_" + intToString(i+1) + "=" + doubleToString(y[i],0);
	}
      
      for (int i=x.size();i<m_poly_max;i++)
	{
	  view_msg += ",x_" + intToString(i+1) + "=" + doubleToString(x[0],0) + ","
	    + "y_" + intToString(i+1) + "=" + doubleToString(y[0],0);
	}
      
      MOOSTrace("polygon message created:" + view_msg + "\n\n");
      m_Comms.Notify("VIEW_POLYGON_OUT",view_msg);
      
      new_poly = false;
    }

  if (new_segl)
    {
      view_msg = "active=" + segl_active + ","
	+ "mission=" + mission_state;
      int end = x_sl.size()-1;
      for (int i=0;i<x_sl.size();i++)
	{
	  view_msg += ",x_" + intToString(i+1) + "=" + doubleToString(x_sl[i],0) + ","
	    + "y_" + intToString(i+1) + "=" + doubleToString(y_sl[i],0);
	}
      
      for (int i=x_sl.size();i<m_poly_max;i++)
	{
	  view_msg += ",x_" + intToString(i+1) + "=" + doubleToString(x_sl[end],0) + ","
	    + "y_" + intToString(i+1) + "=" + doubleToString(y_sl[end],0);
	}
      
      MOOSTrace("seglist message created:" + view_msg + "\n\n");
      m_Comms.Notify("VIEW_SEGLIST_OUT",view_msg);
      
      new_segl = false;
    }

  return(true);
}


//-------------------------------------------------------------
// Procedure: OnNewMail

bool CViewMessage::OnNewMail(MOOSMSG_LIST &NewMail)


{
  MOOSMSG_LIST::iterator p;

  for(p = NewMail.begin(); p != NewMail.end(); p++) 
    {
      
      CMOOSMsg &Msg = *p;
      
      
      //local node ID
      if(Msg.m_sKey == "VEHICLE_NAME")
	{
	  ownship = tolower(Msg.m_sVal);
	}
      else if(Msg.m_sKey == "MISSION_STATE")
	{

	  string new_state = Msg.m_sVal;
	  kill_old = (!MOOSStrCmp(new_state,old_state));
	  mission_state = new_state;
	}
      else if(Msg.m_sKey == "VIEW_POINT")
	{
	  string view_point = Msg.m_sVal;
	  MOOSTrace("got VIEW_POINT\n");
	  MOOSTrace(view_point + "\n\n");

	  new_point = insertPoint(view_point);
	}
      else if(Msg.m_sKey == "VIEW_POLYGON")
	{
	  string view_poly = Msg.m_sVal;
	  MOOSTrace("got VIEW_POLYGON\n");
	  MOOSTrace(view_poly + "\n\n");
	  if (!new_poly)
	    new_poly = insertPolygon(view_poly);
	}
      else if(Msg.m_sKey == "VIEW_SEGLIST")
	{
	  string view_segl = Msg.m_sVal;
	  MOOSTrace("got VIEW_SEGLIST\n");
	  MOOSTrace(view_segl + "\n\n");
	  if (!new_segl)
	    new_segl = insertSeglist(view_segl);
	}
    }
  return(true);
}

bool CViewMessage::insertPoint(string view_point)
{
  string label_id = MOOSChomp(view_point,",");
  string str_active = "true";
  if (MOOSStrCmp(label_id,"active"))
    {
      str_active = MOOSChomp(view_point,":");
      label_id = MOOSChomp(view_point,",");
      active = MOOSStrCmp(str_active,"true");
    }
  else
    {
      active = true;
      str_active = "true";
    }

  pt_active = str_active;
  pt_label = MOOSChomp(view_point,":");

  //  MOOSTrace("insertPoint: active=" + str_active + ",label=" + pt_label +"\n");  

  string::size_type name_pos = pt_label.find(ownship);
  //  vehicle=tolower(MOOSChomp(pt_label,"_"));
  //      if (ownship == "unknown" || MOOSStrCmp(ownship,vehicle))
  if (ownship == "unknown" || name_pos != string::npos)
    {
      //	  MOOSTrace("Parsing point message\n");
      // then the type
      string type_id = MOOSChomp(view_point,",");
      pt_type = MOOSChomp(view_point,":");

      // then the source behavior
      string source_id = MOOSChomp(view_point,",");
      source = MOOSChomp(view_point,":");

      //      MOOSTrace("insertPoint: type=" + pt_type + ",source=" + source + "\n");  

      // finally the point
      string x_str = MOOSChomp(view_point,",");
      pt_x = atof(x_str.c_str());
      string y_str = MOOSChomp(view_point,":");
      pt_y = atof(y_str.c_str());

      // Check if it already exists
      // if not insert if active
      int index = -1;
      for (int i=0; i< m_pnt_type.size(); i++)
	{
	  if (MOOSStrCmp(m_pnt_type[i],pt_type))
	    index = i;
	}
      if (index < 0 && active)
	{
	  m_pnt_x.push_back(pt_x);
	  m_pnt_y.push_back(pt_y);
	  m_pnt_valid.push_back(active);
	  m_pnt_type.push_back(pt_type);
	  index = m_pnt_type.size()-1;
	}
      //      MOOSTrace("insertPoint: index=%d\n",index);  

      if (index >= 0)
	{
	  m_pnt_x[index]     = pt_x;
	  m_pnt_y[index]     = pt_y;
	  m_pnt_valid[index] = active;
	  m_pnt_type[index]  = pt_type;
	  return(true);
	}
      else
	return(false);
    }
  else
    return(false);
}

bool CViewMessage::deletePoint(string pt_type)
{
  // Check if it exists before trying to delete
  int index = -1;
  for (int i=0; i< m_pnt_type.size(); i++)
    {
      if (MOOSStrCmp(m_pnt_type[i],pt_type))
	index = i;
    }
  if (index >= 0)
    {
      for (int i=index+1; i< m_pnt_type.size(); i++)
	{
	  m_pnt_x[i-1] = m_pnt_x[i];
	  m_pnt_y[i-1] = m_pnt_y[i];
	  m_pnt_valid[i-1] = m_pnt_valid[i];
	  m_pnt_type[i-1] = m_pnt_type[i];
	}
      m_pnt_x.pop_back();
      m_pnt_y.pop_back();
      m_pnt_valid.pop_back();
      m_pnt_type.pop_back();
    }
  return(index>=0);
}


bool CViewMessage::insertPolygon(string view_poly)
{
  bool valid =false;
  // first the active flag and label
  string label_id = MOOSChomp(view_poly,",");
  string str_active = "true";
  
  if (MOOSStrCmp(label_id,"active"))
    {
      str_active = MOOSChomp(view_poly,":");
      label_id = MOOSChomp(view_poly,",");
      active = MOOSStrCmp(str_active,"true");
    }
  else
    {
      active = true;
      str_active = "true";
    }
  if (active)
    {
      poly_active = str_active;
      poly_label = MOOSChomp(view_poly,":");
      MOOSTrace("poly_label = " + poly_label + "\n\n");

      string::size_type name_pos = poly_label.find(ownship);
      //      vehicle=tolower(MOOSChomp(poly_label,"_"));
      //      if (ownship == "unknown" || MOOSStrCmp(ownship,vehicle))
      if (ownship == "unknown" || name_pos != string::npos)
	{
	  valid = true;
	  MOOSTrace("Parsing polygon message\n");
	  // then the edge_size
	  string edge_id = MOOSChomp(view_poly,",");
	  valid = valid && MOOSStrCmp(edge_id,"edge_size");
	  string num = MOOSChomp(view_poly,":");
	  MOOSTrace("insertPolygon: " + edge_id + "=" + num +"\n");
	  valid = valid && num != ""; 
	  if (valid)
	    {
	      edge_size = atof(num.c_str());
	      // then the vertex size
	      string vertex_id = MOOSChomp(view_poly,",");
	      num = MOOSChomp(view_poly,":");
	  MOOSTrace("insertPolygon: " + vertex_id + "=" + num + "\n");
	      valid = valid && num != ""; 
	      vertex_size = atof(num.c_str());
	      // finally the points
	      string point;
	      x.clear();
	      y.clear();
	      while (view_poly != "")
		{
		  string x_str = MOOSChomp(view_poly,",");
		  // valid = valid && x_str != ""; 
		  pt_x = atof(x_str.c_str());
		  x.push_back(pt_x);
		  string y_str = MOOSChomp(view_poly,":");
		  //  valid = valid && y_str != ""; 
		  pt_y = atof(y_str.c_str());
		  y.push_back(pt_y);
		}
	      MOOSTrace("insertPolygon: points= %d \n",x.size());
	      sampleSeglist(m_poly_max,x,y);

	      MOOSTrace("after sampling: points= %d \n",x.size());
	    }
	}
    }
  return(valid);
}


bool CViewMessage::insertSeglist(string view_segl)
{
  bool new_segl = false;
  // first the active flag and label
  string label_id = MOOSChomp(view_segl,",");
  string str_active = "true";
  
  if (MOOSStrCmp(label_id,"active"))
    {
      str_active = MOOSChomp(view_segl,":");
      label_id = MOOSChomp(view_segl,",");
      active = MOOSStrCmp(str_active,"true");
    }
  else
    {
      active = true;
      str_active = "true";
    }
  
  if (active)
    {
      segl_active = str_active;
      segl_label = MOOSChomp(view_segl,":");
      
      string::size_type name_pos = segl_label.find(ownship);
      //      vehicle=tolower(MOOSChomp(segl_label,"_"));
      //      if (ownship == "unknown" || MOOSStrCmp(ownship,vehicle))
      if (ownship == "unknown" || name_pos != string::npos)
	{
	  new_segl = true;
	  MOOSTrace("Parsing seglist message\n");
	  // finally the points
	  string point;
	  x_sl.clear();
	  y_sl.clear();
	  while (view_segl != "")
	    {
	      string x_str = MOOSChomp(view_segl,",");
	      new_segl = new_segl && x_str != ""; 
	      pt_x = atof(x_str.c_str());
	      x_sl.push_back(pt_x);
	      string y_str = MOOSChomp(view_segl,":");
	      new_segl = new_segl && y_str != ""; 
	      pt_y = atof(y_str.c_str());
	      y_sl.push_back(pt_y);
	    }
	}
    }
  return(new_segl);
}

bool CViewMessage::sampleSeglist(int n_pnt, vector<double>& x,vector<double>& y)
{
  vector<double> buf_x ;
  vector<double> buf_y ;

  if (x.size() > n_pnt)
    {
      int n_new = x.size();
      int red_fac = 1;
      while (n_new > n_pnt)
	{
	  red_fac += 1;
	  n_new = (int) x.size()/red_fac;
	}

      buf_x.clear();
      buf_y.clear();
      for (int i=0 ; i < n_new; i++)
	{
	  if (i*red_fac < x.size())
	    {
	      buf_x.push_back(x[i*red_fac]);
	      buf_y.push_back(y[i*red_fac]);
	    }
	}
      x = buf_x;
      y = buf_y;
    }
  return(true);
}

