// Copyright 2001-2005 Massachusetts Institute of Technology and Oxford University
// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#include "SimLoggable.h"
#include "SimParams.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSimLoggable::CSimLoggable() {}

CSimLoggable::~CSimLoggable() {}

bool CSimLoggable::Log(ostringstream& strstream)
{
    if (m_pParams == NULL)
        return false;

    string sName = m_pParams->m_sLogFileName;
    if (!sName.empty())
    {
        ofstream ostr(sName.c_str(), ios::app);
        ostr << strstream.str();

        //free buffer...
        //strstream.rdbuf()->freeze(0);

        return true;
    }
    return false;
}

void CSimLoggable::SpaceLine()
{
    string sName = m_pParams->m_sLogFileName;
    if (!sName.empty())
    {
        ofstream ostr(sName.c_str(), ios::app);
        ostr << endl;
    }
}
