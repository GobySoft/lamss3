// H. Schmidt, henrik@mit.edu 03.08.11
// Laboratory for Autonomous Marine Sensing Systems (LAMSS)
// Massachusetts Institute ofe chnology 
// 
// CVectorUtils.cpp  
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


#include "CVectorUtils.h"

using namespace std;

// >>>>>>>>>>>>>>> vector utility methods <<<<<<<<<<<<
int CmyVectorUtils::str2dbl_vec(string msg, vector<double>& values)
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

bool CmyVectorUtils::dbl_vec2str(vector<double> numbers, string& output)
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

vector<double> CmyVectorUtils::zeros(int m, int n)
{
  vector<double> values;
  values.clear();
  for (int i=0; i<m; i++)
    
    for (int j=0; j<n; j++)
      values.push_back(0.0);
  
  return(values);
}

double CmyVectorUtils::mean(vector<double> values)
{
  int len = values.size();
  double sum=0;
  for (int i=0; i<len; i++)
    sum += values[i];
  
  return(sum/len);
}

bool CmyVectorUtils::vshift(vector<double> in, int indx_in, vector<double>& out, int indx_out, int len)
{
  if (indx_in == 0 && indx_out == 0 && len == in.size())
    {
      // simple copy, clear out first
      out.clear();
      for (int i=0; i<len; i++)
	out.push_back(in[i]);
    }
  else 
    {
      if (indx_in+len > in.size() || indx_out+len > out.size())
	return(false);
      // Make sure it works if in and out same vector (shifting)
      if (indx_in >= indx_out)
	{
	  for (int i=0; i<len; i++)
	    out[indx_out+i] = in[indx_in+i];
	}
      else
	{
	  for (int i=len-1; i>=0; i--)
	    out[indx_out+i] = in[indx_in+i];
	}
    }
  return(true);
}

bool CmyVectorUtils::cumsum(vector<double> in, vector<double>& out)
{
  double sum = 0.0;  
  out.clear();
  for (int i=0; i < in.size(); i++)
    {
      sum += in[i];
      out.push_back(sum);
    }
  
  return(true);
}

bool CmyVectorUtils::cumtrapz(vector<double> x, vector<double> y, vector<double>& out)
{
  double sum = 0.5*y[0]*(x[1]-x[0]);  
  out.clear();
  out.push_back(sum);

  int last = y.size()-1;
  for (int i=1; i < last-1 ; i++)
    {
      sum += y[i]*0.5*(x[i+1]-x[i-1]);
      out.push_back(sum);
    }

  sum += 0.5*y[last]*(x[last]-x[last-1]);;
  out.push_back(sum);
  
  return(true);
}

double CmyVectorUtils::sum(vector<double> in)
{
  double sum = 0.0;  
  for (int i=0; i < in.size(); i++)
    {
      sum += in[i];
    }
  
  return(sum);
}

int CmyVectorUtils::intsum(vector<int> in)
{
  int sum = 0;  
  for (int i=0; i < in.size(); i++)
    {
      sum += in[i];
    }
  
  return(sum);
}

bool CmyVectorUtils::mmul(vector<double> A, int rows_A, int cols_A, vector<double> B, int cols_B, vector<double>& C)
{
  // performs matrix product C(rows_A,cols_B) = A(rows_A,cols_A) * B(cols_A,cols_B), assuming matrices are stored columnwize, FORTRAN style
  C.clear();
  for (int j=0; j<cols_B; j++)
    for (int i=0; i<rows_A; i++)
      {
	double sum =0.0;
	for (int k=0; k<cols_A; k++)
	  sum += A[i+k*rows_A] * B[k+j*cols_A];
	C.push_back(sum);
      }
  return(true);
}

bool CmyVectorUtils::vmul(vector<double> a, vector<double> b, vector<double>& c)
{
  if (a.size() != b.size())
    return(false);

  if(a.size() == c.size())
    {
      for (int i=0; i<a.size(); i++)
	c[i] = a[i] * b[i];
    }
  else
    {
      c.clear();
      for (int i=0; i<a.size(); i++)
	c.push_back(a[i]*b[i]);
    }
  return(true);
}

