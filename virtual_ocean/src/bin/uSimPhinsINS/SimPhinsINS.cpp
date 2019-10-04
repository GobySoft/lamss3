/************************************************************/
/*    NAME: Supun Randeni                                              */
/*    ORGN: MIT                                             */
/*    FILE: SimPhinsINS.cpp                                        */
/*    DATE:                                                 */
/************************************************************/

#include <iterator>
#include "MBUtils.h"
#include "SimPhinsINS.h"
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

using namespace std;

//---------------------------------------------------------
// Constructor

SimPhinsINS::SimPhinsINS()
{
}

//---------------------------------------------------------
// Destructor

SimPhinsINS::~SimPhinsINS()
{
}

//---------------------------------------------------------
// Procedure: OnNewMail

bool SimPhinsINS::OnNewMail(MOOSMSG_LIST &NewMail)
{
  MOOSMSG_LIST::iterator p;
   
  for(p=NewMail.begin(); p!=NewMail.end(); p++) {
    CMOOSMsg &msg = *p;

#if 0 // Keep these around just for template
    string key   = msg.GetKey();
    string comm  = msg.GetCommunity();
    double dval  = msg.GetDouble();
    string sval  = msg.GetString(); 
    string msrc  = msg.GetSource();
    double mtime = msg.GetTime();
    bool   mdbl  = msg.IsDouble();
    bool   mstr  = msg.IsString();
#endif
   }
	
   return(true);
}

//---------------------------------------------------------
// Procedure: OnConnectToServer

bool SimPhinsINS::OnConnectToServer()
{
   // register for variables here
   // possibly look at the mission file?
   // m_MissionReader.GetConfigurationParam("Name", <string>);
   // m_Comms.Register("VARNAME", 0);
	
   RegisterVariables();
   return(true);
}

//---------------------------------------------------------
// Procedure: Iterate()
//            happens AppTick times per second

bool SimPhinsINS::Iterate()
{
  // // Converting string to binary
  // std::string s = "IX";

  // int n = s.length(); 
  // for (int i = 0; i <= n; i++) 
  // { 
  //   // convert each char to ASCII value 
  //   int val = int(s[i]); 
  //   // Convert ASCII value to binary 
  //   string bin = ""; 
  //   while (val > 0) 
  //   { 
  //     (val % 2)? bin.push_back('1') : 
  //               bin.push_back('0'); 
  //               val /= 2; 
  //   } 
  //     reverse(bin.begin(), bin.end()); 
  
  //     cout << bin << " "; 
  //     m_udp_client->send(bin, n);
  // } 


  return(true);
}

//---------------------------------------------------------
// Procedure: OnStartUp()
//            happens before connection is open

bool SimPhinsINS::OnStartUp()
{
  list<string> sParams;
  m_MissionReader.EnableVerbatimQuoting(false);
  if(m_MissionReader.GetConfiguration(GetAppName(), sParams)) {
    list<string>::iterator p;
    for(p=sParams.begin(); p!=sParams.end(); p++) {
      string original_line = *p;
      string param = stripBlankEnds(toupper(biteString(*p, '=')));
      string value = stripBlankEnds(*p);
      
      if(param == "FOO") {
        //handled
      }
      else if(param == "BAR") {
        //handled
      }
    }
  }
  string sVal;
    
  if(m_MissionReader.GetConfigurationParam("phins_ip",sVal))
  {
    c_phins_ins_ip = sVal;
    std::cout << "IP address: " << c_phins_ins_ip << std::endl;
  }
  if(m_MissionReader.GetConfigurationParam("phins_port",sVal))
  {
    c_phins_ins_port = sVal;
    std::cout << "Port: " << c_phins_ins_port << std::endl;
  } 
  RegisterVariables();	
  m_udp_client = new UDPClient(m_io, c_phins_ins_ip.c_str(), c_phins_ins_port.c_str());
  return(true);
}

//---------------------------------------------------------
// Procedure: RegisterVariables

void SimPhinsINS::RegisterVariables()
{
  // Register("FOOBAR", 0);
}

