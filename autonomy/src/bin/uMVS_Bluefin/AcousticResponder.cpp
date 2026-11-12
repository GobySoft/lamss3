// Copyright 2001-2005 Massachusetts Institute of Technology and Oxford University
// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#ifdef _WIN32
#pragma warning(disable : 4786)
#endif

#include "AcousticResponder.h"
#include "SimEntity.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CAcousticResponder::CAcousticResponder()
{
    m_dfTAT = 0.125;
    SetRxChan(ACOUSTIC_CHAN_CIF);
}

CAcousticResponder::~CAcousticResponder() {}

bool CAcousticResponder::OnAcousticHit(CAcousticSignal& Signal, double dfTime)
{
#ifdef VERBOSE
    MOOSTrace("CAcousticResponder::OnAcousticHit on %s Signal[%d] from %s on Channel %d\n",
              GetFullName().c_str(), Signal.m_nID, Signal.GetSrcName().c_str(),
              Signal.GetChannel());
#endif

    if (Listening(Signal.GetChannel()))
    {
        //this is a trap for self excitation..
        if (Signal.GetSrcName() == GetFullName())
            return false;

        Matrix NodePosAfterTAT;

        GetParent()->GetNodePosition(*this, GetTAT(), NodePosAfterTAT);

        CAcousticSignal ReplyPing(NodePosAfterTAT(1, 1), NodePosAfterTAT(2, 1),
                                  NodePosAfterTAT(3, 1), dfTime + GetTAT());

        ReplyPing.SetChannel(GetTxChannel());

        //here we set the src name to be something like "B1:TDR"
        ReplyPing.SetSrcName(GetFullName());

        m_pEnvironment->AddSignal(ReplyPing);

#ifdef VERBOSE
        MOOSTrace("%s is pinged by Signal[%d] from %s on Chan[%d] and replies with Signal[%d] "
                  "Chan[%d] \n",
                  GetFullName().c_str(), Signal.m_nID, Signal.GetSrcName().c_str(),
                  Signal.GetChannel(), ReplyPing.m_nID, ReplyPing.GetChannel());
#endif
    }

    return true;
}
double CAcousticResponder::GetTAT() { return m_dfTAT; }

bool CAcousticResponder::SetTAT(double dfTAT)
{
    m_dfTAT = dfTAT;

    return true;
}
