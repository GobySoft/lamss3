// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// Author: Raymond Lum
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

#ifndef uSimCTD_HEADER
#define uSimCTD_HEADER

#define STRLEN         200

#include "MOOSLIB/MOOSLib.h"
#include "MOOSGenLib/MOOSGenLib.h"

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

class CuSimCTD : public CMOOSApp
{
 public:
  CuSimCTD();
  virtual ~CuSimCTD();

  bool Iterate();
  bool OnNewMail(MOOSMSG_LIST &NewMail);
  bool OnConnectToServer();
  bool OnStartUp();
  
 protected:
  
  double dfC;
  double dfT;
  double dfS;
  
  double dfLat;
  double dfLong;
  double dfDepth;
  
  const  char *sMods_Bin_File;
  FILE   *fsMods_Bin_File;
  double **dfMods_DB;
  
  int    NDataPoint;

  char   GetCtsfrLatLong();
  
};

#endif
