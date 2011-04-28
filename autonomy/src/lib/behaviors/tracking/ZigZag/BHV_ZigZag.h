/*****************************************************************/
/*    NAME: Henrik Schmidt                                       */
/*    ORGN: MIT Cambridge MA                                     */
/*    FILE: BHV_ZigZag.h                                   */
/*    DATE: Mar 3, 2007                                          */
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
 
#ifndef BHV_ZIGZAG_HEADER
#define BHV_ZIGZAG_HEADER

#include <string>
#include "IvPBehavior.h"

class IvPDomain;
class BHV_ZigZag : public IvPBehavior {
public:
  BHV_ZigZag(IvPDomain);
  ~BHV_ZigZag() {};
  
  bool         setParam(std::string, std::string);
  IvPFunction* onRunState();
  void         onIdleState();

protected:

  double       currentCompensatedHeading(double desired_current, double water_speed);

private: // Behavior Parameters
  std::string zigzag_function;
  double      period,distance_traveled,heading,amplitude,start_x,start_y;
  double head_x,head_y,new_heading,speed;

  double       east_current_spd;
  double       north_current_spd;
  bool         m_current_compensator;
  bool        first_iteration;
};

extern "C" {
  IvPBehavior * createBehavior(std::string name, IvPDomain domain)
  {return new BHV_ZigZag(domain);}
}

#endif
 
