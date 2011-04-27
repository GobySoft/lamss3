#include "str_num.h"

using namespace std;

int str2int_vec(string msg, vector<int>& values)
{
  string bite;
  
  // MOOSTrace("string = %s\n",msg.c_str());
  values.clear();
  int nz = 0;
  while (msg.length() > 0)
    {
      bite = MOOSChomp(msg,",");
      values.push_back(atoi(bite.c_str()));
      nz++;
    }
  // MOOSTrace("number of values = %d\n",nz);
  
  return(nz);
}

bool int_vec2str(vector<int> numbers, string& output)
{
  output = "";
  for(int i=0; i<numbers.size(); i++) 
    {
      if(i > 0)
	output += ",";
      string istr = intToString(numbers[i]); 
      output += istr;
    }
  return(true);
}

int str2dbl_vec(string msg, vector<double>& values)
{
  string bite;
  
  // MOOSTrace("string = %s\n",msg.c_str());
  values.clear();
  int nz = 0;
  while (msg.length() > 0)
    {
      bite = MOOSChomp(msg,",");
      values.push_back(atof(bite.c_str()));
      nz++;
    }
  // MOOSTrace("number of values = %d\n",nz);
  
  return(nz);
}

bool dbl_vec2str(vector<double> numbers, string& output)
{
  output = "";
  for(int i=0; i<numbers.size(); i++) 
    {
      if(i > 0)
	output += ",";
      string istr = doubleToString(numbers[i], 2); // 2 is the precision 123.45
      output += istr;
    }
  return(true);
}

