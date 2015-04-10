////////////////////////////////////////////////////////
//#Include <iostream>
#include <iomanip>
#include <sstream>
#include <iterator>
#include <string>
#include <algorithm>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <cctype>
#include <stdio.h>
#include <list>
#include <sys/time.h>
#include "MOOS/libMOOS/MOOSLib.h"
#include "MOOS/libMOOS/Utils/MOOSUtils.h"
#include "ivp/MOOSGeodesy.h"
#include "iBackSeatServer.h"
#include "lamss/lib_anrp_util/CommServer.h"
#include "lamss/lib_anrp_util/NMEAMessage.h"
#include "lamss/lib_anrp_util/binary2hex.h"
#include "lamss/lib_anrp_util/CClientTCPSocket.h"
#include "lamss/lib_anrp_util/CUDPSocket.h"
#include "lamss/lib_anrp_util/CDatagramCtl.h"
#include "MBUtils.h"

//#include <goby/util/liblinebasedcomms/tcp_server.h>

using namespace std;
//using goby::util::as;

//goby::util::TCPServer tcp_server(as<unsigned>(std::string()));

CiBackSeatServer::CiBackSeatServer()
{
  // constructor
  MOOSTrace("====================================\n");
  MOOSTrace("\n");
  MOOSTrace("             iBackSeatServer\n");
  MOOSTrace("\n");
  MOOSTrace("\n");
  MOOSTrace("(c) Copyright, 2011 Arjuna Balasuriya MIT\n");
  MOOSTrace("Modified on 2/1/2012\n");
  MOOSTrace("====================================\n");

  navx = "";
  navy = "";
  navz = "";
  heading = "";
  depth = "";
  speed = "";
  yaw = "";
  pitch = "";
  altitude_str = "";
  last_bathy_z =0;
  prev_pit =0;
  prev_yw = 0;
  yw_time = 0;
  pit_time = 0;

  msgack = false;
}

CiBackSeatServer::~CiBackSeatServer()
{
  // destructor
}

