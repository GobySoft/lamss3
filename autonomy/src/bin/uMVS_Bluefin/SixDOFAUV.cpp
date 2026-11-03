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


// SixDOFAUV.cpp: implementation of the CSixDOFAUV class.
//
//////////////////////////////////////////////////////////////////////
#ifdef _WIN32
	#pragma warning(disable : 4786)
#endif

//#define VERBOSE

#include <math.h>
#include "SixDOFAUV.h"
#include "AcousticTransceiver.h"
#include "SimGlobalHelper.h"

using namespace std;

#ifndef PI
	#define PI 3.141592653589793
#endif

Matrix CrossProduct(Matrix& v1, Matrix& v2)
{
    double a1 = v1(1,1);
    double a2 = v1(2,1);
    double a3 = v1(3,1);
    
    double b1 = v2(1,1);
    double b2 = v2(2,1);
    double b3 = v2(3,1);

    Matrix Cross(3,1);
    Cross<<(a2*b3 - a3*b2)<<
           (a3*b1 - a1*b3)<<
           (a1*b2 - a2*b1);

    return Cross;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CSixDOFAUV::CSixDOFAUV()
{
  // Defaults are for an Odyssey III in long configuration
  m_dfHullLength = 3.56;          // Streamlined body length (m)
  m_dfHullDiam   = 0.53;          // Hull diameter (m)
  m_dfMass       = 600;           // Vehicle mass in water (kg)
  m_dfTrim       = 2.2;           // Vehicle excess buoyancy
  m_dfX_cm       = 0.082;         // X-axis center of mass
  m_dfZ_cm       = -0.015;        // Z-axis center of mass
  m_dfX_cv       = 0.082;         // X-axis center of volume
  m_dfZ_cv       = 0.0;           // Z-axis center of volume
  m_dfI_yy       = 659;           // Body moment of inertia
  m_dfI_zz       = 548;           // Body moment of inertia

  m_dfY_vdot     = -616;          // Cross-flow added mass
  m_dfM_wdot     = -57.1;         // Cross-flow added mass
  m_dfM_qdot     = -458;          // Cross-flow added mass
  m_dfX_udot     = -26.2;         // Cross-flow added mass
  
  m_dfY_vv       = -944;          // Cross-flow drag coefficient
  m_dfY_rr       = -34;           // Cross-flow drag coefficient
  m_dfZ_ww       = -944;          // Cross-flow drag coefficient
  m_dfZ_qq       = 34;            // Cross-flow drag coefficient
  m_dfM_ww       = 33;            // Cross-flow drag coefficient
  m_dfM_qq       = -1027;         // Cross-flow drag coefficient
  m_dfN_vv       = -33;           // Cross-flow drag coefficient
  m_dfN_rr       = -1027;         // Cross-flow drag coefficient

  m_dfDuctDiam   = 0.415;         // Tail duct diameter
  m_dfDuctChord  = 0.12;          // Tail duct chord
  m_dfDuctX      = -1.7;          // Tail duct x-axis displacement (long config)
  m_dfdCLdAlpha  = 5.1;           // Tail duct lift derivative
  m_dfCDDuctMin  = 0.01;          // Min section duct drag coefficient
  m_dfCDDuctCF   = 0.81;          // Cross-flow duct drag coefficient
  m_dfPropDiam   = 0.375;         // Propeller diameter
  m_dfTowPointX  = -2.7;          // Towpoint X coordinate in body frame
  m_dfTowPointY  =  0.0;          // Towpoint Y coordinate in body frame
  m_dfTowPointZ  = -0.3;          // Towpoint Z coordinate in body frame

  m_dfBodyX1     = 2.8;         // Fixed X coordinate in body frame
  m_dfBodyY1     = 0.075;           // Fixed Y coordinate in body frame
  m_dfBodyZ1     = 0.0;           // Fixed Z coordinate in body frame

  m_dfBodyX2     = 2.1;           // Fixed X coordinate in body frame
  m_dfBodyY2     = 0.075;           // Fixed Y coordinate in body frame
  m_dfBodyZ2     = 0.0;           // Fixed Z coordinate in body frame

  m_dfBodyX3     = 2.8;         // Fixed X coordinate in body frame
  m_dfBodyY3     = -0.075;           // Fixed Y coordinate in body frame
  m_dfBodyZ3     = 0.0;           // Fixed Z coordinate in body frame

  m_dfBodyX4     = 2.1;           // Fixed X coordinate in body frame
  m_dfBodyY4     = -0.075;           // Fixed Y coordinate in body frame
  m_dfBodyZ4     = 0.0;           // Fixed Z coordinate in body frame

  m_dfMaxRPM     = 500;           // Maximum propeller RPM
  m_dfMaxElevator= 13;            // Maximum elevator angle (deg)
  m_dfMaxRudder  = 13;            // Maximum rudder angle (deg)

  // Terms obtained by symmetry
  m_dfN_vdot     = -m_dfM_wdot;   // Cross-flow added mass
  m_dfY_rdot     =  m_dfN_vdot;   // Cross-flow added mass
  m_dfZ_qdot     =  m_dfM_wdot;   // Cross-flow added mass
  m_dfZ_wdot     =  m_dfY_vdot;   // Cross-flow added mass
  m_dfN_rdot     =  m_dfM_qdot;   // Cross-flow added mass

  m_dfX_wq       =  m_dfZ_wdot;   // Added mass cross term
  m_dfX_vr       = -m_dfY_vdot;   // Added mass cross term
  m_dfX_qq       = -m_dfZ_qdot;   // Added mass cross term
  m_dfX_rr       = -m_dfY_rdot;   // Added mass cross term
  m_dfY_ur       = -m_dfX_udot;   // Added mass cross term
  m_dfZ_uq       =  m_dfX_udot;   // Added mass cross term
  m_dfM_uq       =  m_dfZ_qdot;   // Added mass cross term
  m_dfN_ur       = -m_dfY_rdot;   // Added mass cross term

}

CSixDOFAUV::~CSixDOFAUV()
{
}

//////////////////////////////////////////////////////////////////////
// Initialise vehicle state
//////////////////////////////////////////////////////////////////////
bool CSixDOFAUV::Initialise()
{
  double A_f  = 1.14; // Figure out how to calculate this !
  double A_p  = m_dfHullDiam*m_dfHullLength;
  double DonL = m_dfHullDiam/m_dfHullLength;
  double LonD = 1/DonL;

  // Weight of vehicle
  m_dfWeight = m_dfMass*GRAVITY_ACCEL;

  // Body lift/drag constant
  m_dfRhoA_pon2 = 0.5*SEAWATER_RHO*A_p; 

  // Axial drag coefficient (check this) 
  m_dfCD_body = 0.004*M_PI*A_p/A_f*(1+60*pow(DonL,3)+0.0025*LonD); //Eq. 4.8

  // Tail duct lift/drag constant
  m_dfRhoA_effon2 = 0.5*SEAWATER_RHO*m_dfDuctDiam*m_dfDuctChord; //Constant of Eq.4.39
  
  Matrix Cmat(5,5);

  // Only five equations here, as we don't consider roll
  Cmat << m_dfMass-m_dfX_udot << 0 << 0 << m_dfMass*m_dfZ_cm << 0
       << 0 << m_dfMass-m_dfY_vdot << 0 << 0 << m_dfMass*m_dfX_cm-m_dfY_rdot
       << 0 << 0 << m_dfMass-m_dfZ_wdot << -m_dfMass*m_dfX_cm-m_dfZ_qdot << 0
       << m_dfMass*m_dfZ_cm << 0 << -m_dfMass*m_dfX_cm-m_dfM_wdot << m_dfI_yy-m_dfM_qdot << 0
       << 0 << m_dfMass*m_dfX_cm-m_dfN_vdot << 0 << 0 << m_dfI_zz-m_dfN_rdot; //Eq. 6.19

  m_Cinv = Cmat.i();

  // MOOS to inertial transformation
  m_Rm2i.ReSize(3,3);

  m_Rm2i << 0 << 1 << 0
	 <<-1 << 0 << 0
	 << 0 << 0 << 1;

  // Actuator state-space model
  m_Actuator_A.ReSize(4,4);
  m_Actuator_B.ReSize(4);
  m_Actuator_C.ReSize(4);

  m_Actuator_A << -25.24 << -15.86 << -9.53 << -4.019
	       <<  16    <<   0    <<  0    <<  0
	       <<   0    <<   8    <<  0    <<  0
	       <<   0    <<   0    <<  4    <<  0;
  m_Actuator_B <<   4    <<   0    <<  0    <<  0;
  m_Actuator_C <<   0    << 0.5208 << -0.775<<  1.005 ; //State space model of tail cone
							//Eq. 7.6 transformed using MATLAB to state space

  // Body point 1
  m_BodyPoint1.ReSize(3);
  m_BodyPoint1 << m_dfBodyX1 << m_dfBodyY1 << m_dfBodyZ1;
  
  // Body point 2
  m_BodyPoint2.ReSize(3);
  m_BodyPoint2 << m_dfBodyX2 << m_dfBodyY2 << m_dfBodyZ2;

  // Body point 3
  m_BodyPoint3.ReSize(3);
  m_BodyPoint3 << m_dfBodyX3 << m_dfBodyY3 << m_dfBodyZ3;
  
  // Body point 4
  m_BodyPoint4.ReSize(3);
  m_BodyPoint4 << m_dfBodyX4 << m_dfBodyY4 << m_dfBodyZ4;

  // Towpoint in body frame
  m_BodyTowPoint.ReSize(3);
  m_BodyTowPoint << m_dfTowPointX << m_dfTowPointY << m_dfTowPointZ;

  // Initialize towpoint position
  m_TowPos.ReSize(3);
  m_TowPos=0;

  // Initialize towpoint velocity
  m_TowVel.ReSize(3);
  m_TowVel=0;

  // Initialize state vector
  m_State.ReSize(18);
  m_State=0;

  // Initialize dynamic variables
  m_dfX=0;
  m_dfY=0;
  m_dfZ=0;
  m_dfBodyVelX=0;
  m_dfBodyVelY=0;
  m_dfCableTension=0;
  m_dfDesiredRudder=0;
  m_dfDesiredElevator=0;
  m_dfDesiredThrust=0;
  m_dfRudder=0;
  m_dfElevator=0;
  m_dfThrust=0;
  m_dfYaw=0;
  m_dfYawRate=0;
  m_dfPitch=0;
  m_dfPitchRate=0;
  m_dfDepth=0;
  m_dfSpeed=0;
  m_dfHeading=0;

  // Temporary until we get the environment working again
  m_dfAltitude=100;
  
  return true;
}

//////////////////////////////////////////////////////////////////////
// Run vehicle dynamic model
//////////////////////////////////////////////////////////////////////
bool CSixDOFAUV::Iterate(double dfTimeNow,double dfDT)
{   
    DoModel(dfDT);

#ifdef VERBOSE
    MOOSTrace("u = %.2f\n",m_u);
    MOOSTrace("v = %.2f\n",m_v);
    MOOSTrace("w = %.2f\n",m_w);
    MOOSTrace("delta_v = %.2f\n",m_delta_v);
    MOOSTrace("delta_w = %.2f\n",m_delta_w);
    MOOSTraceMatrix(m_F_th,"Thrust forces");
    MOOSTraceMatrix(m_F_td,"Tailduct forces");
    MOOSTraceMatrix(m_M_th,"Thrust moments");
    MOOSTraceMatrix(m_M_td,"Tailduct moments");
    MOOSTraceMatrix(m_F_body,"Body forces");
    MOOSTraceMatrix(m_M_body,"Body moments");
#endif    

    return true;
}

//////////////////////////////////////////////////////////////////////
// Integrate the non-linear model and do some housekeeping
//////////////////////////////////////////////////////////////////////
bool CSixDOFAUV::DoModel(double dfDT)
{
  int nbad,nok;
  
  // Do integration here
  odeint(m_State,18,0,dfDT,1.0e-2,0.1,1e-9,&nok,&nbad); // Numerical recepies in C

  // Calculate actuator positions
  m_dfRudder = MOOSRad2Deg((m_Actuator_C*m_State.Rows(11,14)).AsScalar());
  m_dfElevator = MOOSRad2Deg((m_Actuator_C*m_State.Rows(15,18)).AsScalar());

  // Enforce actuator limits
  MOOSAbsLimit(m_dfRudder,m_dfMaxRudder);  
  MOOSAbsLimit(m_dfElevator,m_dfMaxElevator);  
  
  // Look after angles...
  m_State(4) = MOOS_ANGLE_WRAP(m_State(4));
  m_State(5) = MOOS_ANGLE_WRAP(m_State(5));
  
  // Figure out our heading -ve around z axis from x axis...
  m_dfHeading = HeadingFromYaw(m_State(5));
  
  // Figure out speed
  m_dfSpeed = sqrt(pow(m_State(6),2)+pow(m_State(7),2)+pow(m_State(8),2));

  // Angular rates - no finite differences!
  m_dfPitchRate=-m_State(9);
  m_dfYawRate=m_State(10);

  // Miscellaneous conversions from Rentschler's coordinate system
  m_dfDepth=-m_State(3);
  m_dfPitch=-m_State(4);

  ////////////////////////////////////////////////////////
  // Environmental properties...
  //  some things are a function of environment and so we need
  //  to access it to resolve some properties....
  if(m_pEnvironment!=NULL)
  {
      //m_dfDepth = m_pEnvironment->GetDepth(GetZ());
      //m_dfAltitude = m_pEnvironment->GetAltitude(GetX(),GetY(),GetZ());
  }
  
  return true;
}

//////////////////////////////////////////////////////////////////////
// Non-linear state-derivative calculations
//////////////////////////////////////////////////////////////////////
void CSixDOFAUV::Derivatives(double t,ColumnVector& Zvec,ColumnVector& Zdot)
{
  // LOCAL state components
  double x          = Zvec(1);
  double y          = Zvec(2);
  double z          = Zvec(3);
  double pitch      = Zvec(4);
  double yaw        = Zvec(5);
  double u          = Zvec(6);
  double v          = Zvec(7);
  double w          = Zvec(8);
  double q          = Zvec(9);
  double r          = Zvec(10);
  ColumnVector rvec = Zvec.Rows(11,14);
  ColumnVector evec = Zvec.Rows(15,18);
  double rudder     = MOOSDeg2Rad(m_dfRudder);
  double elevator   = MOOSDeg2Rad(m_dfElevator);

  // Precompute trig quantities
  double sine = sin(-elevator);
  double cose = cos(-elevator);
  double sinr = sin(rudder);
  double cosr = cos(rudder);  
  double sinp = sin(pitch);
  double cosp = cos(pitch);
  double siny = sin(yaw);
  double cosy = cos(yaw);

  Matrix Rang(2,2);
  Matrix Ri2b(3,3);

  // Angular velocity transformation
  Rang << 1 << 0
       << 0 << 1/cosp;

  // Inertial to body frame transformation
  Ri2b << cosp*cosy << cosp*siny << -sinp
       << -siny     << cosy      << 0
       << sinp*cosy << sinp*siny << cosp;

  // Body velocity in inertial frame
  ColumnVector xdot=m_Rm2i.t()*Ri2b.t()*Zvec.Rows(6,8);
  ColumnVector Edot=Rang*Zvec.Rows(9,10);
  ColumnVector AngVel(3);
  AngVel << 0 << q << r;

  // Body point 1 in inertial frame
  m_Pos1=Zvec.Rows(1,3)+m_Rm2i.t()*Ri2b.t()*m_BodyPoint1;

  // Body point 2 in inertial frame
  m_Pos2=Zvec.Rows(1,3)+m_Rm2i.t()*Ri2b.t()*m_BodyPoint2;

  // Body point 3 in inertial frame
  m_Pos3=Zvec.Rows(1,3)+m_Rm2i.t()*Ri2b.t()*m_BodyPoint3;

  // Body point 4 in inertial frame
  m_Pos4=Zvec.Rows(1,3)+m_Rm2i.t()*Ri2b.t()*m_BodyPoint4;

  // Towpoint coordinates in inertial frame
  m_TowPos=Zvec.Rows(1,3)+m_Rm2i.t()*Ri2b.t()*m_BodyTowPoint;
  
  // Towpoint velocities in inertial frame
  m_TowVel=xdot+m_Rm2i.t()*Ri2b.t()*CrossProduct(AngVel,m_BodyTowPoint);

  ColumnVector T_cab(3);
  ColumnVector M_cab(3);
  
  // Process a towed array if present
  if (m_ArrayShapeX.Nrows() && m_ArrayShapeY.Nrows() && m_ArrayShapeZ.Nrows())
  {    
    // Calculate unit force vector at array tow point
    // Note sign change in Z axis from array coordinate system
    T_cab << m_ArrayShapeX(2)-m_ArrayShapeX(1)
	  << m_ArrayShapeY(2)-m_ArrayShapeY(1)
	  << m_ArrayShapeZ(2)-m_ArrayShapeZ(1);
    // H. Schmidt, 3/5/2011
    // Changed rhs coordinates after change in uSimTowedArray (pArraySim) 
    //	  << m_ArrayShapeZ(1)-m_ArrayShapeZ(2);
    
    T_cab = T_cab/T_cab.NormFrobenius();
    
    // Cable tension in inertial frame
    T_cab = m_dfCableTension*T_cab; //Tow point frame
    
    // Cable tension in body frame
    T_cab = Ri2b*m_Rm2i*T_cab; //Transformed to vehicle body frame
    
    // Cable moments in body frame
    M_cab = CrossProduct(m_BodyTowPoint,T_cab); //Moment vector
  }
  else
  {
    T_cab=0;
    M_cab=0;
  }

  // Squared velocities for drag calculations
  double uabsu = u*fabs(u);
  double vabsv = v*fabs(v);
  double wabsw = w*fabs(w);
  double qabsq = q*fabs(q);
  double rabsr = r*fabs(r);

  // Body angles of attack
  // See page 21
  double delta_w = atan2(w,u);
  double delta_v = atan2(-v,u);

  // Body lift coefficients
  double CL_pitch = 0.131*delta_w; //Eq 4.31
  double CL_yaw   = 0.131*delta_v;
  double CM_pitch = 0.123*delta_w;
  double CM_yaw   = 0.123*delta_v;

  ColumnVector F_body(3);
  ColumnVector M_body(3);
  ColumnVector Length(3);
  
  // Forces involved in body moments
  F_body <<  0  
	 <<  m_dfRhoA_pon2*CM_yaw*uabsu
	 << -m_dfRhoA_pon2*CM_pitch*uabsu;
   
  // Body moments (Munk moment etc)
  Length << m_dfHullLength << 0 << 0;
  M_body = CrossProduct(Length,F_body);

  // Body forces (drag and lift)
  F_body << -m_dfRhoA_pon2*m_dfCD_body*uabsu
	 <<  m_dfRhoA_pon2*CL_yaw*uabsu
	 << -m_dfRhoA_pon2*CL_pitch*uabsu;

  // Propeller RPM from percentage thrust
  double rpm = GetRPM(); 
  double Tp,Up,J,Kt;

  if (rpm==0) Tp=0;
  else
  {
    //Converting propeller rpm to Force
    //Eq. 4.55
    Up=Zvec(6)*0.9;
    J=Up/m_dfPropDiam/fabs(rpm);
    Kt=0.035-0.3*J;
    Tp=rpm*fabs(rpm)*Kt*pow(m_dfPropDiam,4);
  }

  ColumnVector DuctPos(3);
  ColumnVector F_th(3);
  ColumnVector M_th(3);

  // Thrust forces in body frame
  F_th << cosr*cose << sinr << -sine*cosr;
  F_th=Tp*F_th;

  // Thrust moments in body frame
  DuctPos << m_dfDuctX << 0 << 0;
  M_th=CrossProduct(DuctPos,F_th);

  ColumnVector DuctVel_b(3);
  ColumnVector DuctVel_r(3);
  Matrix Rb2r(3,3);

  // Body to ring frame transformation
  //Eq. 4.47
  Rb2r <<  cosr*cose << sinr << -cosr*sine
       << -sinr*cose << cosr <<  sinr*sine
       <<  sine      << 0    <<  cose;

  // Tail duct velocity calculation
  DuctVel_b  = Zvec.Rows(6,8)+CrossProduct(AngVel,DuctPos);
  DuctVel_r  = Rb2r*DuctVel_b;
  double nt1 = DuctVel_r.NormFrobenius()+TINY;

  ColumnVector Forward(3);
  ColumnVector Reverse(3);
  Forward <<  1 << 0 << 0;
  Reverse << -1 << 0 << 0;

  // Tail duct angle of attack
  double Alpha = asin(CrossProduct(DuctVel_r,Reverse).NormFrobenius()/nt1);

  // Tail duct lift coefficient
  double CL_duct = m_dfdCLdAlpha*Alpha+m_dfCDDuctCF*Alpha*fabs(Alpha);

  // Tail duct lift force
  double L_duct = m_dfRhoA_effon2*pow(u,2)*CL_duct;

  // Tail duct drag coefficient
  double CD_duct = m_dfCDDuctMin+pow(CL_duct,2)*m_dfDuctChord/(M_PI*m_dfDuctDiam*0.9);

  // Tail duct drag force
  double D_duct = m_dfRhoA_effon2*pow(u,2)*CD_duct;

  Matrix Rl2r(3,3);
  ColumnVector F_td(3);
  ColumnVector M_td(3);

  // Ring to body-frame transformation
  Matrix t2  = CrossProduct(DuctVel_r,Forward);
  Matrix t3  = CrossProduct(DuctVel_r,t2);
  double nt2 = t2.NormFrobenius()+TINY;
  double nt3 = t3.NormFrobenius()+TINY;
  Rl2r.Column(1) = DuctVel_r/nt1;
  Rl2r.Column(2) = t2/nt2;
  Rl2r.Column(3) = t3/nt3;

  // Tail duct forces in body-frame
  F_td << -D_duct << 0 << -L_duct;
  F_td = Rb2r.t()*Rl2r*F_td;

  // Tail duct moments in body-frame
  M_td = CrossProduct(DuctPos,F_td);

  // Vehicle buoyancy
  double B_g = m_dfWeight+m_dfTrim*GRAVITY_ACCEL;

  // Buoyancy at surface
  double b = -SIGN(z)*fmin(fabs(z),m_dfHullDiam/2);
  double beta = acos(b/m_dfHullDiam*2);
  double disp_frac = ((M_PI-beta)+2*b*sin(beta)/m_dfHullDiam)/M_PI;
  double B = B_g*disp_frac;

  // Hydrostatics
  double X_hs = (m_dfWeight-B)*sinp;
  double Z_hs = (B-m_dfWeight)*cosp;
  double M_hs = (m_dfWeight*m_dfX_cm-B*m_dfX_cv)*cosp+m_dfWeight*m_dfZ_cm*sinp;
 
  // Buoyancy and drag at bottom
  if (m_dfAltitude < m_dfHullDiam/2)
  {
    b = -SIGN(m_dfAltitude)*fmin(fabs(m_dfAltitude),m_dfHullDiam/2);
    beta = acos(b/m_dfHullDiam*2);
    disp_frac = ((M_PI-beta)+2*b*sin(beta)/m_dfHullDiam)/M_PI;
    X_hs = X_hs-10*uabsu*disp_frac;
    Z_hs = Z_hs+(B_g-10*wabsw)*disp_frac;
  }

#ifdef VERBOSE
  m_u=u;
  m_v=v;
  m_w=w;
  m_delta_v=delta_v;
  m_delta_w=delta_w;
  m_F_th=F_th;
  m_F_td=F_td;
  m_M_th=M_th;
  m_M_td=M_td;
  m_F_body=F_body;
  m_M_body=M_body;
#endif

  // Body-referenced external forces (functions of vehicle state)
  // Eq. 6.13 - 6.18
  double SigmaX = X_hs+(m_dfX_wq-m_dfMass)*w*q+(m_dfX_vr+m_dfMass)*v*r+
    (m_dfX_qq+m_dfMass*m_dfX_cm)*q*q+(m_dfX_rr+m_dfMass*m_dfX_cm)*r*r+F_body(1)+
    F_th(1)+F_td(1)+T_cab(1);
  
  double SigmaY = m_dfY_vv*vabsv+m_dfY_rr*rabsr+(m_dfY_ur-m_dfMass)*u*r-
    m_dfMass*r*q*m_dfZ_cm+F_body(2)+F_th(2)+F_td(2)+T_cab(2);
  
  double SigmaZ = Z_hs+m_dfZ_ww*wabsw+m_dfZ_qq*qabsq+(m_dfZ_uq+m_dfMass)*u*q+
    m_dfMass*q*q*m_dfZ_cm+F_body(3)+F_th(3)+F_td(3)+T_cab(3);
  
  double SigmaM = M_hs+m_dfM_ww*wabsw+m_dfM_qq*qabsq+(m_dfM_uq-m_dfMass*m_dfX_cm)*u*q+
    m_dfMass*m_dfZ_cm*(r*v-q*w)+M_body(2)+M_th(2)+M_td(2)+M_cab(2);
  
  double SigmaN = m_dfN_vv*vabsv+m_dfN_rr*rabsr+(m_dfN_ur-m_dfMass*m_dfX_cm)*u*r+
    M_body(3)+M_th(3)+M_td(3)+M_cab(3);

  // Body force vector
  ColumnVector Fvec(5);
  Fvec << SigmaX << SigmaY << SigmaZ << SigmaM << SigmaN;

  // Actuator derivatives
  ColumnVector edot = m_Actuator_A*evec+m_Actuator_B*m_dfDesiredElevator;
  ColumnVector rdot = m_Actuator_A*rvec+m_Actuator_B*m_dfDesiredRudder;
  
  // Total state derivative
  Zdot.Rows(1,3)   = xdot;
  Zdot.Rows(4,5)   = Edot;
  Zdot.Rows(6,10)  = m_Cinv*Fvec;
  Zdot.Rows(11,14) = rdot;
  Zdot.Rows(15,18) = edot;
}

//////////////////////////////////////////////////////////////////////
// Driver for 5th order Runge-Kutter integration
//////////////////////////////////////////////////////////////////////
void CSixDOFAUV::odeint(ColumnVector& ystart, int nvar, double x1, double x2,
			double eps, double h1, double hmin, int *nok, int *nbad)
{
	int nstp,i;
	double x,hnext,hdid,h;

	ColumnVector yscal(nvar);
	ColumnVector dydx(nvar);
	ColumnVector y(nvar);

	x=x1;
	h=SIGN2(h1,x2-x1);
	*nok = (*nbad) = 0;
	for (i=1;i<=nvar;i++) y(i)=ystart(i);
	for (nstp=1;nstp<=MAXSTP;nstp++) {
		Derivatives(x,y,dydx);
		for (i=1;i<=nvar;i++)
			yscal(i)=fabs(y(i))+fabs(dydx(i)*h)+TINY;
		if ((x+h-x2)*(x+h-x1) > 0.0) h=x2-x;
		rkqs(y,dydx,nvar,&x,h,eps,yscal,&hdid,&hnext);
		if (hdid == h) ++(*nok); else ++(*nbad);
		if ((x-x2)*(x2-x1) >= 0.0) {
			for (i=1;i<=nvar;i++) ystart(i)=y(i);
			return;
		}
		if (fabs(hnext) <= hmin) MOOSTrace("Step size too small in odeint!\n");
		h=hnext;
	}
	MOOSTrace("Too many steps in routine odeint!\n");
}

//////////////////////////////////////////////////////////////////////
// Fifth-order Runge-Kutta step control
//////////////////////////////////////////////////////////////////////
void CSixDOFAUV::rkqs(ColumnVector& y, ColumnVector& dydx, int n, double *x, double htry,
		      double eps, ColumnVector& yscal, double *hdid, double *hnext)
{
	int i;
	double errmax,h,htemp,xnew;

	ColumnVector yerr(n);
	ColumnVector ytemp(n);
	h=htry;
	for (;;) {
		rkck(y,dydx,n,*x,h,ytemp,yerr);
		errmax=0.0;
		for (i=1;i<=n;i++) errmax=fmax(errmax,fabs(yerr(i)/yscal(i)));
		errmax /= eps;
		if (errmax <= 1.0) break;
		htemp=SAFETY*h*pow(errmax,PSHRNK);
		h=(h >= 0.0 ? fmax(htemp,0.1*h) : fmin(htemp,0.1*h));
		xnew=(*x)+h;
		if (xnew == *x) MOOSTrace("Stepsize underflow in rkqs!\n");
	}
	if (errmax > ERRCON) *hnext=SAFETY*h*pow(errmax,PGROW);
	else *hnext=5.0*h;
	*x += (*hdid=h);
	for (i=1;i<=n;i++) y(i)=ytemp(i);
}

//////////////////////////////////////////////////////////////////////
// Cash-Karp Runger-Kutter step
//////////////////////////////////////////////////////////////////////
void CSixDOFAUV::rkck(ColumnVector& y, ColumnVector& dydx, int n, double x,
		      double h, ColumnVector& yout, ColumnVector& yerr)
{
	int i;
	static double a2=0.2,a3=0.3,a4=0.6,a5=1.0,a6=0.875,b21=0.2,
	        b31=3.0/40.0,b32=9.0/40.0,b41=0.3,b42 = -0.9,b43=1.2,
		b51 = -11.0/54.0, b52=2.5,b53 = -70.0/27.0,b54=35.0/27.0,
		b61=1631.0/55296.0,b62=175.0/512.0,b63=575.0/13824.0,
		b64=44275.0/110592.0,b65=253.0/4096.0,c1=37.0/378.0,
		c3=250.0/621.0,c4=125.0/594.0,c6=512.0/1771.0,
		dc5 = -277.00/14336.0;
	double dc1=c1-2825.0/27648.0,dc3=c3-18575.0/48384.0,
		dc4=c4-13525.0/55296.0,dc6=c6-0.25;

	ColumnVector ak2(n);
	ColumnVector ak3(n);
	ColumnVector ak4(n);
	ColumnVector ak5(n);
	ColumnVector ak6(n);
	ColumnVector ytemp(n);

	for (i=1;i<=n;i++)
		ytemp(i)=y(i)+b21*h*dydx(i);
	Derivatives(x+a2*h,ytemp,ak2);
	for (i=1;i<=n;i++)
		ytemp(i)=y(i)+h*(b31*dydx(i)+b32*ak2(i));
	Derivatives(x+a3*h,ytemp,ak3);
	for (i=1;i<=n;i++)
		ytemp(i)=y(i)+h*(b41*dydx(i)+b42*ak2(i)+b43*ak3(i));
	Derivatives(x+a4*h,ytemp,ak4);
	for (i=1;i<=n;i++)
		ytemp(i)=y(i)+h*(b51*dydx(i)+b52*ak2(i)+b53*ak3(i)+b54*ak4(i));
	Derivatives(x+a5*h,ytemp,ak5);
	for (i=1;i<=n;i++)
		ytemp(i)=y(i)+h*(b61*dydx(i)+b62*ak2(i)+b63*ak3(i)+b64*ak4(i)+b65*ak5(i));
	Derivatives(x+a6*h,ytemp,ak6);
	for (i=1;i<=n;i++)
		yout(i)=y(i)+h*(c1*dydx(i)+c3*ak3(i)+c4*ak4(i)+c6*ak6(i));
	for (i=1;i<=n;i++)
		yerr(i)=h*(dc1*dydx(i)+dc3*ak3(i)+dc4*ak4(i)+dc5*ak5(i)+dc6*ak6(i));
}

//////////////////////////////////////////////////////////////////////
// Customize each vehicle model from the moos file
//////////////////////////////////////////////////////////////////////
bool CSixDOFAUV::Load(std::string sName,STRING_LIST &sParams)
{
	STRING_LIST::iterator p;

	MOOSTrace("Customizing vehicle %s:\n",sName.c_str());

	for(p=sParams.begin();p!=sParams.end();p++)
	{
		string sLine,sTok,sVal;
		sLine=*p;

		if(GetTokenValPair(sLine,sTok,sVal))
		{
			if(MOOSStrCmp(sTok,sName+"_HullLength"))
			{
			  m_dfHullLength = atof(sVal.c_str());
			}
			if(MOOSStrCmp(sTok,sName+"_HullDiam"))
			{
			  m_dfHullDiam = atof(sVal.c_str());
			}
			if(MOOSStrCmp(sTok,sName+"_Mass"))
			{
			  m_dfMass = atof(sVal.c_str());
			}
			if(MOOSStrCmp(sTok,sName+"_Trim"))
			{
			  m_dfTrim = atof(sVal.c_str());
			}
			if(MOOSStrCmp(sTok,sName+"_X_cm"))
			{
			  m_dfX_cm = atof(sVal.c_str());
			}
			if(MOOSStrCmp(sTok,sName+"_Z_cm"))
			{
			  m_dfZ_cm = atof(sVal.c_str());
			}
			if(MOOSStrCmp(sTok,sName+"_X_cv"))
			{
			  m_dfX_cv = atof(sVal.c_str());
			}
			if(MOOSStrCmp(sTok,sName+"_Z_cv"))
			{
			  m_dfZ_cv = atof(sVal.c_str());
			}
			if(MOOSStrCmp(sTok,sName+"_I_yy"))
			{
			  m_dfI_yy = atof(sVal.c_str());
			}
			if(MOOSStrCmp(sTok,sName+"_I_zz"))
			{
			  m_dfI_zz = atof(sVal.c_str());
			}
			if(MOOSStrCmp(sTok,sName+"_X_udot"))
			{
			  m_dfX_udot = atof(sVal.c_str());
			}
			if(MOOSStrCmp(sTok,sName+"_Y_vdot"))
			{
			  m_dfY_vdot = atof(sVal.c_str());
			}
			if(MOOSStrCmp(sTok,sName+"_M_wdot"))
			{
			  m_dfM_wdot = atof(sVal.c_str());
			}
			if(MOOSStrCmp(sTok,sName+"_M_qdot"))
			{
			  m_dfM_qdot = atof(sVal.c_str());
			}
			if(MOOSStrCmp(sTok,sName+"_Y_vv"))
			{
			  m_dfY_vv = atof(sVal.c_str());
			}
			if(MOOSStrCmp(sTok,sName+"_Y_rr"))
			{
			  m_dfY_rr = atof(sVal.c_str());
			}
			if(MOOSStrCmp(sTok,sName+"_Z_ww"))
			{
			  m_dfZ_ww = atof(sVal.c_str());
			}
			if(MOOSStrCmp(sTok,sName+"_Z_qq"))
			{
			  m_dfZ_qq = atof(sVal.c_str());
			}
			if(MOOSStrCmp(sTok,sName+"_M_ww"))
			{
			  m_dfM_ww = atof(sVal.c_str());
			}
			if(MOOSStrCmp(sTok,sName+"_M_qq"))
			{
			  m_dfM_qq = atof(sVal.c_str());
			}
			if(MOOSStrCmp(sTok,sName+"_N_vv"))
			{
			  m_dfN_vv = atof(sVal.c_str());
			}
			if(MOOSStrCmp(sTok,sName+"_N_rr"))
			{
			  m_dfN_rr = atof(sVal.c_str());
			}
			if(MOOSStrCmp(sTok,sName+"_DuctDiam"))
			{
			  m_dfDuctDiam = atof(sVal.c_str());
			}
			if(MOOSStrCmp(sTok,sName+"_DuctChord"))
			{
			  m_dfDuctChord = atof(sVal.c_str());
			}
			if(MOOSStrCmp(sTok,sName+"_DuctX"))
			{
			  m_dfDuctX = atof(sVal.c_str());
			}
			if(MOOSStrCmp(sTok,sName+"_dCLdAlpha"))
			{
			  m_dfdCLdAlpha = atof(sVal.c_str());
			}
			if(MOOSStrCmp(sTok,sName+"_CDDuctMin"))
			{
			  m_dfCDDuctMin = atof(sVal.c_str());
			}
			if(MOOSStrCmp(sTok,sName+"_CDDuctCF"))
			{
			  m_dfCDDuctCF = atof(sVal.c_str());
			}
			if(MOOSStrCmp(sTok,sName+"_PropDiam"))
			{
			  m_dfPropDiam = atof(sVal.c_str());
			}
			if(MOOSStrCmp(sTok,sName+"_TowPointX"))
			{
			  m_dfTowPointX = atof(sVal.c_str());
			}
			if(MOOSStrCmp(sTok,sName+"_TowPointY"))
			{
			  m_dfTowPointY = atof(sVal.c_str());
			}
			if(MOOSStrCmp(sTok,sName+"_TowPointZ"))
			{
			  m_dfTowPointZ = atof(sVal.c_str());
			}
			if(MOOSStrCmp(sTok,sName+"_MaxElevator"))
			{
			  m_dfMaxElevator = atof(sVal.c_str());
			}
			if(MOOSStrCmp(sTok,sName+"_MaxRudder"))
			{
			  m_dfMaxRudder = atof(sVal.c_str());
			}
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////
// Get a token value pair from config line
//////////////////////////////////////////////////////////////////////
bool CSixDOFAUV::GetTokenValPair(string sLine, string &sTok, string &sVal)
{
    if(sLine.find("=")!=string::npos)
    {
        MOOSRemoveChars(sLine," \t\r");
        sTok = MOOSChomp(sLine,"=");
        sVal = sLine;
        return true;
    }
    else
    {
        return false;
    } 
}

//////////////////////////////////////////////////////////////////////
// Store array shape into column vectors
//////////////////////////////////////////////////////////////////////
bool CSixDOFAUV::StoreArrayX(std::string sCoord)
{
  m_ArrayShapeX.ReSize(2);
  string sVal=MOOSChomp(sCoord,",");
  m_ArrayShapeX(1)=atof(sVal.c_str());
  sVal=MOOSChomp(sCoord,",");
  m_ArrayShapeX(2)=atof(sVal.c_str());
  return true;
}
bool CSixDOFAUV::StoreArrayY(std::string sCoord)
{
  m_ArrayShapeY.ReSize(2);
  string sVal=MOOSChomp(sCoord,",");
  m_ArrayShapeY(1)=atof(sVal.c_str());
  sVal=MOOSChomp(sCoord,",");
  m_ArrayShapeY(2)=atof(sVal.c_str());
  return true;
}
bool CSixDOFAUV::StoreArrayZ(std::string sCoord)
{
  m_ArrayShapeZ.ReSize(2);
  string sVal=MOOSChomp(sCoord,",");
  m_ArrayShapeZ(1)=atof(sVal.c_str());
  sVal=MOOSChomp(sCoord,",");
  m_ArrayShapeZ(2)=atof(sVal.c_str());
  return true;
}

//////////////////////////////////////////////////////////////////////
// Convert a comma-delimited MOOS string into a column vector
//////////////////////////////////////////////////////////////////////
bool CSixDOFAUV::String2Vector(std::string sCoord,ColumnVector& Shape)
{
  Shape.ReSize(2);

  string sVal=MOOSChomp(sCoord,",");
  Shape(1)=atof(sVal.c_str());
  sVal=MOOSChomp(sCoord,",");
  Shape(2)=atof(sVal.c_str());
  
  MOOSTraceMatrix(Shape,"Shape vector\n");

  return true;
}
