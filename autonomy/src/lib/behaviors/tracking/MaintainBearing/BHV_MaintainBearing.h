/***********************************************************/
/*    NAME: Henrik Schmidt                                 */
/*    FILE: MaintainBearing.h                            */
/***********************************************************/
 
#ifndef BHV_MAINTAINBEARING_HEADER
#define BHV_MAINTAINBEARING_HEADER

#include <string>

#include "IvPBehavior.h"
#include "lamss/lib_henrik_util/CVectorUtils.h"

//using namespace std;

class IvPDomain;

class BHV_MaintainBearing : public IvPBehavior {
public:
  BHV_MaintainBearing(IvPDomain);
  ~BHV_MaintainBearing() {};
  
  IvPFunction*  onRunState();
  void          onIdleState();
  bool setParam(std::string, std::string);
  double getRelevance();

protected:
  
  int decode(std::string);

  bool new_run;
  bool alternate;
  double curr_time;
  double last_switch;
  double alternate_interval;
  double bearing_rate_threshold;
  std::vector<double> bearings;
  std::vector<double> times;

  bool starboard;
  double over_shoot;
  double desired_angle,true_bearing;
  int    width,pwidth;
  int bearing_state;
  std::string heading_variable;
  std::string bearing_variable;
  std::string bearing_type;
  bool absolute_bearing;
  std::string bearing_mode;

  CmyVectorUtils vec;
};

extern "C" {
  IvPBehavior * createBehavior(std::string name, IvPDomain domain)
  {return new BHV_MaintainBearing(domain);}
}

#endif

