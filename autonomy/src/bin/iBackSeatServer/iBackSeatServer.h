////////////////////////////////////////////////

#ifndef __CiBackSeatServer_h__
#define __CiBackSeatServer_h__

#include <memory>
#include "MOOSLIB/MOOSLib.h"
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
	// insert local vars here
	std::string SecondsSinceMidnight(int nDigits);
	std::string gen_NMEAchecksum(const char *str);
	void Translate(NMEAMessage Sentence);
	std::string tcp_port;
  	std::auto_ptr<goby::util::TCPServer> tcp_server;
      
//goby::util::TCPServer tcp_server(as<unsigned>(std::string()));

};

#endif /* __CiBackSeatServer_h__ */
