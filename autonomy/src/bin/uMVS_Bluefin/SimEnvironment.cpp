// Copyright 2001-2005 Massachusetts Institute of Technology and Oxford University
// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#include <MOOS/libMOOS/MOOSLib.h>
#include <MOOS/libMOOS/Utils/MOOSUtils.h>

#include "SimEnvironment.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSimEnvironment::CSimEnvironment()
{
    m_dfTideHeight = 30;
    m_dfMagneticOffset = 0.0;
}

CSimEnvironment::~CSimEnvironment() {}

double CSimEnvironment::GetAltitude(double dfX, double dfY, double dfZ)
{
    double dfTmp = m_Terrain.GetAltitude(dfX, dfY, dfZ);

    return dfTmp;
}

double CSimEnvironment::GetDepth(double dfZ) { return m_dfTideHeight - dfZ; }

double CSimEnvironment::GetTideHeight() { return m_dfTideHeight; }

bool CSimEnvironment::Initialise(const char* sTerrainFile) { return m_Terrain.Load(sTerrainFile); }

bool CSimEnvironment::RemoveOldSignals(double dfTimeNow)
{
    //remove old acoustic signals
    ACOUSTIC_SIGNAL_LIST::iterator q, t;
    for (q = m_AcousticSignals.begin(); q != m_AcousticSignals.end(); q++)
    {
        CAcousticSignal& rSignal = *q;
        if (rSignal.HasDecayed(dfTimeNow))
        {
            t = q;
            t++;
            m_AcousticSignals.erase(q);
            q = t;

            //            MOOSTrace("removing decayed signal\n");
        }
    }

    return true;
}

bool CSimEnvironment::AddSignal(CAcousticSignal NewSignal)
{
    //  MOOSTrace("Adding Signal!\n");
    m_AcousticSignals.push_back(NewSignal);

#ifdef SIM_ENV_VERBOSE
    MOOSTrace("Add Signal - Acoustic Signals are now [%d]:\n", m_AcousticSignals.size());
    ACOUSTIC_SIGNAL_LIST::iterator p;
    for (p = m_AcousticSignals.begin(); p != m_AcousticSignals.end(); p++)
    {
        MOOSTrace("\tSignal[%d] from %s on Channel[%d]\n", p->m_nID, p->GetSrcName().c_str(),
                  p->GetChannel());
    }
#endif
    return true;
}

double CSimEnvironment::GetStartTime() { return m_dfStartTime; }

bool CSimEnvironment::SetStartTime(double dfStartTime)
{
    m_dfStartTime = dfStartTime;

    return true;
}

bool CSimEnvironment::Clean()
{
    m_AcousticSignals.clear();

    return true;
}

double CSimEnvironment::GetElapsedTime(double dfTimeNow) { return dfTimeNow - m_dfStartTime; }

bool CSimEnvironment::AddReport(const std::string& sName, const std::string& sData)
{
    m_MailOut.push_back(CMOOSMsg(MOOS_NOTIFY, sName, sData.c_str()));
    return true;
}
