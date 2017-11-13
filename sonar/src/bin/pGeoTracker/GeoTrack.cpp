#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>


//deprecated (tes)
//#include <values.h>
// replaced by:
#include <limits.h>
#include <float.h>
#define MAXFLOAT FLT_MAX

#include "GeoTrack.h"

using namespace std;

CGeoTrack::CGeoTrack()
{
  
  nvar = NUM_P;
  
  search_times = 10000;
  //search_eps = 1.0e-15;
  search_eps = 1.0e-9;
  mu1 = 0.0002;
  // mu1 = 0.01;
  mu2 = 0.9;
  
  //allocate memory for our measurements
  vposx    = new double[MAX_BEARINGS];
  vposy    = new double[MAX_BEARINGS];
  bearings = new double[MAX_BEARINGS];
  dt       = new double[MAX_BEARINGS];

  meas_num = 0;
  solution = false;
  zindex = 0;
  reset_init = 0;
}

CGeoTrack::~CGeoTrack()
{
  //give back memory
  delete [] vposx;
  delete [] vposy;
  delete [] bearings;
  delete [] dt;
}

void CGeoTrack::reset()
{
  meas_num=0;
  solution = false;
  zindex = 0;
  reset_init = 0;
  cout << "GeoTrack reset()\n"<<endl;
}

void CGeoTrack::reset_solution()
{
  solution = false;
  reset_init = 0;
  zindex = 0;
  cout << "GeoTrack solution reset()\n"<<endl;
}

void CGeoTrack::set_range(double rg,double dt)
{
  range_guess = rg;
  dtguess=dt;
}

void CGeoTrack::set_range_max(double rmax)
{
  range_max = rmax;
}

void CGeoTrack::set_speed_max(double vmax)
{
  speed_max = vmax;
}

void CGeoTrack::set_reset_init(int res)
{
  reset_init = res;
}

//how many observations do we use?
void CGeoTrack::set_Z_length(int len)
{
  zlength = len;
}

void CGeoTrack::set_guess(double xdot,double ydot,double tx,double ty,double dt)
{
  xguess = tx;
  yguess = ty;
  xdotguess = xdot;
  ydotguess = ydot;
  dtguess = dt;
  cout << "guess = "<< xguess<<","<<yguess<<","<<xdot<<","<<ydot<<","<<dt<<endl;
}

void CGeoTrack::set_measurement(double veh_x, double veh_y, double bear,double diff)
{
  int i;
  // rotate buffer

  if (zindex > zlength)
    {
     zindex = zlength;
     for (i=0; i < zindex-1; i++)
       { 
	 vposx[i]=vposx[i+1];
	 vposy[i]=vposy[i+1];
	 bearings[i]=bearings[i+1];
	 dt[i]=dt[i+1];
       }
    }

    vposx[zindex] = veh_x;
    vposy[zindex] = veh_y;
    bearings [zindex] = bear;
    dt[zindex] = diff;
    zindex++;
    meas_num++;

    if(meas_num >= zlength)
      meas_num = zlength;
    cout<<"zindex = "<<zindex-1<<" vpos_x = "<<veh_x<<" vposy = "<<veh_y<<" bear = "<<bear<<" diff "<<diff<<endl;
    cout<<"meas_num = "<<meas_num<<endl;
}