bool CiBackSeatServer::OnNewMail(MOOSMSG_LIST &NewMail)
{
 
    CMOOSMsg Msg;


    double curr_time = MOOSTime();

  //  MOOSMSG_LIST::iterator pp;

    for(MOOSMSG_LIST::iterator p = NewMail.begin(); p != NewMail.end(); p++)
//  for(pp = NewMail.begin(); pp != NewMail.end(); pp++)
    {
    CMOOSMsg &Msg = *p;
 
    if(Msg.m_sKey == "FRONTSEAT_X")  
    {
	nav_x  = Msg.GetDouble();
	navx = doubleToString(nav_x, 2);
//	std::cout<<"Nav x : "<<nav_x<<std::endl;
    }
    if(Msg.m_sKey == "FRONTSEAT_Y")  
    {
	nav_y  = Msg.GetDouble();
	navy = doubleToString(nav_y, 2);
    }
    if(Msg.m_sKey == "FRONTSEAT_Z")  
    {
	nav_z  = Msg.GetDouble();
	navz = doubleToString(nav_z, 2);
    }
    if(Msg.m_sKey == "FRONTSEAT_HEADING")  
    {
	course  = Msg.GetDouble();
	heading = doubleToString(course, 1);
    }
    if(Msg.m_sKey == "FRONTSEAT_DEPTH")  
    {
	dep  = Msg.GetDouble();
	depth = doubleToString(dep, 1);
    }
    if(Msg.m_sKey == "FRONTSEAT_SPEED")  
    {
	spd  = Msg.GetDouble();
	speed = doubleToString(spd, 2);
    }
    if(Msg.m_sKey == "FRONTSEAT_YAW")  
    {
	yw  = Msg.GetDouble();
      // BF NMEA in degrees
	double yw_deg = yw*180/M_PI;
	yaw = doubleToString(yw_deg, 1);
	double delta_time = curr_time-yw_time;
	yaw_rate = doubleToString((yw_deg-prev_yw)/delta_time,1);
	prev_yw = yw_deg;
	yw_time = curr_time;
    }
    if(Msg.m_sKey == "FRONTSEAT_PITCH")  
    {
	pit  = Msg.GetDouble();
      // BF NMEA in degrees
	double pit_deg = pit*180/M_PI;
	pitch = doubleToString(pit_deg, 1);
	double delta_time = curr_time-pit_time;
	pitch_rate = doubleToString((pit_deg-prev_pit)/delta_time,1);
	prev_pit = pit_deg;
	pit_time = curr_time;
    }
    if(Msg.m_sKey == "BATHY_Z")
    {
	last_bathy_z = Msg.GetDouble();

    }

    if(Msg.m_sKey == "BUOYANCY_REPORT")
    {
	string buoy_report = Msg.m_sVal;

	double status, error, buoyancy;
   	MOOSValFromString(status,buoy_report,"status");
	MOOSValFromString(error,buoy_report, "error");
	MOOSValFromString(buoyancy,buoy_report,"buoyancy");

	string buoy_data = "$BFBOY,"+SecondsSinceMidnight(3)+","+intToString(int(status))+","+intToString(int(error))+",report,"+doubleToString(buoyancy,2)+"*";

   	string buoy_checksum=gen_NMEAchecksum(buoy_data.c_str());
	buoy_data+=buoy_checksum+"\r\n";
	std::cout <<"To clients:["<<buoy_data<<"]"<<std::endl;
	tcp_server->write(buoy_data);
    }

    if(Msg.m_sKey == "TRIM_REPORT")
    {
	string trim_report = Msg.m_sVal;

	double status,error,trm_pitch, trm_roll;
   	MOOSValFromString(status,trim_report,"status");
	MOOSValFromString(error,trim_report, "error");
	MOOSValFromString(trm_pitch,trim_report,"trim_pitch");
	MOOSValFromString(trm_roll,trim_report,"trim_roll");


	string trim_data = "$BFTRM,"+SecondsSinceMidnight(3)+","+intToString(int(status))+","+intToString(int(error))+",report,"+doubleToString(trm_pitch,2)+","+doubleToString(trm_roll,2)+"*";

   	string trim_checksum=gen_NMEAchecksum(trim_data.c_str());
	trim_data+=trim_checksum+"\r\n";
	std::cout <<"To clients:["<<trim_data<<"]"<<std::endl;
	m_Comms.Notify("NMEA_TO_BACKSEAT",trim_data);

	tcp_server->write(trim_data);

    }

    if(Msg.m_sKey == "INJECT_TO_BACKSEAT")
    {
	tcp_server->write(Msg.GetString() + "\r\n");
    }
    
    

  }

  
  altitude = nav_z - last_bathy_z;
 std::cout <<"Bathy_z = "<<last_bathy_z<<", Nav_z = "<<nav_z<<", nav_z - last_bathy_z = "<<altitude<<std::endl;
 m_Comms.Notify("FRONTSEAT_ALTITUDE",altitude);
  altitude_str = doubleToString(altitude,1);

  UpdateMOOSVariables(NewMail);
  
  return true;
}


