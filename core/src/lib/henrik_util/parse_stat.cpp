#include "parse_stat.h"
#include <string>

using namespace std;

bool parse_tgt_stat(std::string val, 
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
		  std::vector<double> & tone_dbl)
{

  string bite;
 
  cout << "string = " << val << endl;
 
  MOOSValFromString(bite,val,"tgt_num");
  tgt_num = atoi(bite.c_str());
  MOOSValFromString(tgt_x,val,"tgt_x");
  MOOSValFromString(tgt_y,val,"tgt_y");
  MOOSValFromString(tgt_dep,val,"tgt_dep");
  MOOSValFromString(tgt_hdg,val,"tgt_hdg");
  MOOSValFromString(tgt_spd,val,"tgt_spd");
  MOOSValFromString(tgt_utc,val,"tgt_utc");
  MOOSValFromString(tgt_frq,val,"tgt_freq");
  MOOSValFromString(tgt_bw,val,"tgt_bw");
  MOOSValFromString(tgt_spl,val,"tgt_spl");

  tone_frq.clear();
  tone_dbl.clear();
  double freq;
  for (int i=0; i<5;i++)
    {
      string fkey = "tn" + intToString(i+1) + "_freq";
      if (MOOSValFromString(freq,val,fkey))
	if (freq > 0)
	  {
	    tone_frq.push_back(freq);
	    double dbl;
	    string dbkey = "tn" + intToString(i+1) + "_dbl";
	    MOOSValFromString(dbl,val,dbkey);
	    tone_dbl.push_back(dbl);
	  }
    }


  return(true);
}

bool parse_track_stat(std::string val,
		      int & node,
		      int & state,
		      double & x,
		      double & y,
		      double & heading,
		      double & speed,
		      double & time
)
{

  string bite;
  //MOOSTrace("string = %s\n",status.c_str());

  MOOSValFromString(bite,val,"node");
  node = atoi(bite.c_str());
  MOOSValFromString(bite,val,"state");
  state = atoi(bite.c_str());
  if (state == 2)
    {
      MOOSValFromString(x,val,"x");
      MOOSValFromString(y,val,"y");
      MOOSValFromString(heading,val,"heading");
      MOOSValFromString(speed,val,"speed");
      MOOSValFromString(time,val,"time");
      return(true);
    }
  else
    return(false);
}