bool CmyVectorUtils::vadd(vector<double> a, vector<double> b, vector<double>& c)
{
  if (a.size() != b.size())
    return(false);

  if(a.size() == c.size())
    {
      for (int i=0; i<a.size(); i++)
	c[i] = a[i] + b[i];
    }
  else
    {
      c.clear();
      for (int i=0; i<a.size(); i++)
	c.push_back(a[i]+b[i]);
    }
  return(true);
}

bool CmyVectorUtils::vsub(vector<double> a, vector<double> b,  vector<double>& c)
{
  if (a.size() != b.size())
    return(false);

  if(a.size() == c.size())
    {
      for (int i=0; i<a.size(); i++)
	c[i] = a[i] - b[i];
    }
  else
    {
      c.clear();
      for (int i=0; i<a.size(); i++)
	c.push_back(a[i]-b[i]);
    }
  return(true);
}

bool CmyVectorUtils::vneg(vector<double> a, vector<double>& c)
{
  if(a.size() == c.size())
    {
      for (int i=0; i<a.size(); i++)
	c[i] = -a[i];
    }
  else
    {
      c.clear();
      for (int i=0; i<a.size(); i++)
	c.push_back(-a[i]);
    }
  return(true);
}

bool CmyVectorUtils::vsmul(vector<double> a, double b, vector<double>& c)
{
  if(a.size() == c.size())
    {
      for (int i=0; i<a.size(); i++)
	c[i] = b*a[i];
    }
  else
    {
      c.clear();
      for (int i=0; i<a.size(); i++)
	c.push_back(b*a[i]);
    }
  return(true);
}

bool CmyVectorUtils::vmov(vector<double> a, vector<double>& c)
{
  if(a.size() == c.size())
    {
      for (int i=0; i<a.size(); i++)
	c[i] = a[i];
    }
  else
    {
      c.clear();
      for (int i=0; i<a.size(); i++)
	c.push_back(a[i]);
    }
  return(true);
}

double CmyVectorUtils::vmax(vector<double> in)
{
  double maxval = -1e300;  
  for (int i=0; i < in.size(); i++)
    {
      if ( in[i] > maxval )
	maxval = in[i];
    }
  
  return(maxval);
}

double CmyVectorUtils::vmin(vector<double> in)
{
  double minval = 1e300;  
  for (int i=0; i < in.size(); i++)
    {
      if ( in[i] < minval )
	minval = in[i];
    }
  
  return(minval);
}

bool CmyVectorUtils::minv(vector<double> a, int dim, vector<double>& c)
{
  nrows = dim;
  ncols = dim;
  c = zeros(dim,dim);
  for (int i=1; i<= dim; i++)
    c[indx(i,i)] = 1;

  for (int k=1; k<=dim; k++)
    {
      // pivoting
      double mx= fabs(a[indx(k,k)]);
      int imx=k;
      for (int i=k+1; i<=dim; i++)
	if (fabs(a[indx(i,k)]) > mx)
	  {
	    mx = fabs(a[indx(i,k)]);
	    imx = i;
	  }
      if (imx > k)
	{
	  for (int j=k; j<=dim;j++)
	    {
	      double temp = a[indx(k,j)];
	      a[indx(k,j)] = a[indx(imx,j)];
	      a[indx(imx,j)] = temp;
	    }
	  for (int j=1; j<=dim;j++)
	    {
	      double temp = c[indx(k,j)];
	      c[indx(k,j)] = c[indx(imx,j)];
	      c[indx(imx,j)] = temp;
	    }
	}
      // reduction
      if (a[indx(k,k)] == 0.0)
	return(false);
      a[indx(k,k)] = 1.0/a[indx(k,k)];
      for (int i=k+1; i<= dim; i++)
	{
	  double fac=-a[indx(i,k)]*a[indx(k,k)];
	  a[indx(i,k)]=0.0;
	  for (int j=k+1; j<=dim; j++)
	    a[indx(i,j)] += a[indx(k,j)]*fac;
	  for (int j=1; j<=dim; j++)
	    c[indx(i,j)] += c[indx(k,j)]*fac;
	}
    }
      // back substitition
  for (int i=dim; i>=1; i--)
      for (int j=1; j<= dim; j++)
	{
	  double ss=c[indx(i,j)];
	  for (int k=i+1; k<=dim; k++)
	    ss -= c[indx(k,j)]*a[indx(i,k)];
	  c[indx(i,j)]=ss*a[indx(i,i)];
	}

  return(true);
}

