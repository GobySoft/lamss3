// Copyright 2001-2005 Massachusetts Institute of Technology and Oxford University
// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#if !defined(AFX_SIMPARAMS_H__37251BDC_0220_4EC4_A2C3_066D7735B978__INCLUDED_)
#define AFX_SIMPARAMS_H__37251BDC_0220_4EC4_A2C3_066D7735B978__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <MOOS/libMOOS/Utils/MOOSUtils.h>

#include <string>

using namespace std;

class CSimParams
{
  public:
    bool Load(STRING_LIST& Params);

    CSimParams();
    virtual ~CSimParams();

    string m_sLogFileName;
    bool m_bImmediateAcousticLog;
    double m_dfTOFStd;
    double m_dfProbMultiPath;
    double m_dfXYStd;
    double m_dfZStd;
    double m_dfYawStd;
    double m_dfXYVelStd;
    double m_dfYawBias;
    bool m_bAddNoise;

  private:
    bool GetTokenValPair(string sLine, string& sTok, string& sVal);
};

#endif // !defined(AFX_SIMPARAMS_H__37251BDC_0220_4EC4_A2C3_066D7735B978__INCLUDED_)
