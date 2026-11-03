/***********************************************************/
/*    Author: Don Eickstedt and Henrik Schmidt             */
/*    FILE:   AOF_WiggleTail.h                         */
/*    DATE:   18. March 2011                               */
/***********************************************************/

#ifndef AOF_WIGGLETAIL_HEADER
#define AOF_WIGGLETAIL_HEADER

#include "AOF.h"

#define LR 1
#define OPT 2

class IvPDomain;
class AOF_WiggleTail : public AOF
{
  public:
    AOF_WiggleTail(IvPDomain, double, double);
    ~AOF_WiggleTail(){};

  public:
    double evalBox(const IvPBox*) const; // virtual defined
    double lrmetric(double) const;

  protected:
    double leftabs, rightabs, hwidth, osCourse;

    double crsBase;
    double crsDelta;
};

#endif
