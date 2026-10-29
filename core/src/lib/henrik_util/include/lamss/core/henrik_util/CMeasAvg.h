#include <deque>
#include <math.h>
#include <string.h>
#include <string>

#ifndef _CMEASAVG_HEADER
#define _CMEASAVG_HEADER

class CMeasAvg
{
  public:
    void configAvg(int length);

    double getAvg();
    void setVal(double);

  protected:
    int len;
    int num_measurements;
    double avg;

    std::deque<double> vals;
};

class CAngleAvg
{
  public:
    void configAvg(int length);

    double getAvg();
    void setVal(double);

  protected:
    int len;
    int num_measurements;
    double avg;

    std::deque<double> svals;
    std::deque<double> cvals;
};

class CRadianAvg
{
  public:
    void configAvg(int length);

    double getAvg();
    void setVal(double);

  protected:
    int len;
    int num_measurements;
    double avg;

    std::deque<double> svals;
    std::deque<double> cvals;
};

#endif /* _CMEASAVG_HEADER */