bool CiBackSeatServer::OnConnectToServer()
{

  //Reading the moos file
  int tcp_port = 9113;
  if (!m_MissionReader.GetConfigurationParam("BackSeatPort", tcp_port))
    {
        std::cerr << "tcp port not specified, using "
                  << tcp_port << "." << std::endl;
    }

    
    std::cout << "starting server on port " << tcp_port << std::endl;
    try
    {
        tcp_server.reset(new goby::util::TCPServer(tcp_port));
        tcp_server->start();
    }
    catch(std::exception& e)
    {
        std::cerr << "failed to start TCP Server! Reason: " << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }

    string sValu;
    double dfLatOrigin;
    double dfLongOrigin;

    if(m_MissionReader.GetValue("LatOrigin", sValu))
    {
	dfLatOrigin = atof(sValu.c_str());
	m_Comms.Notify("LAT_ORIGIN",dfLatOrigin);
    }
    else
    {
	std::cout<<"No Lat Origin value in the MOOS file"<<std::endl;
    }

   if(m_MissionReader.GetValue("LongOrigin", sValu))
    {
	dfLongOrigin = atof(sValu.c_str());
	m_Comms.Notify("LONG_ORIGIN",dfLongOrigin);
    }
    else
    {
	std::cout<<"No Long Origin value in the MOOS file"<<std::endl;
    }

    if(!m_Geodesy.Initialise(dfLatOrigin, dfLongOrigin))
    {
	std::cout<<"Error in initialising Geodesy"<<std::endl;
    }


  //Register MOOS variables

  m_Comms.Register("UMVS_BLUEFIN_STATUS",0);
  m_Comms.Register("TRUE_X",0);
  m_Comms.Register("TRUE_Y",0);
  m_Comms.Register("TIDE_HEIGHT",0);
  m_Comms.Register("SIMULATION_MODE",0);
  m_Comms.Register("FRONTSEAT_X",0);
  m_Comms.Register("FRONTSEAT_Y",0);
  m_Comms.Register("FRONTSEAT_Z",0);
  m_Comms.Register("FRONTSEAT_YAW",0);
  m_Comms.Register("FRONTSEAT_YAW_RATE",0);
  m_Comms.Register("FRONTSEAT_SPEED",0);
  m_Comms.Register("FRONTSEAT_RUDDER",0);
  m_Comms.Register("FRONTSEAT_PITCH",0);
  m_Comms.Register("FRONTSEAT_PITCH_RATE",0);
  m_Comms.Register("FRONTSEAT_HEADING",0);
  m_Comms.Register("FRONTSEAT_ELEVATOR",0);
  m_Comms.Register("FRONTSEAT_DEPTH",0);
  m_Comms.Register("FRONTSEAT_BODY_VEL_X",0);
  m_Comms.Register("FRONTSEAT_BODY_VEL_Y",0);
  m_Comms.Register("BATHY_Z",0);
  m_Comms.Register("BUOYANCY_REPORT",0);
  m_Comms.Register("TRIM_REPORT",0);
  m_Comms.Register("INJECT_TO_BACKSEAT",0);

  return true;
}