bool CmyVectorUtils::bminv(vector<double> a, int dim, int bw, vector<double>& c)
{
  nrows = dim;
  ncols = dim;
  int bw2 = bw;

  c = zeros(dim,dim);
  for (int i=1; i<= dim; i++)
    c[indx(i,i)] = 1;

  // equilibration
  for (int i=1; i<=dim; i++)
    {
      int j1=max(1,i-bw);
      int j2=min(dim,i+bw);
      double fac=0.0;
      for (int j=j1; j<=j2; j++)
	fac=max(fac,fabs(a[indx(i,j)]));
      fac = 1.0/fac;
      for (int j=j1; j<=j2; j++)
	a[indx(i,j)] = a[indx(i,j)]*fac;
      c[indx(i,i)] = fac*c[indx(i,i)];
    }

  for (int k=1; k<=dim; k++)
    {
      //MOOSTrace("k=%d, pivoting\n",k);
      // pivoting
      double mx= fabs(a[indx(k,k)]);
      int imx=k;
      for (int i=k+1; i<=min(k+bw,dim); i++)
	if (fabs(a[indx(i,k)]) > mx)
	  {
	    mx = fabs(a[indx(i,k)]);
	    imx = i;
	  }
      if (imx > k)
	{
	  bw2=max(bw2,bw+imx-k);
	  for (int j=k; j<=min(k+bw2,dim);j++)
	    {
	      double temp = a[indx(k,j)];
	      a[indx(k,j)] = a[indx(imx,j)];
	      a[indx(imx,j)] = temp;
	    }
	  for (int j=1; j<=dim;j++)
	    {
	      double temp = c[indx(k,j)];
	      c[indx(k,j)] = c[indx(imx,j)];
	      c[indx(imx,j)] = temp;
	    }
	}
      // MOOSTrace("k=%d, reduction\n",k);
      // reduction
      if (a[indx(k,k)] == 0.0)
	return(false);
      a[indx(k,k)] = 1.0/a[indx(k,k)];
      for (int i=k+1; i<= min(k+bw2,dim); i++)
	{
	  double fac=-a[indx(i,k)]*a[indx(k,k)];
	  a[indx(i,k)]=0.0;
	  for (int j=k+1; j<=min(dim,k+bw2); j++)
	    a[indx(i,j)] += a[indx(k,j)]*fac;
	  for (int j=1; j<=dim; j++)
	    c[indx(i,j)] += c[indx(k,j)]*fac;
	}
    }
      // back substitition
  for (int i=dim; i>=1; i--)
    {
      //  MOOSTrace("i=%d, back substitution\n",i);

      int kmax=min(i+bw2,dim);
      for (int j=1; j<= dim; j++)
	{
	  double ss=c[indx(i,j)];
	  for (int k=i+1; k<=kmax; k++)
	    ss -= c[indx(k,j)]*a[indx(i,k)];
	  c[indx(i,j)]=ss*a[indx(i,i)];
	}
    }

  //  MOOSTrace("back substitution done\n");
  return(true);
}