status CGeoTrack::get_track()
{
  bool silent = true;
  
  double x[nvar];	
  double new_range,x_r,y_r,x_v,y_v, sor;

  new_range = 0;
  status stat;

  /*
  x[0] = xguess;
  x[1] = yguess;
  x[2] = xdotguess;
  x[3] = ydotguess;
  */

  dtinit = dtguess;


  if(!solution)
    {
      if (reset_init == 0)
	{
	  //initial guess
	  x[0] = vposx[zindex-1] + range_guess*cos(bearings[zindex-1]);
	  x[1] = vposy[zindex-1] + range_guess*sin(bearings[zindex-1]);
	  x[2] = xdotguess;
	  x[3] = ydotguess;
	  new_range = range_guess;
	}
      else
	{
	  x[0] = vposx[zindex-1] + range_guess*cos(bearings[zindex-1]);
	  x[1] = vposy[zindex-1] + range_guess*sin(bearings[zindex-1]);
	  x[2] = xdotguess;
	  x[3] = ydotguess;
	  new_range = range_guess;
	}
    }
  else
    {
      int ii=zindex-1;
      double prev_speed = sqrt(pow(xdot,2)+pow(ydot,2));
      double prev_range = sqrt(pow(xzero-vposx[ii],2)+pow(yzero-vposy[ii],2));
      if ( (prev_speed < speed_max) && (prev_range < range_max ) && 
	   (prev_range > 0.05*range_max))
	{ 
	  cout << "Solution - Case 1" << endl;
	  x_r = xzero-vposx[ii];
	  y_r = yzero-vposy[ii];
	  x_v = xdot*(dt[ii] - dt[ii-1]);
	  y_v = ydot*(dt[ii] - dt[ii-1]);
	  // SOR
	  sor = 1.2;
	  new_range = prev_range + sor*(x_r*x_v + y_r*y_v)/prev_range;
	  x[0] = vposx[zindex-1] + new_range*cos(bearings[zindex-1]);
	  x[1] = vposy[zindex-1] + new_range*sin(bearings[zindex-1]);
	  //x[0] = xzero + xdot*(dt[ii] - dt[ii-1]);
	  //x[1] = yzero + ydot*(dt[ii] - dt[ii-1]);
	  x[2] = xdot;
	  x[3] = ydot;
	  //new_range = sqrt(pow(xzero-vposx[ii],2)+pow(yzero-vposy[ii],2));
	}
      else if (prev_speed > speed_max)
	{
	  cout << "Solution - Case 2" << endl;
	  //new_range = range_guess*0.5;
	  new_range = range_guess*speed_max/prev_speed;
	  x[0] = vposx[zindex-1] + new_range*cos(bearings[zindex-1]);
	  x[1] = vposy[zindex-1] + new_range*sin(bearings[zindex-1]);
	  x[2] = xdot/fabs(xdot);
	  x[3] = ydot/fabs(ydot);
	  //x[2] = xdot*speed_max/prev_speed;
	  //x[3] = ydot*speed_max/prev_speed;
	  //x[2] = 1.0;
	  //x[3] = 1.0;
	}
      else
	{
	  cout << "Solution - Case 3" << endl;
	  new_range = 0.5*range_guess;
	  //new_range = range_guess;
	  x[0] = vposx[zindex-1] + new_range*cos(bearings[zindex-1]);
	  x[1] = vposy[zindex-1] + new_range*sin(bearings[zindex-1]);
	  x[2] = xdot/fabs(xdot);
	  x[3] = ydot/fabs(ydot);
          //x[0] = xguess;
	  //x[1] = yguess;
	}
    }

    //  if(!silent)
  cout << "Guess: x = " << x[0] << " y = " << x[1] << " xdot = " << x[2] << " ydot = "<< x[3] << " range = " << new_range <<endl;
  
  stat = QuasiNewtonMethod(nvar, x, &fopt, search_times, search_eps);
   
  if(stat == success)
    {
      xzero = x[0];
      yzero = x[1];
      xdot  = x[2];
      ydot  = x[3];
      solution = true;
    }
  else
    {
      solution =  false;
      if (reset_init == 0)
	reset_init = 1;
      else
	reset_init = 0;
    }

  return stat;
}

dbl CGeoTrack::track_func(int n, double x[])
{
 
  int i;
  double sum,time_step,x0,x1,x2,x3,xpot,ypot,bear_pot,dtsum;

  sum   = 0.0; //time_step = 1.0;
  dtsum = 0.0;

  x0 = x[0]; //target initial X
  x1 = x[1]; //target initial Y
  x2 = x[2]; //target x dot
  x3 = x[3]; //target y dot

  //calculate negative log-likelihood function
  for (i=0; i < meas_num; i++)
  {
    //    dtsum = dt[i];
    // Changed to time relative current time. HS
    dtsum = dt[i] - dtinit;
    //calculate what bearing measurement should be given x0,x1,x2,x3     
    //assuming constant velocity
    //xpot = x0 + ((time_step*i)*x2);
    xpot = x0 + (dtsum*x2);
    ypot = x1 + (dtsum*x3);    
    bear_pot = atan2((ypot-vposy[i]),(xpot-vposx[i]));
    
    sum = sum + pow((bearings[i] - bear_pot),2);
  }
 
  return (sum);
}