bool CiBackSeatServer::Iterate()
{

    std::string in;
    std::string in_final;
    NMEAMessage Sentence;

//Tell the client that front seat is ready to receive commands
//string data_packet ="$BFMIS,"+SecondsSinceMidnight(3)+",file,Running*";
string data_packet ="$BFMSC,"+SecondsSinceMidnight(3)+",Running*";

   string checksum=gen_NMEAchecksum(data_packet.c_str());

   data_packet+=checksum+"\r\n";

   std::cout <<"To clients:["<<data_packet<<"]"<<std::endl;
 //  tcp_server.write(data_packet);

  tcp_server->write(data_packet);



    while(tcp_server->readline(&in)){
	std::cout <<"From clients:["<<in<<"]"<<std::endl;
	in_final=in;
    
    std::cout <<"<==From client :" << in_final << std::endl;
    Sentence.Set(in_final.c_str());

    m_Comms.Notify("RAW_MSG_FROM_BACKSEAT",in_final.c_str());

    Translate(Sentence);
    }

    
  double latti = 0;
  double longi = 0;


  if(!m_Geodesy.UTM2LatLong(nav_x, nav_y,latti, longi))
  {
	std::cout<<"Error in converting local coordinates to global"<<std::endl;
  }
  else
    {
      std::cout<<"Latitude = "<<latti<<", Longitude = "<<longi<<std::endl;
      m_Comms.Notify("FRONTSEAT_LATITUDE",latti);
      m_Comms.Notify("FRONTSEAT_LONGITUDE",longi);
      //  m_Comms.Notify("NAV_LAT",latti);
      //  m_Comms.Notify("NAV_LONG",longi);
      
      //Hemisphere Lat_a (N/S) & Long_b (E/W)
      string Lat_a;
      string Long_b;
      
      if(latti < 0.0) Lat_a = "S";
      else Lat_a = "N";
      
      if(longi < 0.0) Long_b = "W";
      else Long_b = "E";
      
      latti = fabs(latti);
      longi = fabs(longi);
      //Convert Decimal Degrees to BF NMEA standard
      //Latitude llll.ll - first 2 digits are decimal degrees
      //Longitude yyyyy.yy - first 3 are decimal degrees
      
      //Latitude conversion
      //  double lat_degrees = abs(floor(latti));
      //  double lat_minutes = abs(latti - floor(latti))*60;
      
      //  double nmea_lat = lat_degrees*100 + lat_minutes;
      
      int lat_degrees = std::floor(latti);
      int lat_minutes = std::floor((latti - lat_degrees)*60);
      int lat_ten_thousandth_minutes = std::floor(((latti-lat_degrees)*60 - lat_minutes)*10000);
      
      double nmea_lat = lat_degrees*100 + lat_minutes + 0.0001*lat_ten_thousandth_minutes;
      
      //    double dfLatDecDeg = m_Geodesy.DMS2DecDeg(nmea_lat);
      
      //    std::cout<<" =============> Lat Dec Deg = "<<dfLatDecDeg<<std::endl;
      
      std::cout<<"lat_degrees = "<<lat_degrees<<" lat_minutes = "<<lat_minutes<< " lat_ten_thousandth_minutes = "<< lat_ten_thousandth_minutes<<" nmea_lat = "<<nmea_lat<<std::endl;
      
      //Longitude conversion
      
      //  double lon_degrees = abs(floor(longi));
      //  double lon_minutes = abs(longi - floor(longi))*60;
      
      //  double nmea_lon = lon_degrees*100 + lon_minutes;
      
      int lon_degrees = std::floor(longi);
      int lon_minutes = std::floor((longi - lon_degrees)*60);
      int lon_ten_thousandth_minutes = std::floor(((longi - lon_degrees)*60 - lon_minutes)*10000);
      
      double nmea_lon = lon_degrees*100 + lon_minutes + 0.0001*lon_ten_thousandth_minutes;
      std::cout<<"nmea_lon = "<<nmea_lon<<std::endl;
      
      string lattitude_str = doubleToString(nmea_lat,4);
      if (lat_degrees < 1)
	lattitude_str = "00" + lattitude_str;
      if (lat_degrees < 10)
	lattitude_str = "0" + lattitude_str;
      
      string longitude_str = doubleToString(nmea_lon,4);
      if (lon_degrees < 1)
	longitude_str = "000" + longitude_str;
      else if (lon_degrees < 10)
	longitude_str = "00" + longitude_str;
      else if (lon_degrees < 100)
	longitude_str = "0" + longitude_str;
      
      
      
      string data_packet = "$BFNVG,"+SecondsSinceMidnight(3)+","+lattitude_str+","+Lat_a+","+longitude_str+","+Long_b+",1,"+altitude_str+","+depth+","+heading+",0,"+pitch+","+SecondsSinceMidnight(3)+"*";
      
      //string data_packet ="$BFSIMNVG,"+SecondsSinceMidnight(3)+","+navx+","+navy+","+navz+","+heading+","+depth+","+speed+","+yaw+","+pitch+","+altitude_str+"*";
      
      string checksum=gen_NMEAchecksum(data_packet.c_str());
      
      data_packet+=checksum+"\r\n";
      
      std::cout <<"To clients:["<<data_packet<<"]"<<std::endl;
      //  tcp_server.write(data_packet);
      
      tcp_server->write(data_packet);
      
      // Now the navigation rate message
      
      string vel_east = doubleToString(-spd * sin(yw));
      string vel_north = doubleToString(spd * cos(yw));
      string vel_down = doubleToString(-spd * sin(pit));
      
      data_packet = "$BFNVR,"+SecondsSinceMidnight(3)+","
	+vel_east+","
	+vel_north+","
	+vel_down+","
	+pitch_rate+",1,"
	+"0.0"+","
	+yaw_rate+","
	+SecondsSinceMidnight(3)+"*";
      
      checksum=gen_NMEAchecksum(data_packet.c_str());
      
      data_packet+=checksum+"\r\n";
      
      std::cout <<"To clients:["<<data_packet<<"]"<<std::endl;
      
      tcp_server->write(data_packet);
    }
  return true;
}


bool CiBackSeatServer::OnStartUp()
{

   return true;
}

