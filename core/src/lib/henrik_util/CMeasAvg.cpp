// H. Schmidt, henrik@mit.edu 07.02.10
// Laboratory for Autonomous Marine Sensing Systems (LAMSS)
// Massachusetts Institute of Technology 
// 
// CMeasAvg.cpp  
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


#include "CMeasAvg.h"

using namespace std;


//****************************************************************************
//*  MeasAvg::set_RawVal()                                                   *
//*                                                                          *
//*  This method calculates the moving average of a sequence of measurements *
//*                                                                          *
//**************************************************************************** 

void CMeasAvg::configAvg(int length)
{

	len = length;
	num_measurements = 0;
}

void CMeasAvg::setVal(double val)
{

	if (num_measurements < len)
	{	  
		vals.push_back(val);
		num_measurements += 1;
	}
	else
	{
		vals.push_back(val);
		vals.pop_front();
		num_measurements = len;
	}

	double sum = 0.0;

	for (int kk = 0; kk < num_measurements; kk++)
	{   
		sum +=vals[kk];
	}

	if (num_measurements>0)
	  avg = sum/num_measurements;
	else
	  avg = 0;
}


//***************************************************************************
//*  MeasAvg::getAvg()                                                       *
//*                                                                         *
//*  This method returns the value of the moving average                    *
//*                                                                         *
//*************************************************************************** 
double CMeasAvg::getAvg()  
{
  if (num_measurements > 0)
    return (avg);
  else
    return(0.0);
}

//***************************************************************************
//*  CAngleAvg::setVal()                                                     *
//*                                                                         *
//*  This method calculates the moving average of a sequence of angles.     *
//*                                                                         *
//*************************************************************************** 

void CAngleAvg::configAvg(int length)
{

	len = length;
	num_measurements = 0;
}

void CAngleAvg::setVal(double val)
{
	double csum,ssum,s,c;

	if (num_measurements < len)
	{	  
		s = sin(val*M_PI/180.0);
		c = cos(val*M_PI/180.0);
		svals.push_back(s);
		cvals.push_back(c);
		num_measurements += 1;
	}
	else
	{
		s = sin(val*M_PI/180);
		c = cos(val*M_PI/180.0);
		svals.push_back(s);
		cvals.push_back(c);
		svals.pop_front();
		cvals.pop_front();
		num_measurements = len;      
	}

	csum = 0.0;
	ssum = 0.0;

	for (int kk = 0; kk < num_measurements; kk++)
	{   
		csum +=cvals[kk];
		ssum +=svals[kk];
	}

	avg = atan2(ssum,csum)*180.0/M_PI;

//MOOSTrace("ssum = %f, csum = %f, average angle = %f\n",ssum,csum,avg);
}


//***************************************************************************
//*  CAngleAvg::get_FVal()                                                       *
//*                                                                         *
//*  This method returns the value of the moving average                    *
//*                                                                         *
//*************************************************************************** 
double CAngleAvg::getAvg()  
{
  if (num_measurements > 0)
    return (avg);
  else
    return(0.0);
}




