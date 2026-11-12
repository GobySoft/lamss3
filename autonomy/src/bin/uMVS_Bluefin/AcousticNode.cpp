// Copyright 2001-2005 Massachusetts Institute of Technology and Oxford University
// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#ifdef _WIN32
#pragma warning(disable : 4786)
#endif

#include "AcousticNode.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CAcousticNode::CAcousticNode()
{
    m_OffsetPos.ReSize(3, 1);
    m_OffsetPos = 0;

    m_bEnabled = true;
}

CAcousticNode::CAcousticNode(double dfX, double dfY, double dfZ)
{
    m_OffsetPos.ReSize(3, 1);

    m_OffsetPos << dfX << dfY << dfZ;
}

CAcousticNode::~CAcousticNode() {}

CSimEntity* CAcousticNode::GetParent() { return m_pParent; }

void CAcousticNode::SetParent(CSimEntity* pParent) { m_pParent = pParent; }

bool CAcousticNode::Listening(AcousticChannel eChan)
{
    if (!m_bEnabled)
        return false;

    return m_RxChannels.find(eChan) != m_RxChannels.end();
}

bool CAcousticNode::SetRxChan(AcousticChannel eChan, bool bAdd)
{
    if (bAdd)
    {
        m_RxChannels.insert(eChan);
    }
    else
    {
        m_RxChannels.erase(eChan);
    }

    return true;
}

bool CAcousticNode::SetTxChan(AcousticChannel eChan)
{
    m_TxChannel = eChan;

    return true;
}

AcousticChannel CAcousticNode::GetTxChannel() { return m_TxChannel; }

bool CAcousticNode::OnAcousticHit(CAcousticSignal& /*Signal*/, double /*dfTime*/) { return true; }

bool CAcousticNode::Iterate(double /*dfTimeNow*/) { return true; }

bool CAcousticNode::SetEnvironment(CSimEnvironment* pEnv)
{
    m_pEnvironment = pEnv;

    return pEnv != NULL;
}

string CAcousticNode::GetFullName() { return ((CSimBase*)m_pParent)->GetName() + "/" + GetName(); }
