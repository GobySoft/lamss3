////////////////////////////////////////////////

#ifndef __CiBackSeatServer_h__
#define __CiBackSeatServer_h__

#include <memory>
#include "MOOSLIB/MOOSLib.h"
#include "MOOSUtilityLib/MOOSGeodesy.h"
#include <string>
#include "lamss/lib_anrp_util/NMEAMessage.h"
#include "goby/util/linebasedcomms.h"

//#include </home/arjuna/goby/src/util/liblinebasedcomms/tcp_server.h>
//#include <goby/util/liblinebasedcomms/tcp_server.h>

class CiBackSeatServer : public CMOOSApp
{
public:
	CiBackSeatServer();
	virtual ~CiBackSeatServer();

	bool OnNewMail(MOOSMSG_LIST &NewMail);
	bool Iterate();
	bool OnConnectToServer();
	bool OnStartUp();



private:
	CMOOSGeodesy m_Geodesy;
	// insert local vars here
	std::string SecondsSinceMidnight(int nDigits);
	std::string gen_NMEAchecksum(const char *str);
	void Translate(NMEAMessage Sentence);
	std::string tcp_port;
  	std::auto_ptr<goby::util::TCPServer> tcp_server;

    	std::string navx;
    	std::string navy;
    	std::string navz;
    	std::string heading;
    	std::string depth;
    	std::string speed;
    	std::string yaw;
    	std::string pitch;
    	std::string altitude_str;
	std::string pitch_rate;
	std::string yaw_rate;


	double nav_x;
	double nav_y;
	double nav_z;
	double course;
	double dep;
	double spd;
	double yw;
	double pit;
	double last_bathy_z;
	double altitude;
	
	double prev_pit;
	double prev_yw;
	double pit_time;      
	double yw_time;
      
//goby::util::TCPServer tcp_server(as<unsigned>(std::string()));

};

#endif /* __CiBackSeatServer_h__ */