double *CGeoTrack::grad_track(int n,double x[],double grad[])
{

  double x0,x1,x2,x3,g0sum,g1sum,g2sum,g3sum,hx0,hx1,hx2,hx3;
  double xpot,ypot,time_step,bear_pot,diff,dtsum,dtavg;
  int i;

  x0 = x[0]; //target initial X
  x1 = x[1]; //target initial Y
  x2 = x[2]; //target x dot
  x3 = x[3]; //target y dot
  
  //time_step = 1.0;
  g0sum = 0.0;  g1sum = 0.0; g2sum = 0.0; g3sum = 0.0;

  dtsum = 0.0;
  dtavg=0.0;

  //calculate the gradient of the negative log-likelihood function
  for (i=0; i < meas_num; i++)
  {
    //    dtsum = dt[i];
    // Changed to time relative current time. HS
    dtsum = dt[i] - dtinit;
    dtavg= dtavg + dtsum;
    //calculate what bearing measurement should be given x0,x1,x2,x3     
    //xpot = x0 + ((time_step*i)*x2);
    xpot = x0 + (dtsum*x2);
    ypot = x1 + (dtsum*x3);
    bear_pot = atan2((ypot-vposy[i]),(xpot-vposx[i]));
    //calculate gradient
    hx0 = (-(ypot-vposy[i])/ (pow((ypot-vposy[i]),2) + pow((xpot-vposx[i]),2)));
    hx1 = ( (xpot-vposx[i])/ (pow((ypot-vposy[i]),2) + pow((xpot-vposx[i]),2)));
    hx2 = dtsum*hx0;
    hx3 = dtsum*hx1;

    diff = 2*(bearings[i] - bear_pot);

    g0sum = g0sum + (diff*(-1.0*hx0));
    g1sum = g1sum + (diff*(-1.0*hx1));
    g2sum = g2sum + (diff*(-1.0*hx2));
    g3sum = g3sum + (diff*(-1.0*hx3));

}
  dtavg=dtavg/meas_num;

  grad[0] = g0sum;   
  grad[1] = g1sum;
  grad[2] = g2sum;
  grad[3] = g3sum;
  
  return grad;
}

void CGeoTrack::initialize()
{
  xtmp    = new dbl[nvar];
  gtmp    = new dbl[nvar];	
  h       = new dbl[nvar*nvar];
  dl      = new dbl[nvar];
  gm      = new dbl[nvar];
  dg      = new dbl[nvar];
  gd      = new dbl[nvar];
  matq    = new dbl[nvar*nvar];
  hnew    = new dbl[nvar*nvar];
  xnew    = new dbl[nvar];
  grad    = new dbl[nvar];
  gradnew = new dbl[nvar];
  d       = new dbl[nvar];
}

void CGeoTrack::update(dbl *h, dbl *dl, dbl *gm)
{
	int	i, j, k, l;
	dbl	sum;
	dbl	a, p, q;
	
	if ((sum = vectorvector(nvar, dl, gm)) == 0.00) {
		return;
	}
	
	for (i=0; i<nvar; i++) {
		for (j=0; j<nvar; j++) {
			matq[i*nvar+j] = - dl[i]*gm[j]/sum;
		}
		matq[i*nvar+i] += 1.00;
	}
	
	for (i=0; i<nvar; i++) {
		for (j=0; j<nvar; j++) {
			a = 0;
			for (k=0; k<nvar; k++) {
				p = matq[i*nvar+k];
				for (l=0; l<nvar; l++) {
					a += p*h[k*nvar+l]*matq[j*nvar+l];
				}
			}
			hnew[i*nvar+j] = a;
		}
	}
	
	for (i=0; i<nvar; i++) {
		for (j=0; j<nvar; j++) {
			h[i*nvar+j] = hnew[i*nvar+j] + dl[i]*dl[j]/sum;
		}
	}
}

dbl CGeoTrack::theta_1(dbl *xinit, dbl *d, dbl finit, dbl p1, dbl ro)
{
	int	i;
	
	for (i=0; i<nvar; i++) xtmp[i] = xinit[i] + ro*d[i];
	return ((*this.*objective)(nvar, xtmp) - finit - p1*ro);
}

dbl CGeoTrack::theta_2(dbl *xinit, dbl *d, dbl p2, dbl ro)
{
	int	i;
	
	for (i=0; i<nvar; i++) xtmp[i] = xinit[i] + ro*d[i];
	(*this.*gradient)(nvar, xtmp, gtmp);
	return (vectorvector(nvar, gtmp, d) - p2);
}


