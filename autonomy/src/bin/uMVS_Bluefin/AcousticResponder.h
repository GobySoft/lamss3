// Copyright 2001-2005 Massachusetts Institute of Technology and Oxford University
// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#if !defined(AFX_ACOUSTICRESPONDER_H__D3E25A26_3864_430D_8857_4B32A096BED8__INCLUDED_)
#define AFX_ACOUSTICRESPONDER_H__D3E25A26_3864_430D_8857_4B32A096BED8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "AcousticNode.h"

class CAcousticResponder : public CAcousticNode
{
  public:
    CAcousticResponder();
    virtual ~CAcousticResponder();
    virtual bool OnAcousticHit(CAcousticSignal& Signal, double dfTime);
    bool SetTAT(double dfTAT);
    double GetTAT();

    double m_dfTAT;
};

#endif // !defined(AFX_ACOUSTICRESPONDER_H__D3E25A26_3864_430D_8857_4B32A096BED8__INCLUDED_)
