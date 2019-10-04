/************************************************************/
/*    NAME: Supun Randeni                                              */
/*    ORGN: MIT                                             */
/*    FILE: SimPhinsINS.h                                          */
/*    DATE:                                                 */
/************************************************************/

#ifndef SimPhinsINS_HEADER
#define SimPhinsINS_HEADER

#include "MOOS/libMOOS/MOOSLib.h"
#include <boost/asio.hpp>
#include <pcap.h>

using boost::asio::ip::udp;

class UDPClient{
public:
    //UDPClient();
    UDPClient(
              boost::asio::io_service& io_service,
              const std::string& host,
              const std::string& port
              ) : io_service_(io_service), socket_(io_service, udp::endpoint(udp::v4(), 0)) {
        udp::resolver resolver(io_service_);
        udp::resolver::query query(udp::v4(), host, port);
        udp::resolver::iterator iter = resolver.resolve(query);
        endpoint_ = *iter;
    }
    
    ~UDPClient()
    {
        socket_.close();
    }
    
    void send(const u_char* data, size_t dataLength) {
        socket_.send_to(boost::asio::buffer((void*)data, dataLength), endpoint_);
        std::cout << "Data " << data << std::endl;
    }
    
private:
    boost::asio::io_service& io_service_;
    udp::socket socket_;
    udp::endpoint endpoint_;
};

class SimPhinsINS : public CMOOSApp
{
 public:
   SimPhinsINS();
   ~SimPhinsINS();

 protected: // Standard MOOSApp functions to overload  
   bool OnNewMail(MOOSMSG_LIST &NewMail);
   bool Iterate();
   bool OnConnectToServer();
   bool OnStartUp();

 protected:
   void RegisterVariables();

 private: // Configuration variables
 std::string c_phins_ins_ip;
 std::string c_phins_ins_port;

 private: // State variables
 // UDP client related
 boost::asio::io_service m_io;
 UDPClient *m_udp_client;
 const u_char* startOfData;
 size_t dataLength;
};

#endif 
