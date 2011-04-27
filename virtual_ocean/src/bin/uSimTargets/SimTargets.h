#ifndef SIMTARGETS_HEADER
#define SIMTARGETS_HEADER


//#include "lamss/lib_newmat10D/newmatap.h"
//#include <sys/time.h>
//#include <deque>

#include "MOOSLIB/MOOSLib.h"
#include "MBUtils.h"
#include "MOOSGenLib/MOOSGenLib.h"
#include <math.h>
#include <time.h>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string.h>
#include <vector>

// from lib_henrik_util
#include <lamss/lib_henrik_util/CTarget.h>
#include <lamss/lib_henrik_util/parse_stat.h>
#include <lamss/lib_henrik_util/str_num.h>

using namespace std;



class CSimTargets : public CMOOSApp
{
 public:

  CSimTargets();
  virtual ~CSimTargets();

  bool OnNewMail(MOOSMSG_LIST &NewMail);
  bool Iterate();
  bool OnConnectToServer();
  bool OnStartUp();

 protected:

  void updateTargetList();
  void cancelTarget(std::string);
  void postTargetList();

  bool prosecuting,target_set;
  bool target_init;

  double target_utc,target_x,target_y,target_heading,target_speed;
  double time_now,auv_x,auv_y,target_depth,last_time,current_time;
  double elapsed, source_range, rangeCrit, target_x_start, target_y_start;
  int deploy_mission,prosecute_mode, target_number, t_num; 
  bool source_control;

  double target_freq;
  double target_spl;
  double target_bw;
  double dest_id;
  vector<double> tone_freq;
  vector<double> tone_dbl;

  bool got_x;
  bool got_y;
  bool noRangeCrit;

  std::vector<CTarget> Targets;


};

#endif 
