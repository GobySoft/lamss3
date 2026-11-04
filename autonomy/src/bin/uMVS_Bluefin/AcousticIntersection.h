// Copyright 2001-2005 Massachusetts Institute of Technology and Oxford University
// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#if !defined(AFX_ACOUSTICINTERSECTION_H__F1518DFE_9012_407B_801A_50D801F0176C__INCLUDED_)
#define AFX_ACOUSTICINTERSECTION_H__F1518DFE_9012_407B_801A_50D801F0176C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <cstddef>

class CAcousticNode;
class CAcousticSignal;

class CAcousticIntersection
{
  public:
    CAcousticIntersection();
    CAcousticIntersection(double dfETA, CAcousticSignal* pSignal = NULL,
                          CAcousticNode* pNode = NULL);
    virtual ~CAcousticIntersection();

    CAcousticNode* m_pNode;
    CAcousticSignal* m_pSignal;
    bool operator<(const CAcousticIntersection& Obj) const;
    double m_dfETA;
};

#endif // !defined(AFX_ACOUSTICINTERSECTION_H__F1518DFE_9012_407B_801A_50D801F0176C__INCLUDED_)
