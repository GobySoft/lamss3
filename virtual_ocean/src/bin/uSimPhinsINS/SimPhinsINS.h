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
#include "lamss/lib_lamss_protobuf/IXBlueINSData.pb.h"
#include "goby/moos/goby_moos_app.h"

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
   void PublishPhinsDataMOOSVar();

 private: // Configuration variables
 std::string c_phins_ins_ip;
 std::string c_phins_ins_port;
 double c_ins_bias_err_udot;
 double c_ins_bias_err_vdot;
 double c_ins_bias_err_wdot;
 double c_ins_scale_err;
 double c_ins_stdev;

 private: // State variables
 double m_current_u;
 double m_current_v;
 double m_current_w;
 double m_current_water_vel_x;
 double m_current_water_vel_y;
 double m_current_nav_depth;
 double m_current_nav_roll;
 double m_current_nav_pitch;
 double m_current_nav_heading;
 double m_current_u_time;
 double m_current_nav_depth_time;
 bool m_current_u_received;
 double m_auv_udot;
 double m_auv_vdot;
 double m_auv_wdot;
 double m_prev_auv_u;
 double m_prev_auv_v;
 double m_prev_auv_w;
 double m_prev_u_time;
 double m_ins_udot;
 double m_ins_vdot;
 double m_ins_wdot;
 double m_current_nav_x;
 double m_current_nav_y;
 int m_ins_update_no;
 double m_ins_calc_u;
 double m_ins_calc_v;
 double m_ins_calc_w;
 double m_ins_calc_x;
 double m_ins_calc_y;

 // UDP client related
 boost::asio::io_service m_io;
 UDPClient *m_udp_client;
 const u_char* startOfData;
 size_t dataLength;
};

#endif 
