// *****************************************************************************
// Author: Raymond Lum
// Date:   Aug 3, 2011
// *****************************************************************************

#ifndef uSimCTD_HEADER
#define uSimCTD_HEADER

#define LARGEVAL 1000000

#include "MOOSLIB/MOOSLib.h"
#include "MOOSGenLib/MOOSGenLib.h"

//#include <string>
//#include <iostream>
//#include <fstream>
//#include <sstream>

#include <math.h>
//#include <stdio.h>
//#include <stdlib.h>

using namespace std;

class CuSimCTD : public CMOOSApp
{
 
 public:
  
  CuSimCTD();
  virtual ~CuSimCTD();
  
  bool OnConnectToServer();
  bool OnStartUp();
  bool OnNewMail(MOOSMSG_LIST &NewMail);
  bool Iterate();
  
 protected:
  
  string   sCTD_Txt_File;
  string   sCTD_Bin_File;
  char     cCTD_File_Type;
  ifstream fsCTD_File;
  
  int      NDataPoint;
  string   sCTD_DB_buffer;
  double   **dfCTD_DB;
  
  double   dfLong;
  double   dfLat;
  double   dfDepth;
  
  double   dfLongMatch;
  double   dfLatMatch;
  double   dfDepthMatch;
  
  double   dfC;
  double   dfT;
  double   dfS;
  
  void     GetCtsfrLongLatDepth();
 
};

#endif
