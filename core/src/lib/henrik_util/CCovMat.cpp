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


#include "CCovMat.h"

using namespace std;

//
// Covariance matrix class
//

bool CovMat::putCovMat(int nrcv, double frq, vector<complex<double> > newcov)
{
  // lower triangular stored row-wise
  num_rcv = nrcv;
  freq = frq;
  cov.clear(); 
  c.clear();
  for (unsigned int ij=0; ij< nrcv*(nrcv+1)/2; ij++)
    {
	cov.push_back(complex<double>(0,0));
	c.push_back(complex<double>(0,0));
    }
  for (unsigned int i=1; i <= nrcv; i++)
    for (unsigned int j=1; j <= i; j++)
      cov[indx_tri(i,j)] = newcov[indx(i,j)];
  c_done = false;
  return(true);
}

bool CovMat::getCovMat(int nrcv, double& frq,  vector<complex<double> >& covout)
{
  if (nrcv == num_rcv)
    {
      frq = freq;
      for (unsigned int i=1; i <= nrcv; i++)
	for (unsigned int j=1; j <= i; j++)
	  {
	    covout[indx(i,j)] = cov[indx_tri(i,j)];
	    covout[indx(j,i)] = conjg(cov[indx_tri(i,j)]);
	  }
	  return(true);
    }
  else
    {
      cout << "Inconsistent covariance matrix dimension";
      return(false);
    }
}

vector<vector<complex<double> > > CovMat::getCCovMat()
{
  // This version retrieves the covariance matrix using
  // the C++ matrix indexing convention
  vector<vector<complex<double> > > covout;
  covout.resize(num_rcv);
  for (unsigned int i=0; i <= num_rcv; i++)
    covout[i].resize(num_rcv);
    
  for (unsigned int i=0; i <= num_rcv; i++)
    for (unsigned int j=0; j <= i; j++)
      {
	covout[i][j] = cov[indx_tri(i+1,j+1)];
	covout[j][i] = conjg(cov[indx_tri(i+1,j+1)]);
      }
  return(covout);
}

complex<double> CovMat::getCCovar(int i, int j)
{
  // returning covariance, element  1,2...)
  // with calling indeces i,j being C style i,j = 0,1....
  if (j<=i)
    return(cov[indx_tri(i+1,j+1)]);
  else
    return(conjg(cov[indx_tri(j+1,i+1)]));
}

bool CovMat::getCovLow(int nrcv, double& frq,  vector<complex<double> >& covout)
{
  if (nrcv == num_rcv)
    {
      frq = freq;
      covout = cov;
      return(true);
    }
  else
    {
      cout << "Inconsistent covariance matrix dimension";
      return(false);
    }
}

bool CovMat::getCholLow(int nrcv, double& frq,  vector<complex<double> >& cholout)
{
  if (nrcv == num_rcv)
    {
      frq = freq;
      double wn =1e-6;
      if (!c_done)
	while (!cholesky())
	  {
	    //	    MOOSTrace("Freq = %f: Adding %f white noise intensity\n",frq,wn);
	    addWhiteNoise(wn);
	    wn *= 10.0;
	  }
      c_done =true;
      cholout = c;
      return(true);
    }
  else
    {
      cout << "Inconsistent covariance matrix dimension";
      return(false);
    }
}

bool CovMat::wnChol()
{
  for (unsigned int i=1; i <= num_rcv; i++)
    {
      double rr = cov[indx_tri(i,i)].real();
      c[indx_tri(i,i)] = sqrt(rr);
    }
  c_done = true;
  return true;
}

  bool CovMat::putCholLow(int nrcv, double frq,  vector<complex<double> > cholin)
{
  num_rcv = nrcv;
  freq = frq;
  c = cholin;
  c_done =true;
  return(true);
}

bool CovMat::putCovLow(int nrcv, double frq,  vector<complex<double> > covin)
{
  num_rcv = nrcv;
  freq = frq;
  cov = covin;
  c_done = false;
  return(true);
}

bool CovMat::scaleCovMat(double factor)
{
      for (unsigned int i=1; i <= num_rcv; i++)
	for (unsigned int j=1; j <= i; j++)
	  {
	    cov[indx_tri(i,j)] *= factor*factor;
	    c[indx_tri(i,j)] *= factor;
	  }
      return(true);
}

double CovMat::normCholTrace()
{
  double sum = 0.0;
  for (unsigned int i=1; i <= num_rcv; i++)
      sum += pow(c[indx_tri(i,i)].real(),2) + pow(c[indx_tri(i,i)].imag(),2);  
  sum /= num_rcv;
  return(sum);
}

