/*****************************************************************/
/*    NAME: Henrik Schmidt                                       */
/*    ORGN: MIT Cambridge MA               */
/*    FILE: BHV_ConstantBuoyancy.cpp                                 */
/*    DATE: July 10, 2020                                         */
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

#ifdef _WIN32
#pragma warning(disable : 4786)
#pragma warning(disable : 4503)
#endif
#include <math.h> 
#include <stdlib.h>
#include "BHV_ConstantBuoyancy.h"
#include "MBUtils.h"
#include "BuildUtils.h"
#include "ZAIC_PEAK.h"
#include "OF_Coupler.h"

using namespace std;

//-----------------------------------------------------------
// Procedure: Constructor

BHV_ConstantBuoyancy::BHV_ConstantBuoyancy(IvPDomain gdomain) : 
  IvPBehavior(gdomain)
{
  this->setParam("descriptor", "(d)bhv_buoyancy");

  // Behavior Parameter Default Values:
  buoyancy = 0;

  // Behavior State Variable Initial Values:

  m_domain = subDomain(m_domain, "buoyancy");

  // Declare the variables we will need from the info_buffer
  addInfoVars("NAV_DEPTH");
  addInfoVars("NAV_ALTITUDE");

}

//-----------------------------------------------------------
// Procedure: onIdleState
//      Note: This function overrides the onIdleState() virtual
//            function defined for the IvPBehavior superclass
//            This function will be executed by the helm each
//            time the behavior FAILS to meet its run conditions.

void BHV_ConstantBuoyancy::onIdleState()
{
  // Do your thing here
   }


//-----------------------------------------------------------
// Procedure: setParam

bool BHV_ConstantBuoyancy::setParam(string g_param, string g_val) 
{
  if(IvPBehavior::setParamCommon(g_param, g_val))
    return(true);

  g_val = stripBlankEnds(g_val);

  if(g_param == "buoyancy") 
    {
      double dval = atof(g_val.c_str());
      if((!isNumber(g_val)))
	return(false);
      buoyancy = dval;
      return(true);
    }
}

//-----------------------------------------------------------
// Procedure: onRunState

IvPFunction *BHV_ConstantBuoyancy::onRunState() 
{
  double curr_time = getBufferCurrTime();
 
  bool   ok,ok1,ok2;
  double curr_depth = getBufferDoubleVal("NAV_DEPTH", ok);
  if(!ok)
    {
      postEMessage("No ownship NAV_DEPTH info in the helm info_buffer");
      postMessage("CONST_BUOY_STATUS", "Missing NAV_DEPTH info");
      return(0);
    }
  double curr_altitude = getBufferDoubleVal("NAV_ALTITUDE", ok1);
  if(!ok1)
    {
      postEMessage("No ownship NAV_ALTITUDE info in the helm info_buffer");
      curr_altitude = 0;
    }

  double peak_width = 0.1*fabs(buoyancy);
  ZAIC_PEAK zaic_buoyancy(m_domain, "buoyancy");
  zaic_buoyancy.setSummit(buoyancy);
  zaic_buoyancy.setPeakWidth(peak_width);
  zaic_buoyancy.setBaseWidth(fabs(buoyancy)-peak_width);
  zaic_buoyancy.setSummitDelta(80.0);
  zaic_buoyancy.setMinMaxUtil(0, 100);

  IvPFunction *ipf_buoyancy = zaic_buoyancy.extractOF();

  ipf_buoyancy->setPWT(m_priority_wt);

  return(ipf_buoyancy);
}

