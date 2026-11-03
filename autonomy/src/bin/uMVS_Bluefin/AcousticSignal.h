// Copyright 2001-2005 Massachusetts Institute of Technology and Oxford University
// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#if !defined(AFX_ACOUSTICSIGNAL_H__C9D1AD0E_042D_4D0F_93AF_39D8C18E0E8B__INCLUDED_)
#define AFX_ACOUSTICSIGNAL_H__C9D1AD0E_042D_4D0F_93AF_39D8C18E0E8B__INCLUDED_

#include "AcousticDefs.h" // Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <newmat/newmatap.h>

using namespace NEWMAT;

#include <MOOS/libMOOS/Utils/MOOSUtils.h>

#include "SimBase.h"

class CAcousticSignal : public CSimBase
{
  public:
    std::string GetSrcName();
    bool SetSrcName(const std::string& sSrcName);
    static AcousticChannel ChannelFromString(std::string& sStr);
    double GetStartTime();
    AcousticChannel GetChannel();
    bool SetChannel(AcousticChannel eChannel);
    bool HasDecayed(double dfTimeNow);
    double Age(double dfTimeNow);
    double GetExpectedIntersectionTime(Matrix& Location);

    CAcousticSignal();
    CAcousticSignal(double dfX, double dfY, double dfZ, double dfStartTime);
    virtual ~CAcousticSignal();

  protected:
    Matrix m_SrcPos;
    std::string m_sSrcName;
    double m_dfSV;
    double m_dfStartTime;
    double m_dfMaximumRange;
    AcousticChannel m_eChannel;
};

#endif // !defined(AFX_ACOUSTICSIGNAL_H__C9D1AD0E_042D_4D0F_93AF_39D8C18E0E8B__INCLUDED_)
