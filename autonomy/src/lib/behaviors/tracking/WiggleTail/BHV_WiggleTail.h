/***********************************************************/
/*    Author: Don Eickstedt                                */
/*    FILE:   BHV_WiggleTail.h                             */
/*    DATE:   18. March 2011                               */
/*    MODIFIED: 20. feb. 2007, 18. March 2011              */
/***********************************************************/
 
#ifndef BHV_WIGGLETAIL_HEADER
#define BHV_WIGGLETAIL_HEADER

#include "IvPBehavior.h"

#define NO_TRACK 0
#define LR_TRACK 1
#define TRACKING 2

using namespace std;

class IvPDomain;
class BHV_WiggleTail : public IvPBehavior {
public:
  BHV_WiggleTail(IvPDomain);
  ~BHV_WiggleTail() {};
  
  IvPFunction*  onRunState();
  void          onIdleState();
  bool          setParam(std::string, std::string);

protected:
  
  int decode(string);
  //double opt_course(double,double);

  double course_fix,course_new,course_lr_init,turn_angle;
  double heading,speed,ty,tx,time;
  int    state;
  bool course_fixed, first_time;
  int current_state,new_state,next_turn,curr_turn;

  std::string heading_variable;

};
extern "C" {
  IvPBehavior * createBehavior(std::string name, IvPDomain domain)
  {return new BHV_WiggleTail(domain);}
}

#endif

