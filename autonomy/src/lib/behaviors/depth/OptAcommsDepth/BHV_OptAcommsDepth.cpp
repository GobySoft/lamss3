// copyright 2010 H. Schmidt henrik@mit.edu
// 
// this file is part of lib_behaviors-henrik, a collection of pHelmIvP behaviors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this software.  If not, see <http://www.gnu.org/licenses/>.

// skeleton from tes' BHV_AcommsDepth for TL. Subtracting ambient noise profile

#include <iostream>
#include <cmath>
#include <fstream>
#include <sstream>
#include <algorithm>

#include <boost/lexical_cast.hpp>
#include <boost/assign.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>

#include "goby/version.h"
#if GOBY_VERSION_MAJOR >= 2
#include "goby/moos/moos_string.h"
using goby::moos::val_from_string;
#include "goby/time/legacy.h"
using goby::common::goby_time;
#include "goby/moos/moos_protobuf_helpers.h"
#else
#include "goby/util/string.h"
using goby::util::val_from_string;
#include "goby/util/time.h"
using goby::util::goby_time;
#include "goby/moos/libmoos_util/moos_protobuf_helpers.h"
#endif

#include "MBUtils.h"
#include "BuildUtils.h"
#include "ZAIC_Vector.h"

#include "BHV_OptCohAcoustDepth.h"

using goby::util::as;

BHV_OptCohAcoustDepth::BHV_OptCohAcoustDepth(IvPDomain gdomain)
  : IvPBehavior(gdomain),
    last_calc_time_(goby_time()),
    calculate_interval_(boost::posix_time::seconds(10)),
    look_ahead_seconds_(120),
    depth_envelope_(1000),
    depth_uncertainty_(50),
    departure_range_(20000),
    approach_range_(30000),
    range_max(45000),
    max_pitch(20.0*M_PI/180.0),
    current_range(10000),
    current_rate(10),
    num_forecast_ranges(10),
    depth_min(2000.0),
    depth_max(5000.0),
    number_depths(301),
    theta_min(-60.0),
    theta_max(60.0),
    number_theta(1000),
    max_number_bounces(6),
    tl_factor(1),
    adapt_tl_threshold(0.5),
    tl_threshold(0.5),
    active_region(true),
    peak_filter_(true),
    optimize_tl(true),
    optimize_snr(false),
    optimize_se(false),
    optimize_nl(false),
    multipath_penalty(0),
    log_open(false),
    svp_depth_resolution(5.0),
    svp_range_resolution(200.0),
    mseas_update_interval(41),
    last_svp_request_(0),
    svp_ready(false),
    svp_requested(false),
    got_lat_origin(false),
    got_long_origin(false),
    geo_init(false),
    valid_tl(false),
    new_ivp(false),
    water_depth(6000.0),
    surface_sound_speed(1480.0),
    sound_speed_gradient(0.016),
    beam_width(6.0),
    dip_min(-10.0),
    dip_max(10.0),
    debug(false)
{
    this->setParam("descriptor", "bhv_optcohacoustdepth");

    m_domain = subDomain(m_domain, "depth");

    //    addInfoVars("BELLHOP_TL_DEPTH");
    addInfoVars("BELLHOP_RESPONSE");
    addInfoVars("AMBIENT_NL_DEPTH");
    addInfoVars("VLA_ARRAY_GAIN");
    addInfoVars("NAV_DEPTH");
    addInfoVars("NAV_SPEED");
    addInfoVars("LOGGER_DIRECTORY");
    addInfoVars("SVP_RESPONSE");
    addInfoVars("LAT_ORIGIN");
    addInfoVars("LONG_ORIGIN");
    addInfoVars("NAV_X");
    addInfoVars("NAV_Y");
    addInfoVars("NAV_SPEED");
    addInfoVars("NAV_HEADING");

}


