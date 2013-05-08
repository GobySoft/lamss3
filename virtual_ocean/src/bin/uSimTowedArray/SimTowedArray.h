#include <iostream>
#include <string>
#include <vector>
#include <math.h>
#include "MOOSLIB/MOOSLib.h"
#include "MOOSGenLib/MOOSGenLib.h"
#include "MOOSUtilityLib/MOOSGeodesy.h"

// From lib_henrik_util
#include <lamss/lib_henrik_util/CVectorUtils.h>
#include <lamss/lib_henrik_util/str_num.h>


#ifndef SIMTOWEDARRAY_HEADER
#define SIMTOWEDARRAY_HEADER

class CSector  
{
 public:
  //  CSector();
  //  ~CSector();

  int    Nsub;
  double len;
  double wgt;
  double dia;
  double td;
  double nd;
  double modEla;

  bool setSectorValues(std::vector<double>& values);
 
};

class CTowbody
{
  public:
  double wgt;
  double drgcoef;
  double area;

  bool setTowbodyValues(std::vector<double>& values);
};

class CmyAngleUtils  
{
 public:
  double angle360(double angle);
};


class CArrayModel  
{
 public:
  CArrayModel();
  ~CArrayModel();

  int indx(int n_rows, int row, int col);

  bool straight_array(double xtowp, double ytowp, double ztowp, 
		      double vx, double vy, double vz, 
		      double nav_yaw, 
		      double& ael_heading, double& ael_pitch);

  bool volumetric_array(double xtowp, double ytowp, double ztowp, 
			double vx, double vy, double vz,
			std::vector<double> local_x,
			std::vector<double> local_y,
			std::vector<double> local_z,
			double nav_yaw, 
			double& ael_heading, double& ael_pitch);

  bool function_steady(double xtowp, double ytowp, double ztowp, double vx, double vy, double vz);

  bool cable_disc();

  bool steady_state(double towvel, double radheadstart);

  bool poly_curr_grad_ith_auv(int ith_order );

  bool function_cable(int& nt, bool Array_init, double xtowp, double ytowp, double ztowp, double vx, double vy, double vz);

  bool dyssdt(double sd,  std::vector<double> yss, std::vector<double>& dydt );

  bool cable_ode45(std::vector<double> x, std::vector<double> y_init, std::vector<double>& y);

  bool find_configf(std::vector<double> yold, std::vector<double> y, int nt, double vx, double vy, double vz, std::vector<double> cx, std::vector<double> cy, std::vector<double> gx, std::vector<double> gy, std::vector<double> mass, std::vector<double> mass1, std::vector<double> ext, std::vector<double> dia, std::vector<double> tandrag, std::vector<double> nordrag, std::vector<double> weight, std::vector<double> dels);

  // Array data
  int count;
  int m_nelm;
  int array_section_number;
  double first_phone_offset;
 
  int dtfilt_length;
  std::vector<double> dt_hist;
  std::vector<double> phone_position;
  std::vector<double> phone_offset;
  std::vector<double> compass_offset_full;
  std::vector<double> depth_offset_full;
  std::vector<double> pitch_offset_full;

  std::vector<double> array_head;
  std::vector<double> array_depth;
  std::vector<double> array_pitch;
  std::vector<double> array_tens;

  std::vector<double> xss1;
  std::vector<double> yss1;
  std::vector<double> zss1;

  std::vector<double> Jacob;
  std::vector<double> Jacobo;
  std::vector<double> InvJacob;


  std::vector<double> ynew;
  std::vector<double> old_ynew;
  std::vector<double> old_bigy;
  std::vector<double> old_bigyold;
  std::vector<double> old_bigynew;
  std::vector<double> old_InvJacob;
  std::vector<double> old_Jacobo;
 
  // The following are all the strings with array geometry to be published in MOOSDB

  std::string arr_heading;
  std::string arr_pitch;
  std::string ac_array_x;
  std::string ac_array_y;
  std::string ac_array_z;
  std::string x_array;
  std::string y_array;
  std::string z_array;

