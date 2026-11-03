// Copyright 2001-2005 Massachusetts Institute of Technology and Oxford University
// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#ifdef _WIN32
#pragma warning(disable : 4786)
#endif

#include "SimParams.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSimParams::CSimParams()
{
    //default of approximately 1m std...
    m_dfTOFStd = 0.00066;
    m_bImmediateAcousticLog = true;

    m_dfXYStd = 0.1;
    m_dfZStd = 0.03;
    m_dfYawStd = MOOSDeg2Rad(0.5);
    m_dfXYVelStd = 0.02;

    m_bAddNoise = false;
    m_dfYawBias = 0;
}

CSimParams::~CSimParams() {}

bool CSimParams::Load(STRING_LIST& sParams)
{
    STRING_LIST::iterator p;

    for (p = sParams.begin(); p != sParams.end(); p++)
    {
        string sLine, sTok, sVal;
        sLine = *p;

        if (GetTokenValPair(sLine, sTok, sVal))
        {
            if (MOOSStrCmp(sTok, "LogFile"))
            {
                m_sLogFileName = sVal;
            }
            else if (MOOSStrCmp(sTok, "AddNoise"))
            {
                m_bAddNoise = MOOSStrCmp(sVal, "TRUE");
            }
            else if (MOOSStrCmp(sTok, "InstantLogAcoustics"))
            {
                m_bImmediateAcousticLog = MOOSStrCmp(sVal, "TRUE");
            }
            else if (MOOSStrCmp(sTok, "MultiPathProbablity"))
            {
                m_dfProbMultiPath = atof(sVal.c_str());
            }
            else if (MOOSStrCmp(sTok, "TOFNoise"))
            {
                m_dfTOFStd = atof(sVal.c_str());
            }
            else if (MOOSStrCmp(sTok, "XYNOISE"))
            {
                m_dfXYStd = atof(sVal.c_str());
            }
            else if (MOOSStrCmp(sTok, "ZNOISE"))
            {
                m_dfZStd = atof(sVal.c_str());
            }
            else if (MOOSStrCmp(sTok, "YAWNOISE"))
            {
                m_dfYawStd = atof(sVal.c_str());
                m_dfYawStd = MOOSDeg2Rad(m_dfYawStd);
            }
            else if (MOOSStrCmp(sTok, "YAWBIAS"))
            {
                m_dfYawBias = atof(sVal.c_str());
            }
            else if (MOOSStrCmp(sTok, "XYVELNOISE"))
            {
                m_dfXYVelStd = atof(sVal.c_str());
            }
        }
    }

    return true;
}

bool CSimParams::GetTokenValPair(string sLine, string& sTok, string& sVal)
{
    if (sLine.find("=") != string::npos)
    {
        MOOSRemoveChars(sLine, " \t\r");
        sTok = MOOSChomp(sLine, "=");
        sVal = sLine;
        return true;
    }
    else
    {
        return false;
    }
}
