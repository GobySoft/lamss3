///////////////////////////////////////////////////////////////////////////
//
//   MOOS - Mission Oriented Operating Suite 
//  
//   A suit of Applications and Libraries for Mobile Robotics Research 
//   Copyright (C) 2001-2005 Massachusetts Institute of Technology and 
//   Oxford University. 
//	
//   This software was written by Paul Newman at MIT 2001-2002 and Oxford 
//   University 2003-2005. email: pnewman@robots.ox.ac.uk. 
//	  
//   This file is part of a  MOOS Utility Component. 
//		
//   This program is free software; you can redistribute it and/or 
//   modify it under the terms of the GNU General Public License as 
//   published by the Free Software Foundation; either version 2 of the 
//   License, or (at your option) any later version. 
//		  
//   This program is distributed in the hope that it will be useful, 
//   but WITHOUT ANY WARRANTY; without even the implied warranty of 
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
//   General Public License for more details. 
//			
//   You should have received a copy of the GNU General Public License 
//   along with this program; if not, write to the Free Software 
//   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 
//   02111-1307, USA. 
//
//////////////////////////    END_GPL    //////////////////////////////////


// SixDOFAUV.h: interface for the CSixDOFAUV class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SIXDOFAUV_H__135C4976_AA4F_495D_AD96_0F91C579E194__INCLUDED_)
#define AFX_SIXDOFAUV_H__135C4976_AA4F_495D_AD96_0F91C579E194__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "lamss/lib_newmat10D/newmatap.h"

//using namespace NEWMAT;

#include <MOOS/libMOOS/Utils/MOOSUtils.h>
//#include "SimEntity.h"
#include "SimVehicle.h"

#define SEAWATER_RHO     1025
#define GRAVITY_ACCEL    9.81
#define MAXSTP           10000
#define TINY             1.0e-30
#define SAFETY           0.9
#define PGROW            -0.2
#define PSHRNK           -0.25
#define ERRCON           1.89e-4

#define SIGN(a)    ((a) >= 0.0 ? 1 : -1)
#define SIGN2(a,b) ((b) >= 0.0 ? fabs(a) : -fabs(a))

class CSixDOFAUV  : public CSimVehicle
{
public:
	bool   Initialise();
	double GetX()        {return m_State(1);};
	double GetY()        {return m_State(2);};
	double GetZ()        {return m_State(3);};
	double GetPitch()    {return m_dfPitch;};
	double GetYaw()      {return m_State(5);};
	double GetBodyVelY() {return m_State(6);};
	double GetBodyVelX() {return m_State(7);};
	double GetSpeed()    {return m_dfSpeed;};
	double GetRudder()   {return m_dfRudder;};
	double GetElevator() {return m_dfElevator;};
	double GetPitchRate(){return m_dfPitchRate;};
	double GetYawRate()  {return m_dfYawRate;};
	double GetHeading()  {return m_dfHeading;};
	double GetAltitude() {return m_dfAltitude;};
	double GetDepth()    {return m_dfDepth;};
	double GetTowPosX()  {return m_TowPos(1);};
	double GetTowPosY()  {return m_TowPos(2);};
	double GetTowPosZ()  {return m_TowPos(3);};
	double GetTowVelX()  {return m_TowVel(1);};
	double GetTowVelY()  {return m_TowVel(2);};
	double GetTowVelZ()  {return m_TowVel(3);};
	double GetPos1X()    {return m_Pos1(1);};
	double GetPos1Y()    {return m_Pos1(2);};
	double GetPos1Z()    {return m_Pos1(3);};
	double GetPos2X()    {return m_Pos2(1);};
	double GetPos2Y()    {return m_Pos2(2);};
	double GetPos2Z()    {return m_Pos2(3);};
	double GetPos3X()    {return m_Pos3(1);};
	double GetPos3Y()    {return m_Pos3(2);};
	double GetPos3Z()    {return m_Pos3(3);};
	double GetPos4X()    {return m_Pos4(1);};
	double GetPos4Y()    {return m_Pos4(2);};
	double GetPos4Z()    {return m_Pos4(3);};
	double GetRPM()      {return m_dfDesiredThrust*m_dfMaxRPM/100;}    

	void   SetX(double dfVal)       {m_State(1)   = dfVal;};
	void   SetY(double dfVal)       {m_State(2)   = dfVal;};
	void   SetZ(double dfVal)       {m_State(3)   = dfVal;};
	void   SetYaw(double dfVal)     {m_State(5)   = dfVal;};
	void   SetDepth(double dfVal)   {m_dfDepth    = dfVal;};
	void   SetAltitude(double dfVal){m_dfAltitude = dfVal;};
	void   SetInputPrefix(const std::string & sStr){m_sInputPrefix = sStr;};
	void   SetOutputPrefix(const std::string & sStr){m_sOutputPrefix = sStr;};
	bool   Iterate(double dfTimeNow,double dfDTime);
	bool   Load(std::string sName,STRING_LIST &Params);
	bool   StoreArrayX(std::string sCoord);
	bool   StoreArrayY(std::string sCoord);
	bool   StoreArrayZ(std::string sCoord);
	bool   String2Vector(std::string sCoord,ColumnVector& Shape);

