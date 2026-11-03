// Copyright 2001-2005 Massachusetts Institute of Technology and Oxford University
// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#if !defined(AFX_SIMBASE_H__582C7AC1_8290_4E84_88EA_2F1DD7F1A558__INCLUDED_)
#define AFX_SIMBASE_H__582C7AC1_8290_4E84_88EA_2F1DD7F1A558__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <string>
using namespace std;

#include "SimParams.h"

class CSimBase
{
  public:
    virtual bool SetParams(CSimParams* pParams);
    CSimParams* m_pParams;
    CSimBase();
    virtual ~CSimBase();

    string GetName() { return m_sName; };
    void SetName(const string& sName) { m_sName = sName; };

    string m_sName;

    int m_nID;
};

#endif // !defined(AFX_SIMBASE_H__582C7AC1_8290_4E84_88EA_2F1DD7F1A558__INCLUDED_)
