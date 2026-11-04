// Copyright 2001-2005 Massachusetts Institute of Technology and Oxford University
// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#if !defined(AFX_ACOUSTICNODE_H__F8C17D77_CD53_4E2A_9D7D_110C9C1334B6__INCLUDED_)
#define AFX_ACOUSTICNODE_H__F8C17D77_CD53_4E2A_9D7D_110C9C1334B6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <set>
#include <string>
using namespace std;

#include <newmat/newmatap.h>
using namespace NEWMAT;

#include "AcousticDefs.h"
#include "SimLoggable.h"

#include "AcousticSignal.h"

class CSimEntity;
class CSimEnvironment;

class CAcousticNode : public CSimLoggable
{
  public:
    string GetFullName();
    bool SetEnvironment(CSimEnvironment* pEnv);
    virtual bool Iterate(double dfTimeNow);
    virtual bool OnAcousticHit(CAcousticSignal& Signal, double dfTime);
    AcousticChannel GetTxChannel();
    bool SetTxChan(AcousticChannel eChan);
    bool SetRxChan(AcousticChannel eChan, bool bAdd = true);
    bool Listening(AcousticChannel eChan);
    void SetParent(CSimEntity* pParent);
    CSimEntity* GetParent();
    void Enable(bool bEnable) { m_bEnabled = bEnable; };
    CAcousticNode();
    CAcousticNode(double dfX, double dfY, double dfZ);
    virtual ~CAcousticNode();

    void SetX(double dfVal) { m_OffsetPos(1, 1) = dfVal; };
    void SetY(double dfVal) { m_OffsetPos(2, 1) = dfVal; };
    void SetZ(double dfVal) { m_OffsetPos(3, 1) = dfVal; };

    CSimEntity* m_pParent;

    set<AcousticChannel> m_RxChannels;
    AcousticChannel m_TxChannel;

    CSimEnvironment* m_pEnvironment;

    bool m_bEnabled;

    double m_dfLastTxTime;

    Matrix m_OffsetPos;
};

#endif // !defined(AFX_ACOUSTICNODE_H__F8C17D77_CD53_4E2A_9D7D_110C9C1334B6__INCLUDED_)
