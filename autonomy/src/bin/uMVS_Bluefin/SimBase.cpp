// Copyright 2001-2005 Massachusetts Institute of Technology and Oxford University
// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#include "SimBase.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
int gID = 0;

CSimBase::CSimBase()
{
    m_pParams = NULL;
    m_nID = gID++;
}

CSimBase::~CSimBase() {}

bool CSimBase::SetParams(CSimParams* pParams)
{
    m_pParams = pParams;
    return true;
}
