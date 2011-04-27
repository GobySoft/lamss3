#include "Random.h"

using namespace std;

double CRandom::ran2(int & idum)
{
  int k,j;
  if (idum <= 0)
    {
      // Initialize first time around
      AM=1./IM1;
      IMM1=IM1-1;
      NDIV=1+IMM1/NTAB;
      RNMX=1.-EPS;
      
      iv.clear();
      for (int i=0; i<NTAB; i++)
	iv.push_back(0);
      idum2 = 123456789;

      idum=max(-idum,1);
      idum2=idum;
      for (j=NTAB+8;j>0;j--)
	{
	  k=idum/IQ1;
          idum=IA1*(idum-k*IQ1)-k*IR1;
          if (idum < 0) 
	    idum=idum+IM1;
          if (j <= NTAB) 
	    iv[j-1]=idum;
	}
      iy=iv[0];
    }

  k=idum/IQ1;
  idum=IA1*(idum-k*IQ1)-k*IR1;
  if (idum < 0) 
    idum=idum+IM1;
  k=idum2/IQ2;
  idum2=IA2*(idum2-k*IQ2)-k*IR2;
  if (idum2 < 0) 
    idum2=idum2+IM2;
  
  j=1+iy/NDIV;
  iy=iv[j-1]-idum2;
  iv[j-1]=idum;
  if(iy < 1)
    iy=iy+IMM1;
  return(min(AM*iy,RNMX));
}


