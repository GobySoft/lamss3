// Copyright 2001-2005 Massachusetts Institute of Technology and Oxford University
// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#if !defined(AFX_ACOUSTICBEACON_H__D8CABA71_9542_47C2_8CA4_128280A0E176__INCLUDED_)
#define AFX_ACOUSTICBEACON_H__D8CABA71_9542_47C2_8CA4_128280A0E176__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <newmat/newmatap.h>
using namespace NEWMAT;

#include "SimEntity.h"

class CAcousticBeacon : public CSimEntity
{
  public:
    CAcousticBeacon();
    virtual ~CAcousticBeacon();
    virtual bool LogState(double dfTimeNow);
};

#endif // !defined(AFX_ACOUSTICBEACON_H__D8CABA71_9542_47C2_8CA4_128280A0E176__INCLUDED_)
