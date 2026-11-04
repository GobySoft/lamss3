// Copyright 2001-2005 Massachusetts Institute of Technology and Oxford University
// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#if !defined(AFX_SIMLOGGABLE_H__DFE6E1E0_3F87_48E9_8AF3_4680BA7C77FC__INCLUDED_)
#define AFX_SIMLOGGABLE_H__DFE6E1E0_3F87_48E9_8AF3_4680BA7C77FC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "SimBase.h"

#include <fstream>
#include <sstream>

class CSimLoggable : public CSimBase
{
  public:
    void SpaceLine();
    CSimLoggable();
    virtual ~CSimLoggable();

  protected:
    bool Log(ostringstream& strstream);
};

#endif // !defined(AFX_SIMLOGGABLE_H__DFE6E1E0_3F87_48E9_8AF3_4680BA7C77FC__INCLUDED_)
