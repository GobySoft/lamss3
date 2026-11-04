// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef BHV_WIGGLETAIL_HEADER
#define BHV_WIGGLETAIL_HEADER

#include "IvPBehavior.h"

#define NO_TRACK 0
#define LR_TRACK 1
#define TRACKING 2

using namespace std;

class IvPDomain;
namespace lamss
{
namespace autonomy
{

class BHV_WiggleTail : public IvPBehavior
{
  public:
    BHV_WiggleTail(IvPDomain);
    ~BHV_WiggleTail(){};

    IvPFunction* onRunState();
    void onIdleState();
    bool setParam(std::string, std::string);

  protected:
    int decode(string);
    //double opt_course(double,double);

    double course_fix, course_new, course_lr_init, turn_angle;
    double heading, speed, ty, tx, time;
    int state;
    bool course_fixed, first_time;
    int current_state, new_state, next_turn, curr_turn;

    std::string heading_variable;
    std::string bearing_variable;
};

} // namespace autonomy
} // namespace lamss

#endif