	CSixDOFAUV();
	virtual ~CSixDOFAUV();

	// Debug variables
	double m_u;
	double m_v;
	double m_w;
	double m_delta_v;
	double m_delta_w;
	ColumnVector m_F_th;
	ColumnVector m_F_td;
	ColumnVector m_M_th;
	ColumnVector m_M_td;
	ColumnVector m_F_body;
	ColumnVector m_M_body;

	// Vehicle parameters
	double m_dfHullLength;
	double m_dfHullDiam;
	double m_dfMass;
	double m_dfTrim;
	double m_dfX_cm;
	double m_dfZ_cm;
	double m_dfX_cv;
	double m_dfZ_cv;
	double m_dfI_yy;
	double m_dfI_zz;
	double m_dfX_wq;
	double m_dfX_vr;
	double m_dfX_qq;
	double m_dfX_rr;
	double m_dfY_ur;
	double m_dfZ_uq;
	double m_dfM_uq;
	double m_dfN_ur;
	double m_dfX_udot;
	double m_dfY_rdot;
	double m_dfY_vdot;
	double m_dfZ_qdot;
	double m_dfZ_wdot;
	double m_dfM_wdot;
	double m_dfM_qdot;
	double m_dfN_vdot;
	double m_dfN_rdot;
	double m_dfY_vv;
	double m_dfY_rr;
	double m_dfZ_ww;
	double m_dfZ_qq;
	double m_dfM_ww;
	double m_dfM_qq;
	double m_dfN_vv;
	double m_dfN_rr;
	double m_dfDuctDiam;
	double m_dfDuctChord;
	double m_dfDuctX;
	double m_dfdCLdAlpha;
	double m_dfCDDuctMin;
	double m_dfCDDuctCF;
	double m_dfPropDiam;
	double m_dfTowPointX;
	double m_dfTowPointY;
	double m_dfTowPointZ;
	double m_dfBodyX1;
	double m_dfBodyY1;
	double m_dfBodyZ1;
	double m_dfBodyX2;
	double m_dfBodyY2;
	double m_dfBodyZ2;
	double m_dfBodyX3;
	double m_dfBodyY3;
	double m_dfBodyZ3;
	double m_dfBodyX4;
	double m_dfBodyY4;
	double m_dfBodyZ4;
	double m_dfMaxRPM;
	double m_dfMaxElevator;
	double m_dfMaxRudder;

	// Derived constants
	ColumnVector m_BodyTowPoint;
	ColumnVector m_BodyPoint1;
	ColumnVector m_BodyPoint2;
	ColumnVector m_BodyPoint3;
	ColumnVector m_BodyPoint4;
	ColumnVector m_Pos1;
	ColumnVector m_Pos2;
	ColumnVector m_Pos3;
	ColumnVector m_Pos4;
	double m_dfWeight;
	double m_dfRhoA_pon2;
	double m_dfRhoA_effon2;
	double m_dfCD_body;

	// Dynamic variables
	ColumnVector m_State;
	Matrix m_Cinv;
	Matrix m_Rm2i;
	double m_dfX;
	double m_dfY;
	double m_dfZ;
	double m_dfBodyVelX;
	double m_dfBodyVelY;
	double m_dfCableTension;
	double m_dfDesiredRudder;
	double m_dfDesiredElevator;
	double m_dfDesiredThrust;
	double m_dfRudder;
	double m_dfElevator;
	double m_dfThrust;
	double m_dfYaw;
	double m_dfYawRate;
	double m_dfPitch;
	double m_dfPitchRate;
	double m_dfAltitude;
	double m_dfDepth;
	double m_dfSpeed;
	double m_dfHeading; //in degrees...

	// Actuator state-space model
	Matrix m_Actuator_A;
	ColumnVector m_Actuator_B;
	RowVector m_Actuator_C;

	// Towed-array parameters
	ColumnVector m_ArrayShapeX;
	ColumnVector m_ArrayShapeY;
	ColumnVector m_ArrayShapeZ;
	ColumnVector m_TowPos;
	ColumnVector m_TowVel;

	// Simulation parameters
	double m_dfTideHeight;

protected:
	bool DoModel(double dfDT);
	void Derivatives(double, ColumnVector& Zvec, ColumnVector& Zdot);
	void odeint(ColumnVector& ystart, int nvar, double x1, double x2, double eps, double h1,
		    double hmin, int *nok, int *nbad);
	void rkqs(ColumnVector& y, ColumnVector& dydx, int n, double *x, double htry, double eps,
		  ColumnVector& yscal, double *hdid, double *hnext);
	void rkck(ColumnVector& y, ColumnVector& dydx, int n, double x, double h, ColumnVector& yout,
		  ColumnVector& yerr);
	bool GetTokenValPair(string sLine, string &sTok, string &sVal);
};

#endif // !defined(AFX_SIXDOFAUV_H__135C4976_AA4F_495D_AD96_0F91C579E194__INCLUDED_)
