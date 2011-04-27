#ifndef __CRandom_h__
#define __CRandom_h__

#include <vector>

#define IM1 2147483563
#define IM2 2147483399
#define IA1 40014
#define IA2 40692
#define IQ1 53668
#define IQ2 52774
#define IR1 12211
#define IR2 3791
#define NTAB 64
#define EPS 1.2e-15

class CRandom 
{
 public:

  double ran2(int & idum);

 protected:
  
  std::vector<int> iv;
  int iy;
  int idum2;
  double AM;
  int IMM1;
  int NDIV;
  double RNMX;
};

#endif /* __Random_h__ */

