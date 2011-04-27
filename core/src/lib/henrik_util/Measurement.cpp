#include "Measurement.h"

using namespace std;

//
// Measurement methods and functions
//

/******************************************************************************
 *                                                                            *
 *  Method: Measurement::Measurement()                                        *
 *                                                                            *
 *  Description: The constructor for the Measurement class                    * 
 *                                                                            * 
 ******************************************************************************/
Measurement::Measurement()
{
}

/******************************************************************************
 *                                                                            *
 *  Method: Measurement::~Measurement()                                       *
 *                                                                            *
 *  Description: The destructor for the Measurement class                     * 
 *                                                                            * 
 ******************************************************************************/
Measurement::~Measurement()
{
}

bool parse_measurement(string val, Measurement* MStruct)
{

  string bite;
  //MOOSTrace("string = %s\n",val.c_str());

  MOOSValFromString(bite,val,"node");
  MStruct->node = atoi(bite.c_str());
  MOOSValFromString(bite,val,"state");
  MStruct->state = atoi(bite.c_str());
  MOOSValFromString(MStruct->bearing,val,"bearing");
  MOOSValFromString(MStruct->sigma,val,"sigma");
  MOOSValFromString(bite,val,"beamno");
  MStruct->beam = atoi(bite.c_str());
  MOOSValFromString(MStruct->auv_x,val,"xp");
  MOOSValFromString(MStruct->auv_y,val,"yp");
  MOOSValFromString(MStruct->Meas_Time,val,"time");

  double true_angle = (90.0-MStruct->bearing)*M_PI/180.0;
  if (true_angle <= -M_PI) 
    true_angle = true_angle + 2* M_PI;
  if (true_angle > M_PI) 
    true_angle = true_angle - 2* M_PI;
  MStruct->true_bearing = true_angle;

  return(true);
}

bool parse_brng_stat(std::string val,
		  int & node,
		  int & state,
		  double & bearing,
		  double & sigma,
		  double & xp,
		  double & yp,
		  double & time
)
{

  string bite;
  //MOOSTrace("string = %s\n",status.c_str());

  MOOSValFromString(bite,val,"node");
  node = atoi(bite.c_str());
  MOOSValFromString(bite,val,"state");
  state = atoi(bite.c_str());
  MOOSValFromString(bearing,val,"bearing");
  MOOSValFromString(sigma,val,"sigma");
  MOOSValFromString(xp,val,"xp");
  MOOSValFromString(yp,val,"yp");
  MOOSValFromString(time,val,"time");

  return(true);
}