bool BHV_OptCohAcoustDepth::setParam(std::string param, std::string val) 
{
    if(IvPBehavior::setParam(param, val))
        return(true);

    if(param == "look_ahead_seconds")
    {
        look_ahead_seconds_ = as<int>(val);
        return true;
    }
    else if(param == "calculate_interval")
    {
        calculate_interval_ = boost::posix_time::seconds(as<int>(val));
        return true;
    }
    else if(param == "collaborator" || param == "contact")
    {
        collaborator_ = val;
      addInfoVars(std::string(boost::to_upper_copy(collaborator_) + "_RANGE"));
      addInfoVars(std::string(boost::to_upper_copy(collaborator_) + "_RATE"));
      addInfoVars(std::string(boost::to_upper_copy(collaborator_) + "_ARR_FILE"));
        return true;
    }    
    else if(param == "max_pitch")
    {
      max_pitch = as<double>(val)*M_PI/180.0;
      return true;
    }    
    else if(param == "sonar")
    {
      sonar = val;
      if (tolower(sonar) == "active")
	tl_factor = 2;
      return true;
    }    
    else if(param == "depth_min")
    {
      depth_min = as<double>(val);
      return true;
    }    
    else if(param == "depth_max")
    {
      depth_max = as<double>(val);
      return true;
    }    
    else if(param == "number_depths")
    {
      number_depths = int(as<double>(val));
      return true;
    }    
    else if(param == "theta_min")
    {
      theta_min = as<double>(val);
      return true;
    }    
    else if(param == "theta_max")
    {
      theta_max = as<double>(val);
      return true;
    }    
    else if(param == "number_theta")
    {
      number_theta = int(as<double>(val));
      return true;
    }    
    else if(param == "adapt_tl_threshold")
    {
      adapt_tl_threshold = as<double>(val);
      tl_threshold = adapt_tl_threshold;
      return true;
    }    
    else if(param == "multipath_penalty")
    {
      multipath_penalty = as<double>(val);
      return true;
    }    
    else if(param == "max_number_bounces")
    {
      max_number_bounces = int(as<double>(val));
      return true;
    }    
    else if(param == "depth_envelope")
    {
      depth_envelope_ = as<double>(val);
        return true;
    }    
    else if(param == "depth_uncertainty")
    {
      depth_uncertainty_ = as<double>(val);
        return true;
    }    
    else if(param == "activation_range")
    {
      departure_range_ = as<double>(val);
      approach_range_ = as<double>(val);
      return true;
    }    
    else if(param == "departure_range")
    {
      departure_range_ = as<double>(val);
      return true;
    }    
    else if(param == "approach_range")
    {
      approach_range_ = as<double>(val);
      return true;
    }    
    else if(param == "range_max")
    {
      range_max = as<double>(val);
      return true;
    }    
    else if(param == "peak_filter")
    {
      peak_filter_ = (tolower(val) == "true");
      return true;
    }    
    else if(param == "optimize")
    {
      optimize_se = (tolower(val) == "se" 
		      || tolower(val) == "signal-excess" );
      optimize_snr = (tolower(val) == "snr" 
		      || tolower(val) == "signal-noise-ratio" );
      optimize_tl = (tolower(val) == "tl" 
		      || tolower(val) == "transmission_loss" );
      optimize_nl = (tolower(val) == "nl" 
		      || tolower(val) == "noise_level" );

      get_tl = optimize_tl || optimize_snr || optimize_se;
      get_nl = optimize_nl || optimize_snr || optimize_se;
      get_ag = optimize_se;

      return true;
    }    
    else if (param == "water_depth")
      {
	water_depth = as<double>(val);
	return true;
      }        
    else if (param == "surface_sound_speed")
      {
	surface_sound_speed = as<double>(val);
	return true;
      }        
    else if (param == "sound_speed_gradient")
      {
	sound_speed_gradient = as<double>(val);
	return true;
      }        
    else if (param == "beam_width")
      {
	beam_width = as<double>(val);
	return true;
      }        
    return true;
}