dbl CGeoTrack::Wolfe_line_search(dbl *xinit, dbl *d)
{
	int	k;
	dbl	t1, t2, ro, dro, a, b;
	dbl	finit, p1, p2;
	
	ro = 1;
	dro = 1;
	
	finit = (*this.*objective)(nvar, xinit);
	(*this.*gradient)(nvar, xinit, gtmp);
	p1 = mu1 * vectorvector(nvar, gtmp, d);
	p2 = mu2 * vectorvector(nvar, gtmp, d);
	
	for (k=0; k<search_times; k++) {
		t1 = theta_1(xinit, d, finit, p1, ro);
		t2 = theta_2(xinit, d, p2, ro);
#if Debug > 1
		if (k % 1 == 0) {
			fprintf(stderr, "%d: t1=%lg t2=%lg\n", k, t1, t2);
		}
#endif
		if (t1 <= 0) {
			if (t2 >= 0) {
				return(ro);
			} else {
				ro += dro;
			}
		} else {
			a = ro - dro;
			b = ro;
			break;
		}
	}
	if (k == search_times) {
		return(0.00);
	}
	
	for (;;) {
#if Debug > 1
		fprintf(stderr, "a=%lg b=%lg\n", a, b);
#endif
		ro = (a + b) / 2.00;
		if (b - a < search_eps) {
			return(ro);
		}
		t1 = theta_1(xinit, d, finit, p1, ro);
		t2 = theta_2(xinit, d, p2, ro);
#if Debug > 1
		fprintf(stderr, "t1=%lg t2=%lg\n", t1, t2);
#endif
		if (t1 <=  0 && t2 >= 0) {
			return(ro);
		}
		if (t1 > 0) {
			b = ro;
		} else {
			a = ro;
		}
	}
}



void CGeoTrack::terminate()
{
	delete [] xtmp;
	delete [] gtmp;
	delete [] h;
	delete [] dl;
	delete [] gm;
	delete [] dg;
	delete [] gd;
	delete [] matq;
	delete [] hnew;	
	delete [] xnew;
	delete [] grad;
	delete [] gradnew;
	delete [] d;
}

status CGeoTrack::QuasiNewtonMethod(int n, dbl *xinit, dbl *fopt, int timeout, dbl eps)
{
	int	k;
	dbl	sum, al;
	status	stat;
	static dbl	finit, fnew;
	
	nvar = n;
	objective = &CGeoTrack::track_func;
	gradient  = &CGeoTrack::grad_track;
	
	initialize();
	
	matrixunit(nvar, h);
	finit = MAXFLOAT;
	
	stat = failure;
	
	for (k=0; k<timeout; k++) {
		fnew = finit;
		finit = (*this.*objective)(nvar, xinit);
		if (finit <= -MAXFLOAT || finit >= MAXFLOAT) {
			fprintf(stderr, "overflow\n");
			stat = failure;
			break;
			/* return(failure); */
		}
#if Debug
		if (k % SKIPTIME == 0) {
			fprintf(stderr, "k = %d   f = %lg\n", k, finit);
			/* vectorfprint(stderr, nvar, xinit); */
		}
#endif
		if (fabs(fnew - finit) < eps) {
			stat = success;
			break;
			/* *fopt = finit; */
			/* return(success); */
		}
		
		(*this.*gradient)(nvar, xinit, grad);
		
		matrixvector(nvar, nvar, d, h, grad);
		scalarvector(nvar, d, -1.00, d);
		
		sum = vectorvector(nvar, grad, d);
		if (sum >= 0) {
			matrixunit(nvar, h);
			matrixvector(nvar, nvar, d, h, grad);
			scalarvector(nvar, d, -1.00, d);
		}
		
		al = Wolfe_line_search(xinit, d);
		if (al == 0.00) {
			fprintf(stderr, "bfgs.c: alpha = %lf\n", al);
			break;
		}
		
		scalarvector(nvar, dl, al, d);
		vectoradd(nvar, xnew, xinit, dl);
		
		(*this.*gradient)(nvar, xnew, gradnew);
		vectorsub(nvar, gm, gradnew, grad);
		
		update(h, dl, gm);
		
		vectorcopy(nvar, xinit, xnew);
	}
	
	if (k >= timeout) fprintf(stderr, "timeout\n");
	
	/* fprintf(stderr, "***** end *****\n"); */
	*fopt = finit;
	terminate();
	return (stat);
	
	/* *fopt = finit; */
	/* return(failure); */
}






