// Copyright 2015 
// Henrik Schmidt
// Toby Schneider 
// Massachusetts Institute of Technology 

#ifndef LamssTrackManager20150402H
#define LamssTrackManager20150402H

#include <boost/circular_buffer.hpp>
#include "LamssTrackManager_fsm.h"
#include "goby/moos/goby_moos_app.h"
#include "LamssTrackManager_config.pb.h"
#include "MBUtils.h"
#include "ivp/MOOSGeodesy.h"

// From lib_henrik_util
#include <lamss/lib_henrik_util/CTrack.h>

#define NO_TRACK     0
#define RELATIVE     1
#define ABSOLUTE     2

class CReport
{
 public:

  bool initReport(int node, double x, double y, double spd, double hdg, double utc);
  int  reportTrack(int id, int prio, int templ, double bearing, double sigma, 
		   double rate, double rate_sigma, double snr);
  std::string createContactReport();

 protected:

  int trk_node;
  double ael_x;
  double ael_y;
  double nav_spd;
  double nav_hdg;
  double nav_dep;
  double nav_utc;

  int num_trk;

  std::vector<int> trk_id;
  std::vector<int> trk_priority;
  std::vector<int> trk_template;
  std::vector<double> trk_bearing;
  std::vector<double> trk_sigma;
  std::vector<double> trk_rate;
  std::vector<double> trk_rate_sigma;
  std::vector<double> trk_snr;
};


class Mavg
{
	public:

		Mavg(int length);
		~Mavg();

		double get_FVal();
		void set_RawVal(double);

	protected:

		int len;
		int num_measurements;
		double avg;

		std::deque<double>  svals;
		std::deque<double>  cvals;
};

class TrackManagerException : public std::runtime_error
{
  public:
  TrackManagerException(const std::string& what) : std::runtime_error(what)
    { }
};

class LamssTrackManager : public GobyMOOSApp
{
  public:
    static void* driver_library_handle_;
    static LamssTrackManager* get_instance();

  private:
    LamssTrackManager(LamssTrackManagerConfig& cfg);
    ~LamssTrackManager() { }
    LamssTrackManager(const LamssTrackManager&);
    LamssTrackManager& operator=(const LamssTrackManager&);

    CMOOSGeodesy m_Geodesy;

    
    // synchronous event
    void loop();    
    void slow_loop(double curr_time);
    void try_publish();
    void send_deploy();

    // mail handlers

    void handle_ifs_data(const CMOOSMsg& msg);
    void handle_deploy(const CMOOSMsg& msg);
    void handle_prosecute(const CMOOSMsg& msg);
    void handle_vehicle_id(const CMOOSMsg& msg);
    void handle_mission_state(const CMOOSMsg& msg);
    void handle_ael_heading(const CMOOSMsg& msg);
    void handle_collaborator_id(const CMOOSMsg& msg);
    void handle_collaborator_name(const CMOOSMsg& msg);
    void handle_bearing_tracks(const CMOOSMsg& msg);
    void handle_collaborator_bearing_tracks(const CMOOSMsg& msg);
    void handle_prosecute_heading(const CMOOSMsg& msg);

    // queue_event for FSM
    template<typename Event>
        void queue_event()
    { scheduler_.queue_event(fsm_, boost::intrusive_ptr<Event>(new Event())); }
  private:
    LamssTrackManagerConfig& cfg_;
    boost::statechart::fifo_scheduler<> scheduler_;
    boost::statechart::fifo_scheduler<>::processor_handle fsm_;
    boost::shared_ptr<boost::thread> fsm_thread_;

    double last_slow_loop_t_;
    
    goby::moos::protobuf::NodeStatus last_nav_;

    static LamssTrackManager* inst_;    

    // Adopted from pTrackMonitor
		CReport BearingTrack;
		CTrack Empty;

		std::vector<CTrack> Tracks;
		std::vector<bool> new_track;

		int tgt_track_id;

		int collaborator_id;
		std::string collaborator_name;

		std::string sId;
		std::vector<CTrack> collab_Tracks;
		std::vector<bool> collab_new_track;
		int collab_num_tracks;

		int trk_num;
		int trk_prio; // 1-10, 10 highest.
                int trk_template;
		int trk_state; // 0: Invalid, 1: Relative, 2: Bbsolute.
		int trk_ips;   // -1: Port, 1: Starboard, 0: Ambiguous
		double trk_x; // array x corresponding to track
		double trk_y; // array y corresponding to track
		double trk_bearing;
		double trk_sigma;
		double trk_utc;
                double trk_rate;
                double trk_rate_sigma;
                double trk_snr;
		int trk_nfeat;
		int trk_beamno;

		double prosecute_heading;
		int prosecute_mission;
		bool auto_prosecute;

		bool detect_mode_;
		std::vector<double> class_features;

		int num_tracks;

		Mavg *pitch_filter;
		Mavg *heading_filter;
		bool filt_set;
		int priority_threshold;
		int filt_len;
		double track_delay;
		int prev_highest;

//vehicle and position
		double xp,yp,auv_depth,ael_x_offset,ael_y_offset,ael_x,ael_y;
		double heading,raw_heading,pitch,raw_pitch;

//prosecute mode stuff
		bool sonar_on_;
		
//local node number
		int node;

		void reset_tracking();
		int str2dbl_vec(std::string, std::vector<double>& values);
		bool dbl_vec2str(std::vector<double> numbers, std::string& str);
		double angle360(double hdg);
		bool postStateVars();
		bool postStateCollab();

};

#endif
