// H. Schmidt, henrik@mit.edu 01.20.10
// Laboratory for Autonomous Marine Sensing Systems (LAMSS)
// Massachusetts Institute of Technology 
// 
// CTarget.cpp  
//
// see the readme file within this directory for information
// pertaining to usage and purpose of this script.
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


#include "CTarget.h"

using namespace std;


//
// CTarget methods and functions
//

bool CTarget::setTargetValues(int tgt_num, bool tgt_on, vector<double> values)
{
  // MOOSTrace("Setting target record  number %d\n",tgt_num); 
  // MOOSTrace("Number of values %d\n",values.size()); 
  //   if (values.size() >= 10)
  //    {
	target_number = tgt_num;
	target_on = tgt_on;
	target_utc = values[0];  
	target_x = values[1];  
	target_y = values[2];  
	target_spd = values[3];
	target_hdg = values[4];  
	target_dep = values[5];  
	target_spl = values[6];
	target_freq = values[7];
	target_bw = values[8];
	dest_id= values[9];
	tone_freq.clear();
	tone_dbl.clear();
	int num_tones=int((values.size()-10)/2);
	// MOOSTrace("Number of tones=%d\n",num_tones);
	for (int i=0; i<num_tones;i++)
	//for (int i=0; i<5;i++)
	  {
	    tone_freq.push_back(values[10+2*i]);
	    tone_dbl.push_back(values[10+2*i+1]);
	  }
	return(true);
	//  }
	//else
	//return(false);
}

bool CTarget::getTargetValues(int& tgt_num, bool& tgt_on, vector<double>& values)
{
  tgt_num = target_number;
  tgt_on = target_on;

  values.clear();
  values.push_back(target_utc);
  values.push_back(target_x);
  values.push_back(target_y);  
  values.push_back(target_spd);
  values.push_back(target_hdg);  
  values.push_back(target_dep);
  values.push_back(target_spl);
  values.push_back(target_freq);
  values.push_back(target_bw);
  values.push_back(dest_id);
  for (int i=0; i<tone_freq.size();i++)
    {
      values.push_back(tone_freq[i]);
      values.push_back(tone_dbl[i]);
    }
  return(true);
}

bool CTarget::printTargetValues()
{
  MOOSTrace("target_number = %d\n",target_number);

  MOOSTrace("target_utc= %f\n",target_utc);
  MOOSTrace("target_x = %f\n",target_x);
  MOOSTrace("target_y = %f\n",target_y);  
  MOOSTrace("target_spd = %f\n",target_spd);
  MOOSTrace("target_hdg = %f\n",target_hdg);  
  MOOSTrace("target_dep = %f\n",target_dep);
  MOOSTrace("target_spl = %f\n",target_spl);
  MOOSTrace("target_freq = %f \n",target_freq);
  MOOSTrace("target_bw = %f\n",target_bw);

  for (int i=0; i<tone_freq.size();i++)
	  {
	    MOOSTrace("tone_freq=%f, tone_dbl=%f\n",tone_freq[i],tone_dbl[i]);
	  }

  return(true);
}

string CTarget::target_id()
{
  string output="TGT_";
  output+= intToString(target_number);
  return(output);
}

string CTarget::target_state()
{
  string output="";

  output += intToString(target_number);
  if (target_on)
    output += ",1,";
  else
    output += ",0,";

  double time_now = MOOSTime();
  double elapsed = time_now - target_utc;
  double tgt_x_now = target_x+(sin(target_hdg*M_PI/180.0)*elapsed*target_spd);
  double tgt_y_now = target_y+(cos(target_hdg*M_PI/180.0)*elapsed*target_spd);
  
  output += doubleToString(time_now,2);
  output += ",";

  output += doubleToString(tgt_x_now,2);
  output += ",";

  output += doubleToString(tgt_y_now,2);
  output += ",";

  output += doubleToString(target_spd,2);
  output += ",";

  output += doubleToString(target_hdg,2);
  output += ",";

  output += doubleToString(target_dep,2);
  output += ",";

  output += doubleToString(target_spl,2);
  output += ",";

  output += doubleToString(target_freq,2);
  output += ",";

  output += doubleToString(target_bw,2);
  output += ",";

  output += doubleToString(dest_id,2);
  output += ",";
 
  for (int i=0; i<tone_freq.size();i++)
    {
      output += doubleToString(tone_freq[i]);
      output += ",";
      output += doubleToString(tone_dbl[i]);
      output += ",";
     }

  return(output);
}
