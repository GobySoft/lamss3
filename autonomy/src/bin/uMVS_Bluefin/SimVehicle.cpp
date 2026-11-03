// Copyright 2001-2005 Massachusetts Institute of Technology and Oxford University
// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#ifdef _WIN32
#pragma warning(disable : 4786)
#endif

#include "SimVehicle.h"
#include "AcousticNode.h"
#include "AcousticResponder.h"
#include "AcousticTransceiver.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CSimVehicle::CSimVehicle()
{
    CAcousticTransceiver* pTcvr = new CAcousticTransceiver;
    pTcvr->SetParams(m_pParams);
    m_AcousticNodes.push_front(pTcvr);
    pTcvr->SetParent(this);
    pTcvr->SetName("Tcvr1");

    CAcousticResponder* pTpdr = new CAcousticResponder;
    pTpdr->SetParams(m_pParams);
    m_AcousticNodes.push_front(pTpdr);
    pTpdr->SetParent(this);
    pTpdr->SetName("Tpdr1");
}

bool CSimVehicle::ConfigureResponder(bool bEnable, AcousticChannel eRx, AcousticChannel eTx,
                                     double dfTAT)
{
    ACOUSTIC_NODE_LIST::iterator p;
    for (p = m_AcousticNodes.begin(); p != m_AcousticNodes.end(); p++)
    {
        if ((*p)->GetName() == "Tpdr1")
        {
            (*p)->Enable(bEnable);
            (*p)->SetRxChan(eRx);
            (*p)->SetTxChan(eTx);
            ((CAcousticResponder*)(*p))->SetTAT(dfTAT);
            return true;
        }
    }

    return false;
}

CSimVehicle::~CSimVehicle() {}