IvPFunction *BHV_OptCohAcoustDepth::onRunState() 
{
  if(!m_domain.hasDomain("depth")) {
    postEMessage("No 'depth' variable in the helm domain");
    return(0);
  }
  
  if (!(optimize_tl || optimize_snr || optimize_nl || optimize_se))
    {
      postEMessage("No valid optimization variable chosen (tl, snr, se, nl)");
      return(0);
    }
 
  // Inititialize Geodesy
  if (!geo_init)
    {
      double lat_origin=getBufferDoubleVal("LAT_ORIGIN",got_lat_origin);
      double long_origin=getBufferDoubleVal("LONG_ORIGIN",got_long_origin);
      if (got_lat_origin && got_long_origin)
	{
	  m_Geodesy.Initialise(lat_origin,long_origin);
	  geo_init = true;
	}
    }

  if (MOOSTime() >= last_svp_request_ + mseas_update_interval)
    {
      requestSVPUpdate();
    }
  else
    {
      std::string serialized_svp_response = 
	getBufferStringVal("SVP_RESPONSE",svp_ready);
      if (svp_requested && MOOSTime() >= last_svp_request_ + 2 && svp_ready)
	handleSVPResponse(serialized_svp_response);
    }

  if (!log_open)
    {
      // Behavior logging
      bool ok_log;
      std::string log_dir = getBufferStringVal("LOGGER_DIRECTORY", ok_log);
      std::string log_name;
      std::stringstream ss;
      ss << "BHV_OptCohAcoustDepth_" << goby::common::goby_file_timestamp() << ".txt";
      if (ok_log)
	log_name = log_dir + "/../" + ss.str();
      else
	log_name = ss.str();

      file_out_.open(log_name.c_str());
      log_open = true;
      file_out_ << goby_time() << " look_ahead_seconds=" << look_ahead_seconds_ << std::endl; 
      file_out_ << goby_time() << " calculate_interval=" << calculate_interval_ << std::endl; 
      file_out_ << goby_time() << " collaborator=" << collaborator_ << std::endl; 
      file_out_ << goby_time() << " depth_envelope=" << depth_envelope_ << std::endl; 
      file_out_ << goby_time() << " depth_uncertainty=" << depth_uncertainty_ << std::endl; 
      file_out_ << goby_time() << " activation_range=" << departure_range_ << std::endl; 
      file_out_ << goby_time() << " departure_range=" << departure_range_ << std::endl; 
      file_out_ << goby_time() << " approach_range=" << approach_range_ << std::endl; 
      file_out_ << goby_time() << " range_max=" << range_max << std::endl; 
      file_out_ << goby_time() << " depth_min=" << depth_min << std::endl; 
      file_out_ << goby_time() << " depth_max=" << depth_max << std::endl; 
      file_out_ << goby_time() << " number_depths=" << number_depths << std::endl; 
      file_out_ << goby_time() << " theta_min=" << theta_min << std::endl; 
      file_out_ << goby_time() << " theta_max=" << theta_max << std::endl; 
      file_out_ << goby_time() << " number_theta=" << number_theta << std::endl; 
      file_out_ << goby_time() << " peak_filter=" << peak_filter_ << std::endl; 
      file_out_ << goby_time() << " optimize=" << optimize << std::endl; 
      file_out_ << goby_time() << " sonar=" << tolower(sonar) << std::endl; 
      file_out_ << goby_time() << " multipath penalty=" << multipath_penalty
		<< std::endl; 
    }
  bool ok_dep;
  double current_depth = getBufferDoubleVal("NAV_DEPTH", ok_dep);
  double current_speed = getBufferDoubleVal("NAV_SPEED", ok_dep);
  
  // file_out_ << goby_time() << "goby_time(): "  << goby_time()  << std::endl;
  // file_out_ << goby_time() << "last_calc_time_: "  << last_calc_time_  << std::endl;
  // file_out_ << goby_time() << "calculate_interval_: "  << calculate_interval_  << std::endl;
  
  
  // Initialize range and depth arrays
  double range1;
  double range2;
  std::vector<double> depth;
  std::vector<double> inc_tl;
  std::vector<double> elev_angle;

  depth.clear();
  inc_tl.clear();
  elev_angle.clear();
  double delta_depth = (depth_max-depth_min)/(number_depths-1);
  for (int i=0; i< number_depths; i++)
    depth.push_back(depth_min+i*delta_depth);
  
  // Make request for Bellhop TL computation
  
  if (get_tl)
    {
      if(goby_time() > last_calc_time_ + calculate_interval_)
	{
	  double forecast_range=0;
	  double forecast_time=0;
	  double delta_range=0;

	  bool ok_range;
	  double current_range = 0;

	  double ran = getBufferDoubleVal(std::string(boost::to_upper_copy(collaborator_) + "_RANGE"), ok_range);

	  bool ok_rate;
	  double range_rate = getBufferDoubleVal(std::string(boost::to_upper_copy(collaborator_) + "_RATE"), ok_rate);
	  if (ok_rate)
	    current_rate = range_rate;
	  else
	    current_rate = 0;

          if(ok_range && ok_rate)
	    {
	      current_range = ran;
	      forecast_time = look_ahead_seconds_ ;
	      forecast_range = current_range + forecast_time*current_rate;       
	      if (forecast_range > range_max)
		{
		  forecast_range = range_max;
		  forecast_time = fabs((forecast_range-current_range)/current_rate);
		}
	      delta_range = fabs((forecast_range-current_range)/
				 (num_forecast_ranges - 1) );
	      if (fabs(delta_range) < 1.0)
		delta_range = 1.0;
	    }

	  file_out_ << goby_time() << " Preparing Bellhop request " << std::endl
		    << "contact=" << collaborator_ << std::endl
		    << "forecast_time=" << forecast_time << std::endl
		    << "forecast_range=" << forecast_range << std::endl
		    << "current_range=" << current_range << std::endl
		    << "num_forecast_ranges=" << num_forecast_ranges << std::endl
		    << "delta_range=" << delta_range << std::endl;

 


	  iBellhopRequest request;
	  bellhop::protobuf::Environment::AdaptiveInfo* adaptive_info =
	    request.mutable_env()->mutable_adaptive_info();
	  
	  adaptive_info->set_contact(collaborator_);
	  adaptive_info->set_look_ahead_seconds(forecast_time);
	  adaptive_info->set_read_shd(true);
	  adaptive_info->set_auto_receiver_ranges(true);
	  adaptive_info->set_auto_source_depth(true);

	  adaptive_info->set_auto_receiver_ranges_delta(fabs(delta_range));	  
	  //request.mutable_env()->mutable_output()->set_type(
	  //   bellhop::protobuf::Environment::Output::INCOHERENT_PRESSURE);
	  request.mutable_env()->mutable_output()->set_type(
							    bellhop::protobuf::Environment::Output::ARRIVAL_TIMES);

	  //set receiver depths
	  bellhop::protobuf::Environment::Receivers* receivers =
	    request.mutable_env()->mutable_receivers();
	  receivers->set_number_in_depth(number_depths);
	  receivers->mutable_first()->set_depth(depth_min);
	  receivers->mutable_last()->set_depth(depth_max);
	  
	  // set beam angles (dip angle positive)
	  bellhop::protobuf::Environment::Beams* beams =
	    request.mutable_env()->mutable_beams();

	  beams->set_theta_min(int(theta_min));
	  beams->set_theta_max(int(theta_max));

	  beams->set_number(number_theta);

	  // Generate random request id
	  request_id = int(1e9 * rand()/RAND_MAX);
	  request.set_request_number(request_id);
          file_out_ << goby_time() << "Bellhoprequest " << request_id << std::endl;

	  // mseas svp updates
	  if (svp_ready)
	    {
	      request.mutable_env()->mutable_water_column()->
		CopyFrom(svp_response.water_column());
	      request.set_water_column_action(iBellhopRequest::OVERWRITE);
	    }

	  std::string serialized;
	  serialize_for_moos(&serialized, request);
	  
	  postMessage("BELLHOP_REQUEST", serialized, "repeatable");

	  // Wait for Bellhop before making next request

          new_tl = false;
	  last_calc_time_ = goby_time();
	}    
      
      bool ok_resp;
      
      iBellhopResponse response;
      
      std::string bellhop_in = getBufferStringVal("BELLHOP_RESPONSE", ok_resp);

      if (ok_resp)
	{
	  parse_for_moos(bellhop_in, &response);
	  bool my_resp = 
	    (response.requestor() == "pHelmIvP") &&
	    (response.request_number() == request_id) ;
            
	  if (my_resp)
	    {      
	      if (response.has_output_file())
		{
		  std::string bellhop_in = response.output_file();
		  file_out_ << goby_time() << "Bellhop response " << request_id << " --- " << bellhop_in << std::endl;
		
		  new_tl=eigenray_table.readEigenRayTable(bellhop_in, max_number_bounces);
		  if (new_tl)
		    {
		      file_out_ << goby_time() << "Got ray_table, ranges: " 
				<< eigenray_table.num_ranges 
				<< " depths: " << eigenray_table.num_depths
				<< std::endl; 
		      //		  number_depths = eigenray_table.getDepths(depth);
		      // Clear request id so we don't reprocess later
		      request_id =0;
		    }
		  else
		    file_out_ << goby_time() << "error reading eigenray table " << std::endl;
		}
	      else
		{
		  file_out_ << goby_time() << "Bellhop response " << request_id 
			    << std::endl;
		  file_out_ << goby_time() << "No eigenray table produced " 
			    << std::endl;
		  request_id = 0;
		  new_tl = 0;
		}
	      // check that target is outside activation ranges 
	      bool ok_range;
	      current_range = getBufferDoubleVal(std::string(boost::to_upper_copy(collaborator_) + "_RANGE"), ok_range);
	      bool ok_rate;
	      current_rate = getBufferDoubleVal(std::string(boost::to_upper_copy(collaborator_) + "_RATE"), ok_rate);
	      active_region =  ( ok_range && ok_rate &&
				 ( ((current_range > departure_range_) && (current_rate > 0.0)) ||
				   ((current_range < approach_range_) && (current_rate < 0.0))) );
	      file_out_ << goby_time() 
			<< " current range=" << current_range 
			<< " current range rate=" << current_rate 
			<< " current depth=" << current_depth 
			<< " , departure_range=" << departure_range_ 
			<< " , approach_range=" << approach_range_ 
			<< " , active_region=" << active_region 
			<< std::endl ; 
	      if (!active_region ) 
		return 0;
	    
	      // eigenray_table.clear();
	      if (current_rate >= 0)
		{
		  range1 = current_range;
		  range2 = current_range + current_rate*look_ahead_seconds_;
		}
	      else
		{
		  range2 = current_range;
		  range1 = current_range + current_rate*look_ahead_seconds_;
		}
	      // use RAP to find current dip angle
	      double dip_rap,amp_rap,tau_rap;
	      bool turn_rap;
	      // Initialize rap ray tracer
	      rap_trace.setEnv(surface_sound_speed, sound_speed_gradient, 
			       water_depth, water_depth); 
	      if ( rap_trace.rapRay(0,0,current_depth,(range1+range2)/2,0,0,
				    dip_rap,amp_rap,tau_rap,turn_rap))
		{
		  dip_min = dip_rap - beam_width;
		  dip_max = dip_rap + beam_width;
		}
	      else
		{
		  // If no RAP path assume horizantal beam
		  dip_min = - beam_width;
		  dip_max =   beam_width;
		}
	    }
	  else
	    new_tl = false;
	}	
      
      // check if we are in active region before continuing      
      if (!active_region ) 
	return 0;
      
      if (new_tl)
	{
	  // If new bellhop tl received, update tl table	
	  std::vector<double> ranges;
	  int num_ranges = eigenray_table.getRanges(ranges);
          if (debug) file_out_ << goby_time() << "ranges = " << ranges << std::endl;

	  // loop over forecast ranges and make sure we can 
	  // reach depth of future minimum tl
	  dom.clear();
	  range.clear();
	  elev_angle.clear();
	  double max_pitch_required = -100;
	  double min_pitch_required = 100;
	  double range_opt;
	  double depth_opt;
	  double min_tl_opt;
	  double mean_tl_opt;
	  bool done = false; 
	  valid_tl=false;
	  for (int j=0; j < num_ranges && !done ; j++)
	    {
	      double max_tl = -1e6;
	      double min_tl = 1e6;
	      double min_tl_dep;
	      double mean_power = 0;
	      double closest_tl = 0;
	      double depth_diff = 1e6;
	      std::vector<double> dep_tl;
	      std::vector<double> val_tl;
	      std::vector<double> val_elev;
	      int valid_depths = 0;
	      for (int i=0; i<number_depths; i++)
		{
		  double range_ref;
		  double depth_ref;
		  eigenray_table.getEigenRays(ranges[j], depth[i], 
					      range_ref, depth_ref, eig);
		  dep_tl.push_back(depth_ref);
		  
		  // Compute incoherent loss and find beamformer elevation 
		  // for most energetic ray
		  if (eig.num_rays <= 0)
		    {
		      file_out_ << goby_time()
				<< ">>> No eigenrays - returning " << std::endl;
		      return(0);
		    }
		  double elev=0;
		  double pow_max = 1e-30;
		  double power = 1e-30;

		  int iray_max = 0;
		  double pow_ratio = 1;
		  double max_tau;
		  for (int iray = 0; iray < eig.num_rays; iray++)
		    {
		      //if (eig.bundle[iray].dip_receiver >= dip_min &&
		      //  eig.bundle[iray].dip_receiver <= dip_max)
		      //{
		      double pow_ray = pow(eig.bundle[iray].amp,2);
		      power += pow_ray;
		      if (pow_ray > pow_max)
			{
			  iray_max = iray;
			  elev = eig.bundle[iray].dip_receiver;
			  pow_max = pow_ray;
			  max_tau = eig.bundle[iray].tau;
			} 
		    }
		  double max_rat = 0;
		  // multipath penalty if enabled
		  if (multipath_penalty > 0)
		    {
		      // Find how pow_max is above the next
		      for (int iray = 0; iray < eig.num_rays; iray++)
			{
			  double pow_ray = pow(eig.bundle[iray].amp,2);
			  double rat = pow_ray/pow_max;
			  if (iray != iray_max && rat > max_rat
			      && fabs(max_tau-eig.bundle[iray].tau) < 0.02)
			    max_rat = rat;
			}
		      //the closer the two top rays are, the higher the penalty
			power *= pow((1-max_rat),multipath_penalty);
		    }
		  if (!isnan(power) && power > 0.0)
		    {
		      mean_power += power;
		      
		      double tl = - tl_factor * 10.0 * log10(power);
		      if (debug) file_out_ << ",range=" << ranges[j]
					   << ",depth=" << depth[i]
					   << " tl=" << tl
					   << " Multipath award="
					   << -10*log10(max_rat)
					   << std::endl;
		      
		      if(tl > max_tl && tl != std::numeric_limits<double>::infinity())
			max_tl = tl;
		      if (tl < min_tl)
			{
			  min_tl =tl;
			  min_tl_dep = dep_tl[i];
			}
		      if (fabs(current_depth-dep_tl[i]) < depth_diff)
			{
			  depth_diff = fabs(current_depth-dep_tl[i]);
			  closest_tl = tl;
			}
		      val_tl.push_back(tl);
		      val_elev.push_back(elev*180.0/M_PI);
		      valid_depths++;
		    }
		  else
		    {
		      val_tl.push_back(0.0);
		      val_elev.push_back(elev*180.0/M_PI);
		    }
		}
	      
	      if (valid_depths == 0)
		return(0);

	      for (int i=0; i<number_depths; i++)
		{
		  if (val_tl[i] == 0.0)
		    val_tl[i] = max_tl;
		}
	      
	      double mean_tl = - tl_factor *
		10*log10(mean_power/valid_depths);
	      double pitch_required = 
		fabs(atan2(-(min_tl_dep - current_depth),
			   fabs(ranges[j]-current_range) )
		     *180./M_PI );
	      file_out_ << goby_time() << " current range=" << current_range 
			<< ", pitch required=" << pitch_required
			<< ", min_pitch required=" << min_pitch_required
			<< ", min_tl_dep=" << min_tl_dep
			<< ", min_tl=" << min_tl
			<< ", mean_tl=" << closest_tl
			<< ", mean_tl-min_tl=" << closest_tl-min_tl
			<< ", adapt_tl_threshold=" << tl_threshold
			<< std::endl;
	      
	      // If benefit of new depth is large ignore pitch required.
	      if ( (( mean_tl - min_tl >=2*tl_threshold) ||
		    (pitch_required < min_pitch_required && mean_tl - min_tl >= tl_threshold))
		   && ranges[j] <= range_max )
		{
		  dom = dep_tl;
		  range = val_tl; 
		  elev_angle = val_elev;
		  //		      max_pitch_required = pitch_required;
		  min_pitch_required = pitch_required;
		  range_opt =ranges[j];
		  depth_opt =min_tl_dep;
		  min_tl_opt = min_tl;
		  mean_tl_opt = mean_tl;
		  valid_tl=true;
		  // TL threshold hysteresis	 
		  tl_threshold = adapt_tl_threshold - 0.5;
		}
	      else
		tl_threshold = adapt_tl_threshold;
	      if (debug) file_out_ << goby_time()
				   << " ranges[" << j << "]=" << ranges[j] 
				   << ", depth_opt=" << min_tl_dep
				   << ", pitch required=" << pitch_required
				   << std::endl;
	    }
	  file_out_ << goby_time() << " range_opt=" << range_opt 
		    << ", depth_opt=" << depth_opt
		    << ", min_tl=" << min_tl_opt
		    << ", mean_tl=" << mean_tl_opt
		    << ", pitch required=" << min_pitch_required
		    << std::endl;
	  
	  //file_out_ << goby_time() << "domain: " << dom << std::endl;
	  //file_out_ << goby_time() << "tl_range: " << range << std::endl;
	  //file_out_ << goby_time() << "elev_angle: " << elev_angle << std::endl;
	  
	  if (valid_tl)
	    {
	      dom_tl_save = dom;
	      range_tl_save = range;
	      elev_angle_save = elev_angle;
	      new_tl =false;
	      new_ivp = true;
	    }
	  else
	    {
	      new_tl =false;
	      return(0);
	    }
	}
      else if (valid_tl)
	{
	  dom = dom_tl_save;
	  range = range_tl_save;
	  elev_angle = elev_angle_save;
	}

    }
  else
    valid_tl = true;
      

  // Now get the ambient noise profile if needed
  
  
  if ( get_nl )
    {   

      bool ok_nl;
      std::string noise_in = getBufferStringVal("AMBIENT_NL_DEPTH", ok_nl);
      if (!ok_nl)
	{
	  file_out_ << "No AMBIENT_NL_DEPTH" << std::endl;
	  return(0);
	}

      std::string depths_nl, nls;    
      val_from_string(depths_nl, noise_in, "depths");
      bool delta;
      if (val_from_string(nls, noise_in, "NL"))
	{
	  delta = false;
	}
      else if (val_from_string(nls, noise_in, "delta"))
	{
	  delta = true;
	}

      std::vector<std::string> depth_nl_vec;
      std::vector<std::string> nl_vec;
      boost::split(depth_nl_vec, depths_nl, boost::is_any_of(","));
      boost::split(nl_vec, nls, boost::is_any_of(","));
      
      std::vector<double> dom_nl, range_nl;
      
      try
	{
	  
	  BOOST_FOREACH(const std::string& s, depth_nl_vec)
	    {
	      if(s=="inf")
		dom_nl.push_back(std::numeric_limits<double>::infinity());
	      else
		dom_nl.push_back(boost::lexical_cast<double>(s));
	    }
	  
	  double max_noise = 0;
	  BOOST_FOREACH(const std::string& s, nl_vec)
	    {
	      if(s=="inf")
		range_nl.push_back(std::numeric_limits<double>::infinity());
	      else
		{
		  double d = boost::lexical_cast<double>(s);
		  if(d > max_noise)
		    max_noise = d;
		  range_nl.push_back(d);
		}
	    }
	  
	  // remove infinity values in favor of MAX non infinite NL.
	  BOOST_FOREACH(double& d, range_nl)
	    {
	      if(d == std::numeric_limits<double>::infinity())
		d = max_noise;
	    }
	}
      catch (boost::bad_lexical_cast&)
	{ return 0; } 

      if ( optimize_nl )
	{
	  dom = dom_nl;
	  range = range_nl;
	}
      else
	{ 
	  // For snr  add noise and tl
	  int n = dom.size();
	  int m = dom_nl.size();
	  if (range.size() < dom.size())
	    range.resize(dom.size(),0);
	  for (int i = 0; i<n; i++)
	    {
	      if (dom[i] <= dom_nl[0]) 
		range[i] += range_nl[0];
	      else if (dom[i] >= dom_nl[m-1]) 
		range[i] += range_nl[m-1];
	      else
		{
		  for (int j = 1; j < dom_nl.size(); j++)
		    {
		      if (dom[i] <= dom_nl[j] && dom[i] > dom_nl[j-1])
			range[i] += range_nl[j-1] 
			  + (dom[i] - dom_nl[j-1])/(dom_nl[j]-dom_nl[j-1])
			  *(range_nl[j] - range_nl[j-1]); 
		    }
		}
	    }
	}
      valid_nl = true;      
    }
  else
    valid_nl = true;
  
  // Finally get the array gain if optimizing signal excess
  
  
  if ( get_ag && valid_tl)
    {   

      bool ok_ag;
      std::string ag_in = getBufferStringVal("VLA_ARRAY_GAIN", ok_ag);
      if (!ok_ag)
	{
	  file_out_ << "No VLA_ARRAY_GAIN" << std::endl;
	  return(0);
	}

      std::string elev_ag, ags;    
      val_from_string(elev_ag, ag_in, "elevation");
      val_from_string(ags, ag_in, "AG");
      
      std::vector<std::string> elev_ag_vec;
      std::vector<std::string> ag_vec;
      boost::split(elev_ag_vec, elev_ag, boost::is_any_of(","));
      boost::split(ag_vec, ags, boost::is_any_of(","));
      
      std::vector<double> dom_ag, range_ag;

      try
	{
	  
	  BOOST_FOREACH(const std::string& s, elev_ag_vec)
	    {
	      if(s=="inf")
		dom_ag.push_back(std::numeric_limits<double>::infinity());
	      else
		{
		  dom_ag.push_back(boost::lexical_cast<double>(s));
		}
	    }
	  
	  double max_ag = 0;
	  BOOST_FOREACH(const std::string& s, ag_vec)
	    {
	      if(s=="inf")
		range_ag.push_back(std::numeric_limits<double>::infinity());
	      else
		{
		  double d = boost::lexical_cast<double>(s);
		  if(d > max_ag)
		    max_ag = d;
		  range_ag.push_back(d);
		}
	    }
	  
	  // remove infinity values in favor of MAX non infinite NL.
	  BOOST_FOREACH(double& d, range_ag)
	    {
	      if(d == std::numeric_limits<double>::infinity())
		d = max_ag;
	    }
	}
      catch (boost::bad_lexical_cast&)
	{ return 0; } 


      int num_depths =  dom.size();


      //
      // Find limits of elevation angles in table
      double max_theta=-360;
      double min_theta=360;
      int indx_max_theta; 
      int indx_min_theta; 
      for (int j=0; j < dom_ag.size(); j++)
	{
	  double d = dom_ag[j]; 
	  if (d > max_theta)
	    {
	      max_theta = d;
	      indx_max_theta = j;
	    }
	  else if (d < min_theta)
	    {
	      min_theta = d;
	      indx_min_theta = j;
	    }
	}
      if (indx_max_theta < indx_min_theta)
	{
	  // flip table
	  std::vector<double> dom_buf;
	  dom_buf = dom_ag;
	  dom_ag.clear();
	  for (int j=dom_buf.size()-1; j>=0; j--)
	    dom_ag.push_back(dom_buf[j]);
	  int idum = indx_min_theta;
	  indx_min_theta = indx_max_theta;
	  indx_max_theta = idum;
	}

      //      file_out_ << goby_time() << "Elevation angle: " << dom_ag << std::endl;
      // file_out_ << goby_time() << "Array Gain: " << range_ag << std::endl;

      // file_out_ << goby_time() << "indx, min_theta: " << indx_min_theta << ", " << min_theta << std::endl;
      // file_out_ << goby_time() << "indx, max_theta: " << indx_max_theta << ", " << max_theta << std::endl;

      int m = dom_ag.size();
      std::vector<double> ag;
      ag.clear();	  
      for (int i = 0; i<num_depths; i++)
	{
	  double ag_int;
	  if (elev_angle[i] <= min_theta) 
	    ag_int = range_ag[indx_min_theta];
	  else if (elev_angle[i] >= max_theta) 
	    ag_int = range_ag[indx_max_theta];
	  else
	    {
	      for (int j = 1; j <= m; j++)
		{
		  if (elev_angle[i] <= dom_ag[j] && elev_angle[i] > dom_ag[j-1])
		    ag_int = range_ag[j-1] 
		      + (elev_angle[i] - dom_ag[j-1])/(dom_ag[j]-dom_ag[j-1])
		      *(range_ag[j] - range_ag[j-1]); 
		}
	    }
	  ag.push_back(ag_int);
	  range[i] -= ag_int;
	}
      //      file_out_ << goby_time() << "array_gain: " << ag << std::endl;

      valid_ag = true;      
    }
  else
    valid_ag = true;
  
  // file_out_ << goby_time() << "domain: " << dom << std::endl;
  // file_out_ << goby_time() << "range: " << range << std::endl;
  // file_out_ << goby_time() << "max_range: " << range << std::endl;
  
  if ( valid_tl && valid_nl && valid_ag)
    {
      // normalize range
      std::vector<double>::iterator it_max =  max_element(range.begin(), range.end());
      std::vector<double>::iterator it_min =  min_element(range.begin(), range.end());
      
      double max = *(it_max);
      double min = *(it_min);
      
      BOOST_FOREACH(double& d, range)
	d = (max-d) / (max-min) * 100;
      
      if (peak_filter_)
	{    
	  
	  std::stringstream ss;
	  ss << "domain={" << dom << "}," << "range={" << range << "}";
	  postMessage("BHV_ACOUST_DEPTH_UNFILT", ss.str());
	  
	  // Use simple RC filter for filtering caustics
	  double int_forw = 0.0;
	  double int_back = 0.0;
	  double dz = dom[1]-dom[0];
	  double a = depth_uncertainty_;
	  double weight1 = exp(-dz/a);
	  double weight2 = 1.0 - weight1;
	  // forward filtering
	  std::vector<double> forw;
	  std::vector<double> back;
	  forw.clear();
	  back.clear();
	  int n=dom.size();
	  for (int i=0; i<n; i++)
	    {
	      forw.push_back(weight2 * range[i] + weight1 * int_forw);
	      int_forw = int_forw*weight1 + dz *range[i]/depth_uncertainty_;
	      back.push_back(weight2 * range[n-i-1] + weight1 * int_back);
	      int_back = int_back*weight1 + dz*range[n-i-1]/depth_uncertainty_;
	    }
	  for (int i=0; i<n; i++)
	    {
	      range[i] = 0.5 * (forw[i] + back[n-i-1]);
	    }
	}
      
      // Next weigh by envelope function to emphasize close depths
      if (ok_dep)
	{
	  int n = dom.size();
	  std::vector<double> win;
	  for (int i = 0; i<n; i++)
	    {
	      win.push_back(0.5 * 
			    (1.0 + exp(-0.5*pow((dom[i]-current_depth)/depth_envelope_,2))));
	      
	      range[i] = range[i] * win[i] ;
	    } 
	}
      
      // renormalize
      double maxval = 0;
      double maxval_depth = 0;
      for (int i = 0; i<dom.size(); i++)
	{
	  if (range[i] > maxval)
	    {
	      maxval = range[i];
	      maxval_depth = dom[i];
	    }
	}
      double scale_fac = 100.0/maxval;
      for (int i = 0; i<dom.size(); i++)
	range[i] *= scale_fac;
      
      // Henrik hack to make it work with limited depth interval
      if (dom[0] > 0.0)
	{
	  dom.push_back(0.0);
	  range.push_back(0.0);
	  int n = dom.size();
	  for (int i = n-1; i>0; i--)
	    {
	      dom[i] = dom[i-1];
	      range[i] = range[i-1];
	    }
	  dom[0] = 0.0;
	  range[0] = 0.0;
	}
      
      std::stringstream ss;
      ss << "domain={" << dom << "}," << "range={" << range << "}";
      if (new_ivp)
	{     
	  file_out_ << goby_time() << std::endl << " Resultng IvPFunction: " << std::endl 
		    << "domain={" << dom << "}," << std::endl 
		    << "range={" << range << "}" << std::endl;
	  new_ivp=false;
	  std::stringstream s2;
	  s2 << "depth=" << maxval_depth ;
	  //	  postMessage("ACOMMS_DEPTH", s2.str());
	}

      if ( optimize_snr )
	postMessage("BHV_SNR_DEPTH_FUNC", ss.str());
      else if ( optimize_nl )
	postMessage("BHV_NL_DEPTH_FUNC", ss.str());
      else if ( optimize_se )
	postMessage("BHV_SE_DEPTH_FUNC", ss.str());
      else
	postMessage("BHV_TL_DEPTH_FUNC", ss.str());
      
      ZAIC_Vector zaic(m_domain, "depth");
      
      zaic.setDomainVals(dom);
      zaic.setRangeVals(range);
      
      IvPFunction* ipf = zaic.extractIvPFunction();
      
      if(ipf)
	ipf->setPWT(m_priority_wt);
      else 
	{
	  postEMessage("Unable to generate OptCohAcoustDepth IvP function");
	  file_out_ << "Unable to generate OptCohAcoustDepth IvP function" 
		    << std::endl;     
	} 
      
      std::string zaic_warnings = zaic.getWarnings();
      if(zaic_warnings != "")
	{
	  postEMessage(zaic_warnings);
	  file_out_ << "ZAIC Warning: " << zaic_warnings << std::endl;      
	}
      //      else
      //	file_out_ << "generated ZAIC " << std::endl;      

      return(ipf);
    }
  else
    {
      file_out_ << " valid_tl = " << valid_tl 
		<< " valid_nl = " << valid_nl 
		<< " valid_ag = " << valid_ag << std::endl; 
      return(0);
    }
}