double CmyVectorUtils::bsolve(vector<double> a, int dim, int bw, vector<double>& b, int bcols)
{
  nrows = dim;
  ncols = dim;
  int bw2 = bw;

  double cond=0.0;

  if (b.size() < dim*bcols)
    {
      MOOSTrace("bsolve: dimension problems with RHS\n");
      return(1e100);
    }
  // equilibration
  for (int i=1; i<=dim; i++)
    {
      int j1=max(1,i-bw);
      int j2=min(dim,i+bw);
      double fac=0.0;
      for (int j=j1; j<=j2; j++)
	fac=max(fac,fabs(a[indx(i,j)]));
      fac = 1.0/fac;
      for (int j=j1; j<=j2; j++)
	a[indx(i,j)] = a[indx(i,j)]*fac;
      for (int j=1; j<=bcols; j++)
	b[indx(i,j)] = fac*b[indx(i,j)];
    }

  for (int k=1; k<=dim; k++)
    {
      //MOOSTrace("k=%d, pivoting\n",k);
      // pivoting
      double mx= fabs(a[indx(k,k)]);
      int imx=k;
      for (int i=k+1; i<=min(k+bw,dim); i++)
	if (fabs(a[indx(i,k)]) > mx)
	  {
	    mx = fabs(a[indx(i,k)]);
	    imx = i;
	  }
      if (imx > k)
	{
	  bw2=max(bw2,bw+imx-k);
	  for (int j=k; j<=min(k+bw2,dim);j++)
	    {
	      double temp = a[indx(k,j)];
	      a[indx(k,j)] = a[indx(imx,j)];
	      a[indx(imx,j)] = temp;
	    }
	  for (int j=1; j<=bcols;j++)
	    {
	      double temp = b[indx(k,j)];
	      b[indx(k,j)] = b[indx(imx,j)];
	      b[indx(imx,j)] = temp;
	    }
	}
      //MOOSTrace("k=%d, reduction\n",k);
      // reduction
      if (a[indx(k,k)] == 0.0)
	return(1e100);
      a[indx(k,k)] = 1.0/a[indx(k,k)];
      cond= max(cond,fabs(a[indx(k,k)]));

      for (int i=k+1; i<= min(k+bw2,dim); i++)
	{
	  double fac=-a[indx(i,k)]*a[indx(k,k)];
	  a[indx(i,k)]=0.0;
	  for (int j=k+1; j<=min(dim,k+bw2); j++)
	    a[indx(i,j)] += a[indx(k,j)]*fac;
	  for (int j=1; j<=bcols; j++)
	    b[indx(i,j)] += b[indx(k,j)]*fac;
	}
    }
  //  back substitition
  for (int i=dim; i>=1; i--)
    {
      //  MOOSTrace("i=%d, back substitution\n",i);

      int kmax=min(i+bw2,dim);
      for (int j=1; j<= bcols; j++)
	{
	  double ss=b[indx(i,j)];
	  for (int k=i+1; k<=kmax; k++)
	    ss -= b[indx(k,j)]*a[indx(i,k)];
	  b[indx(i,j)]=ss*a[indx(i,i)];
	}
    }

  //MOOSTrace("back substitution done\n");
  return(cond);
}

int CmyVectorUtils::indx(int i, int j)
{
  return((i-1)+nrows*(j-1));
}

int CmyVectorUtils::indx_tri(int i, int j)
{
  // vector index for packed triangular matrix including diagonal
  return((i-1)+(j-1));
}



bool CmyVectorUtils::interp(vector<double> x, vector<double> y, vector<double> x_int, vector<double>& y_int)
{
  y_int = zeros(x_int.size(),1);
  for (int i=0; i<x_int.size(); i++)
    {
      if ((x_int[i] < x[0]) || (x_int[i] > x[x.size()-1]))
	return(false);
      bool done =  false; 
      for (int j=1; j<x.size() && !done; j++)
	{
	  if (x_int[i] < x[j])
	    {
	      y_int[i] = y[j-1] + (x_int[i]-x[j-1])*(y[j]-y[j-1])/(x[j]-x[j-1]);
	      done =true;
	    }
	}
    }
  return(true);
}

// >>>>>>>> Complex arithmetic <<<<<<<

  complex<double> CmyVectorUtils::zmul(complex<double> a, complex<double> b)
{
  double rr = a.real()*b.real() -a.imag()*b.imag();
  double im = a.real()*b.imag() + a.imag()*b.real();
  return(complex<double>(rr,im));
}