void CiBackSeatServer::Translate(NMEAMessage Sentence)
{

  std::cout<<"In the Translate with "<<Sentence.Part(0).c_str()<<std::endl;

  if(Sentence.Part(0)=="BPLOG")
  {
	//Message requests from frontseat
	//BFACK request
	if(Sentence.Part(1) == "ACK")
	{
	   if(Sentence.Part(2) == "ON")
	   {
		msgack = true;
	   }
	   else
		msgack = false;
	}
  }

  //Send an ack for any msg from backseat
  
  string messg = Sentence.Part(0);
  string msgtype = messg.substr(2);
  string general_ack = "$BFACK,"+SecondsSinceMidnight(3)+","+msgtype+","+Sentence.Part(1)+",1,2,0,general ack"+"*";
  string genchecksum=gen_NMEAchecksum(general_ack.c_str());
  general_ack+=genchecksum+"\r\n";
  std::cout <<"To clients:["<<general_ack<<"]"<<std::endl;
  tcp_server->write(general_ack);

  if (Sentence.Part(0)=="BPRMB")
  {

	std::cout <<"<===================== Received Desired control commands"<<std::endl;

	double backseat_time = atof(Sentence.Part(1).c_str());
   	double desired_heading = atof(Sentence.Part(2).c_str());
   	double desired_depth = atof(Sentence.Part(3).c_str());
   	double depth_mode = atof(Sentence.Part(4).c_str());
   	double desired_speed = atof(Sentence.Part(5).c_str());

	m_Comms.Notify("DESIRED_HEADING",desired_heading);
	m_Comms.Notify("DESIRED_DEPTH",desired_depth);
	m_Comms.Notify("DESIRED_SPEED",desired_speed);

/*
	string rmb_ack = "$BFACK,"+SecondsSinceMidnight(3)+",RMB,"+Sentence.Part(1)+",1,2,0,rmb ack"+"*";
       	string rmbchecksum=gen_NMEAchecksum(rmb_ack.c_str());
      	rmb_ack+=rmbchecksum+"\r\n";
      	std::cout <<"To clients:["<<rmb_ack<<"]"<<std::endl;
      	tcp_server->write(rmb_ack);
*/
  }
  if (Sentence.Part(0) == "BPNPU")
  {
	string backseat_time = (Sentence.Part(1).c_str());
	string backseat_latitude = (Sentence.Part(2).c_str());
	string backseat_lathemisphere = (Sentence.Part(3).c_str());
	string backseat_longitude = (Sentence.Part(4).c_str());
	string backseat_lonhemisphere = (Sentence.Part(5).c_str());
	string backseat_errorprob = (Sentence.Part(6).c_str());
	string backseat_depth = (Sentence.Part(7).c_str());
	string backseat_forceupdate = (Sentence.Part(8).c_str());
	string backseat_timestamp = (Sentence.Part(9).c_str());

//	string usbl_msg = "BPNPU"+","+backseat_time+","+backseat_latitude+","+backseat_lathemisphere+","+backseat_longitude+","+backseat_lonhemisphere+","+backseat_errorprob+","+backseat_depth+","+backseat_forceupdate+","+backseat_timestamp;
string usbl_msg = "BPNPU";
	usbl_msg = usbl_msg+","+backseat_time+","+backseat_latitude+","+backseat_lathemisphere+","+backseat_longitude+","+backseat_lonhemisphere+","+backseat_errorprob+","+backseat_depth+","+backseat_forceupdate+","+backseat_timestamp;

	m_Comms.Notify("USBL_MESSAGE",usbl_msg);
/*
	string npu_ack = "$BFACK,"+SecondsSinceMidnight(3)+",NPU,"+Sentence.Part(1)+",1,2,0,npu ack"+"*";
       	string npuchecksum=gen_NMEAchecksum(npu_ack.c_str());
      	npu_ack+=npuchecksum+"\r\n";
      	std::cout <<"To clients:["<<npu_ack<<"]"<<std::endl;
      	tcp_server->write(npu_ack);
*/
  }


  if(Sentence.Part(0) == "BPSIL")
  {
	int frontseat_silent = atoi(Sentence.Part(2).c_str());
	if(frontseat_silent)
	{
		m_Comms.Notify("FRONTSEAT_SILENT","true");
	}
	else
	{
		m_Comms.Notify("FRONTSEAT_SILENT","false");
	}

/*
	string sil_ack = "$BFACK,"+SecondsSinceMidnight(3)+",SIL,"+Sentence.Part(1)+",1,2,0,silent ack"+"*";
       	string silchecksum=gen_NMEAchecksum(sil_ack.c_str());
      	sil_ack+=silchecksum+"\r\n";
      	std::cout <<"To clients:["<<sil_ack<<"]"<<std::endl;
      	tcp_server->write(sil_ack);
*/
  }

  if(Sentence.Part(0) == "BPBOY")
  {
	m_Comms.Notify("BUOYANCY_CONTROL","true");

/*
	string boy_ack = "$BFACK,"+SecondsSinceMidnight(3)+",BOY,"+Sentence.Part(1)+",1,2,0,buoyancy ack"+"*";
       	string boychecksum=gen_NMEAchecksum(boy_ack.c_str());
      	boy_ack+=boychecksum+"\r\n";
      	std::cout <<"To clients:["<<boy_ack<<"]"<<std::endl;
      	tcp_server->write(boy_ack);
*/	
  }

  if(Sentence.Part(0) == "BPTRM")
  {
	m_Comms.Notify("TRIM_CONTROL","true");

/*
	string trm_ack = "$BFACK,"+SecondsSinceMidnight(3)+",TRM,"+Sentence.Part(1)+",1,2,0,trim ack"+"*";
       	string trimchecksum=gen_NMEAchecksum(trm_ack.c_str());
      	trm_ack+=trimchecksum+"\r\n";
      	std::cout <<"To clients:["<<trm_ack<<"]"<<std::endl;
      	tcp_server->write(trm_ack);
*/
  }


}