void BHV_OptCohAcoustDepth::requestSVPUpdate()

{
  
  SVP::Request svp_request;
  std::string serialized_svp_request;

  bool ok_osx,ok_osy,ok_cnx,ok_cny;
  double osx,osy,cnx,cny;
  osx = getBufferDoubleVal("NAV_X",ok_osx);
  osy = getBufferDoubleVal("NAV_Y",ok_osy);
  cnx = getBufferDoubleVal(boost::to_upper_copy(collaborator_)+"_NAV_X",
			   ok_cnx);
  cny = getBufferDoubleVal(boost::to_upper_copy(collaborator_)+"_NAV_Y",
			   ok_cny);

  if (ok_osx && ok_osy && ok_cnx && ok_cny)
    {
      double os_lat, os_lon, cn_lat, cn_lon;
      svp_request.set_request_id(11);
      svp_request.set_utc_time(MOOSTime());
      
      m_Geodesy.UTM2LatLong(osx,osy,os_lat,os_lon); 
      m_Geodesy.UTM2LatLong(cnx,cny,cn_lat,cn_lon); 
      
      svp_request.set_receiver_lat(os_lat);
      svp_request.set_receiver_lon(os_lon);
      svp_request.set_source_lat(cn_lat);
      svp_request.set_source_lon(cn_lon);
      
      svp_request.set_delta_range(svp_range_resolution);
      svp_request.set_delta_depth(svp_depth_resolution);
      
      
      serialize_for_moos(&serialized_svp_request, svp_request);
      postMessage("SVP_REQUEST", serialized_svp_request, "repeatable");
      file_out_ << goby_time() << "Requesting SVP update" << serialized_svp_request << endl; 
      last_svp_request_ = MOOSTime();
      svp_requested = true;
    }
  else
    {
      file_out_ << goby_time() << "Waiting for NAV Info" << endl; 
    }
}

bool BHV_OptCohAcoustDepth::handleSVPResponse(std::string serialized_svp_response)
{
  parse_for_moos(serialized_svp_response, &svp_response);
  file_out_ << goby_time() << "Got SVP update for source id " << svp_response.request_id() 
	   << endl; 
  svp_ready =true;
  svp_requested = false;
  return(svp_ready);
}
