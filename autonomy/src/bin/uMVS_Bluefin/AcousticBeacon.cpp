// Copyright 2001-2005 Massachusetts Institute of Technology and Oxford University
// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#ifdef _WIN32
#pragma warning(disable : 4786)
#endif

#include "AcousticBeacon.h"
#include "AcousticResponder.h"
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CAcousticBeacon::CAcousticBeacon()
{
    CAcousticNode* pTheAcousticsBit = new CAcousticResponder;

    pTheAcousticsBit->SetRxChan(ACOUSTIC_CHAN_CIF);

    pTheAcousticsBit->SetTxChan(ACOUSTIC_CHAN_1);

    pTheAcousticsBit->SetParent(this);

    pTheAcousticsBit->SetName("TDR1");

    m_AcousticNodes.push_back(pTheAcousticsBit);

    m_Vel_e = 0;
    m_Pos_e = 0;
}

bool CAcousticBeacon::LogState(double /*dfTimeNow*/)
{
    //no need to log beacon positions!
    return true;
}

CAcousticBeacon::~CAcousticBeacon() {}
