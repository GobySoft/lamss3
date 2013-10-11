#ifndef __MEASUREMENT_h__
#define __MEASUREMENT_h__

#include "MOOS/libMOOS/MOOSLib.h"
#include "MBUtils.h"
#include "MOOS/libMOOS/Utils/MOOSUtils.h"

#include <math.h>
#include <string.h>
#include <string>
 
class Measurement
{

 public:

  Measurement();
  ~Measurement();

  int node;
  int  state;
  int  beam;
  double bearing;
  double true_bearing;
  double sigma;
  double Meas_Time;

  double auv_x;
  double auv_y;
};

bool parse_measurement(std::string,Measurement*);

bool parse_brng_stat(std::string status,
		  int & node,
		  int & state,
		  double & bearing,
		  double & sigma,
		  double & xp,
		  double & yp,
		  double & time
		  );

#endif /* __MEASUREMENT_h__ */