complex<double> CmyVectorUtils::conjg(complex<double> a)
{
  double rr = a.real();
  double im = - a.imag();
  return(complex<double>(rr,im));
}

bool CmyVectorUtils::cholesky(vector<complex<double> > a, int dim, vector<complex<double> >& c)
{
  nrows = dim;
  ncols = dim;
  // cholesky returns only lower triangular
  c.clear();
  c = zzeros(dim,dim);
  double arg = a[indx(1,1)].real();
  c[indx(1,1)]=sqrt(arg);
  for (unsigned int k = 1; k<=nrows; k++)
    {
      complex<double> sum = complex<double>(0,0);
      for (unsigned int l = 1; l <= nrows-1; l++)
	sum = sum + zmul(c[indx(k,l)],conjg(c[indx(k,l)]));
      a[indx(k,k)].real() - sum.real() ;
      c[indx(k,k)] = sqrt(arg);
      if (c[indx(k,k)].real() == 0.0) 
	return(false);
      for (unsigned int i = k+1; i<=nrows; i++)
	{
	  complex<double> sum_l = complex<double>(0,0);
	  for (unsigned int l = 1; l < k; l++)
	    sum_l += zmul(c[indx(i,l)], conjg(c[indx(k,l)]));
	  // Lower triangular
	  c[indx(i,k)] = ( a[indx(i,k)] - sum_l )/c[indx(k,k)].real();
	}
    }
  return(true);
}

bool CmyVectorUtils::mmul(vector<complex<double> > A, int rows_A, int cols_A, vector<complex<double> > B, int cols_B, vector<complex<double> >& C)
{
  // performs complex matrix product C(rows_A,cols_B) = A(rows_A,cols_A) * B(cols_A,cols_B), assuming matrices are stored columnwize, FORTRAN style
  C.clear();
  for (int j=0; j<cols_B; j++)
    for (int i=0; i<rows_A; i++)
      {
	complex<double> sum = 0.0;
	for (int k=0; k<cols_A; k++)
	  sum += A[i+k*rows_A] * B[k+j*cols_A];
	C.push_back(sum);
      }
  return(true);
}

vector<complex<double> >  CmyVectorUtils::zzeros(int m, int n)
{
  vector<complex<double> > values;
  values.clear();
  for (int i=0; i<m; i++)
    for (int j=0; j<n; j++)
      values.push_back(complex<double>(0,0));
  
  return(values);
}

//Added by SDanesh

vector<double> CmyVectorUtils::hann(int N) //creates hanning window of length N
{
  vector<double> window;
  window.clear();
  for (int n=0; n<N;n++)
    {
      window.push_back(pow(sin(PI*n/(N-1)),2));
    }
  return(window);
}
  
int CmyVectorUtils::NN(double num, vector<double> myvec) //Find Nearest Neighbor 
//myvec must be monotonically INCREASING
{
  double dif1, dif2;
  int index;
  int dflt = myvec.size();
  //Nearester neighbor - resturns index in vector
  for (unsigned int i=0; i<myvec.size()-1; i++)
    {
      if ((num>myvec[i]) && (num<=myvec[i+1]))
	{
	  dif1 = abs(num-myvec[i]);
	  dif2 = abs(num-myvec[i+1]);
	  index = i;
	  break;
	}
    }
  if (dif1<=dif2)
    {return(index);}
  if(dif2<dif1)
    {return(index+1);}
  else
    return(dflt);
}

vector<vector<double> > CmyVectorUtils::zero_matrix(int n, int m)
{
  vector<vector<double> > my_matrix;
  vector<double> myvals;
  my_matrix.clear();
  for (int i=0; i<n; i++)
    {
      myvals.clear();
      for (int j=0; j<m; j++)
	{
	  myvals.push_back(0);
	}
      my_matrix.push_back(myvals);
    }
  return(my_matrix);
}