  double ael_x_offset;
  double ael_y_offset;
  double ael_x;
  double ael_y;

  std::string temp1;
  std::string temp2;


  std::vector<double> ycold;
  std::vector<double> yc;

  int Nseg;
  double rho;
  double g;
  int prob;

  std::vector<double> dia;
  std::vector<double> tandrag;
  std::vector<double> nordrag;
  std::vector<double> weight;
  std::vector<double> dels;
  std::vector<double> mass;
  std::vector<double> mass1;
  std::vector<double> ext;

  std::vector<double> depth;
  std::vector<double> cx;
  std::vector<double> cy;
  std::vector<double> cxc;
  std::vector<double> cyc;
  std::vector<double> gxc;
  std::vector<double> gyc;

  bool towbody;
  double towvel;
  double area;
  double dragcoef;
  double wgt;
  double tbmass;
  double vol;
  double array_tension;

  std::vector<double> pcx;
  std::vector<double> pcy;
  std::vector<double> qgx;
  std::vector<double> qgy;
  std::vector<double> dmu;
  std::vector<double> currtim;

  double over_extrap;
  double offset_to_array;
  double var_sus;
  double deltim;
  double time;
  double old_time;
  std::vector<double> sr;
  double offset_full_array;

  // local  vectors 

  std::vector<int> Nsub;
  std::vector<int> iendsect;
  std::vector<double> lngth;
  std::vector<double> dians;
  std::vector<double> tandns;
  std::vector<double> nordns;
  std::vector<double> wgtns;
  std::vector<double> elasns;
  std::vector<double> ss;
  std::vector<double> s;
  std::vector<double> cablel;

  int Nsect;

  std::vector<CSector> Sectors;

  CTowbody Towbody;

  CmyVectorUtils vecUtils;
  CmyAngleUtils angleUtils;



};

class CSimTowedArray : public CMOOSApp
{
 public:
  CSimTowedArray();
  virtual ~CSimTowedArray();

  bool Iterate();
  bool OnNewMail(MOOSMSG_LIST &NewMail);
  bool OnConnectToServer();
  bool OnStartUp();

  
 protected:

  CMOOSGeodesy m_Geodesy;

  bool ready;
  bool first, towing;
  bool got_dir, got_frame;
  double towing_speed;
  bool Array_init;
  int filter_length;
  int cond_check;
  double start_speed;
  double start_time;
  int nt;

  int nelm;
  std::vector<double> local_x;
  std::vector<double> local_y;
  std::vector<double> local_z;
  bool ok_local_x;
  bool ok_local_y;
  bool ok_local_z;
  bool ok_local;

  double history_time;
  double prev_time;
  double iterate_time;

  std::string veh_name;
  std::string  array_type;

  std::vector<double> m_buffer;
  std::vector<double> m_dummy;


  double filt_tension;
  double cable_tension;


  int filter_v;
  double B;
  int hist_length;

  std::vector<double> xtowp_hist;
  std::vector<double> ytowp_hist;
  std::vector<double> tfilter;

  std::vector<double> vxfilter;
  std::vector<double> vyfilter;
  std::vector<double> vzfilter;

  std::vector<double> vx_hist;
  std::vector<double> vy_hist;
  std::vector<double> vz_hist;

  int pp;
  int get_back;

  double nav_x;
  double nav_y;
  double nav_z;
  double nav_yaw;
  double nav_pitch;
  double nav_spd;

  double tow_off_x;
  double tow_off_y;
  double tow_off_z;

  double tow_pos_x;
  double tow_pos_y;
  double tow_pos_z;
  double tow_vel_x;
  double tow_vel_y;
  double tow_vel_z;

  bool ok_x;
  bool ok_y;
  bool ok_z;
  bool ok_spd;
  bool ok_yaw;
  bool ok_pitch;
  bool got_spd;
  bool got_yaw; 
  bool got_pitch;
 
  double ael_heading;
  double ael_pitch;

  int array_definition_type;

  CArrayModel Array;

  CmyVectorUtils vecUtils;
  CmyAngleUtils angleUtils;

  std::vector<double> bod;
};

#endif 

