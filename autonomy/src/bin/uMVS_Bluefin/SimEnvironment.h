// Copyright 2001-2005 Massachusetts Institute of Technology and Oxford University
// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#if !defined(AFX_SIMENVIRONMENT_H__1CF844D2_0B14_4CD1_A8E3_4A07FA3D81C7__INCLUDED_)
#define AFX_SIMENVIRONMENT_H__1CF844D2_0B14_4CD1_A8E3_4A07FA3D81C7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "MOOSTerrain.h"
#include <MOOS/libMOOS/MOOSLib.h>
#include <MOOS/libMOOS/Utils/MOOSUtils.h>

#include "AcousticSignal.h"
typedef std::list<CAcousticSignal> ACOUSTIC_SIGNAL_LIST;

class CSimEnvironment
{
  public:
    bool AddReport(const std::string& sName, const std::string& sData);
    double GetElapsedTime(double dfTimeNow);
    bool Clean();
    bool SetStartTime(double dfStartTime);
    double GetStartTime();
    bool AddSignal(CAcousticSignal NewSignal);
    bool RemoveOldSignals(double dfTimeNow);
    bool Initialise(const char* pTerrainFile = "");
    CSimEnvironment();
    virtual ~CSimEnvironment();

    double GetAltitude(double dfX, double dfY, double dfZ);
    double GetDepth(double dfZ);
    double GetTideHeight();
    double m_dfTideHeight;
    double m_dfMagneticOffset;

    CMOOSTerrain m_Terrain;

    ACOUSTIC_SIGNAL_LIST m_AcousticSignals;
    MOOSMSG_LIST m_MailOut;

  protected:
    double m_dfStartTime;
};

#endif // !defined(AFX_SIMENVIRONMENT_H__1CF844D2_0B14_4CD1_A8E3_4A07FA3D81C7__INCLUDED_)
