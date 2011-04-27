#ifndef __tgt_decode_h__
#define __tgt_decode_h__

#include "MOOSLIB/MOOSLib.h"
#include "MBUtils.h"
#include "MOOSGenLib/MOOSGenLib.h"

#include <stdlib.h>
#include <string.h>

#include <string>
#include <vector>

/*
   Decodes a target cue message. Used by pSearch, pMultiTargetSim, pTargetAIS
   trk_str: String with track cue info: tgt_num=1,tgt_x=1235, etc.
  */

bool parse_brng_stat(std::string status,
		  int & node,
		  int & state,
		  double & bearing,
		  double & sigma,
		  double & xp,
		  double & yp,
		  double & time
		  );

bool parse_tgt_stat(std::string trk_str, 
		int & tgt_num,
		double & tgt_x,
		double & tgt_y,
		double & tgt_dep,
		double & tgt_hdg,
		double & tgt_spd,
		double & tgt_utc,
		double & tgt_frq,
		double & tgt_bw,
		double & tgt_spl,
		std::vector<double> & tone_frq,
		std::vector<double> & tone_dbl
		);

bool parse_track_stat(std::string val,
		      int & node,
		      int & state,
		      double & x,
		      double & y,
		      double & heading,
		      double & speed,
		      double & time
		      );

#endif /* __tgt_decode_h__ */