complex<double> CovMat::conjg(complex<double> a)
{
  return(complex<double>(a.real(),-a.imag()));
}

bool CovMat::cholesky()
{
  int nrows = num_rcv;
  int ncols = num_rcv;
  // cholesky returns only lower triangular
  int nelem = num_rcv*(num_rcv+1)/2;
  if (c.size() < nelem)
    {
      c.clear();
      for (unsigned int i = 0; i< nrows*(nrows+1)/2; i++)
	c.push_back(complex<double>(0,0));
    }
  double eps = 1e-12;
  double norm = 0.0;
  for (unsigned int k = 1; k<=nrows; k++)
    norm += cov[indx_tri(k,k)].real();
  eps = eps*norm/nrows;


  for (unsigned int k = 1; k<=nrows; k++)
    {
      complex<double> sum = complex<double>(0,0);
      for (unsigned int l = 1; l <= k-1; l++)
	sum = sum + c[indx_tri(k,l)]*conjg(c[indx_tri(k,l)]);
      double arg = cov[indx_tri(k,k)].real() - sum.real() ;
      if (arg < eps)
	{ 
	  return(false);
	}
      c[indx_tri(k,k)] = sqrt(arg);
      //      if (k==1)
      //	MOOSTrace("cov(1,1)=%f, arg=%f, cho(1,1)=%f\n",cov[indx_tri(k,k)].real(),arg,c[indx_tri(k,k)].real());
      for (unsigned int i = k+1; i<=nrows; i++)
	{
	  complex<double> sum_l = complex<double>(0,0);
	  for (unsigned int l = 1; l < k; l++)
	    sum_l += c[indx_tri(i,l)] * conjg(c[indx_tri(k,l)]);
	  // Lower triangular
	  c[indx_tri(i,k)] = ( cov[indx_tri(i,k)] - sum_l )/c[indx_tri(k,k)].real();
	}
    }
  return(true);
}

void CovMat::addWhiteNoise(double wn)
{
  double norm = 0;
  for (unsigned int i = 1; i<= num_rcv; i++)
    norm += cov[indx_tri(i,i)].real();
  norm /= num_rcv;
  for (unsigned int i = 1; i<= num_rcv; i++)
    cov[indx_tri(i,i)] += norm*wn;
  return;
}

int CovMat::indx(int i, int j)
{
  return((i-1)+num_rcv*(j-1));
}

int CovMat::indx_tri(int i, int j)
{
  // vector index for packed lower triangular matrix including diagonal
  return(i*(i+1)/2 - 1 -(i-j));
}

double CovMat::gasdev()
{
  // generates random complex number with gaussian amplitude distribution
  // Must be called in sequence for real and imaginary part, with iset=0
  // before each pair.

  double rsq = 0.0;
  double v1,v2;
  double res;

  if (iset == 0)
    {
      while  (rsq >= 1. || rsq == 0.)
	{
	  v1 = 2.*rand()/RAND_MAX - 1.;
	  v2 = 2.*rand()/RAND_MAX - 1.;
	  rsq=pow(v1,2)+ pow(v2,2);
	}
      double fac=sqrt(-2.*log(rsq)/rsq);
      gset = v1*fac;
      res  = v2*fac;
      iset=1;
    }
  else
    {
      res  = gset;
      iset = 0;
    }
  return(res);
}

void CovMat::realization(  vector<complex<double> >& out)
{
  double wn =1e-6;
  double scale_gaussian=sqrt(0.5); //Added AJP 4/12/11
  if (!c_done)
    while (!cholesky())
      {
	addWhiteNoise(wn);
	wn *= 10.0;
      }
  c_done =true;
  
  //     'b' IS THE LOWER TRIANGULAR MATRIX THAT IS USED SUBSEQUENTLY.
  //--------------------random noise in frequency domain-------	
  //     
  //--------------------random phase vector--------------------
  vector<complex<double> > rp;
  for (unsigned int j = 1; j <= num_rcv; j++)
    {
      iset = 0;
      double rr=scale_gaussian*gasdev();
      double ri=scale_gaussian*gasdev();
      rp.push_back(complex<double>(rr,ri));
    }  
  //-------------------random phase vector done-------
  out.clear();
  for (unsigned int i= 1; i <= num_rcv; i++)
    {
      complex<double> sum = complex<double>(0,0);
      for (unsigned int j= 1; j <= i; j++)
	sum += c[indx_tri(i,j)] * rp[j-1];
      out.push_back(sum);
    }
  return;
}


