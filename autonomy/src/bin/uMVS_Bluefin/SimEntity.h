// Copyright 2001-2005 Massachusetts Institute of Technology and Oxford University
// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#if !defined(AFX_SIMENTITY_H__73AEBFEA_E3FA_46D9_91E9_DCA62067F15A__INCLUDED_)
#define AFX_SIMENTITY_H__73AEBFEA_E3FA_46D9_91E9_DCA62067F15A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "AcousticNode.h"
#include "AcousticSignal.h"
#include "SimBase.h"
#include "SimEnvironment.h"
#include "SimLoggable.h"

#include <list>
using namespace std;

typedef list<CAcousticNode*> ACOUSTIC_NODE_LIST;

class CSimEnvironment;

class CSimEntity : public CSimLoggable
{
  public:
    virtual bool SetParams(CSimParams* pParams);
    bool SolveAcoustics(double dfTime, double dfDT);
    void SetEnvironment(CSimEnvironment* pEnv);
    virtual bool GetNodePosition(CAcousticNode& Node, double dfDT, Matrix& Result);
    bool GetAcousticNodes(ACOUSTIC_NODE_LIST& List);
    virtual bool Iterate(double dfTimeNow, double dfDT);
    CSimEntity();
    virtual ~CSimEntity();

    ACOUSTIC_NODE_LIST m_AcousticNodes;

    Matrix m_Pos_e;
    Matrix m_Vel_e;

    double m_dfLastLogState;
    double m_dfLogFrequency;

    virtual bool LogState(double dfTimeNow);

    std::string m_sInputPrefix;
    std::string m_sOutputPrefix;

  protected:
    double HeadingFromYaw(double dfYaw);
    CSimEnvironment* m_pEnvironment;
    bool IsLocalSource(std::string sSrc);
};

#endif // !defined(AFX_SIMENTITY_H__73AEBFEA_E3FA_46D9_91E9_DCA62067F15A__INCLUDED_)