string CiBackSeatServer::SecondsSinceMidnight(int nDigits)
{
    std::stringstream ss;
    struct timeval tv; 
    struct timezone tz;
    struct tm today;
    string sVar;

    gettimeofday(&tv,&tz);
    gmtime_r(&(tv.tv_sec),&today);

    // Decimal digits allowed
    if (nDigits < 1) nDigits=1;
    if (nDigits > 6) nDigits=6;

    // Generate time string
    ss << setw(2) << setfill('0') << today.tm_hour;
    ss << setw(2) << setfill('0') << today.tm_min;
    ss << setw(2) << setfill('0') << today.tm_sec;
    ss << ".";
// Replaced pow10 with pow.  OS X doesn't have pow10.
//   ss << setw(nDigits) << setfill('0') << round(tv.tv_usec*pow10(nDigits-6));
    ss << setw(nDigits) << setfill('0') << round(tv.tv_usec*pow(10.0, nDigits-6));
    ss >> sVar;

    return sVar;
}

string CiBackSeatServer::gen_NMEAchecksum(const char *str)
{
    // This is a modified version of the VerifyChecksum() code written by
    // Andrew Patrikalakis (anrp@mit.edu)
    // The original code can be found in <MOOSPATH>/lib/anrp_util/NMEAMessage.h
	
    const char *st, *ed;
    int sum = 0;
	
    std::stringstream ss;
    string check_sum;
		
    st = strchr(str, '$');
    ed = strchr(str, '*');

    if (!st || !ed) {
        return 0;
    }

    for (st++; st < ed; st++) {
        sum ^= *st;
    }

    //This part was added by Matt on RVCT during May 07
    char buf[8];
    sprintf(buf, "%02x", sum);
    return buf;

    //ss.setf(std::ios_base::hex,std::ios_base::basefield);
    //ss << uppercase << sum;
    //ss >> check_sum;
    //return check_sum;
}


string doubleToString(double val, int digits)
{
  char buffAUX[10] = "%.5f\0";
  if((digits>0)&&(digits<7))
    buffAUX[2] = digits+48;

  char buff[100];
  sprintf(buff, buffAUX, val);

  string str = buff;
  return(str);
}

string intToString(int val)
{
  char buff[500];
  sprintf(buff, "%d", val);
  string str = buff;
  return(str);
}

