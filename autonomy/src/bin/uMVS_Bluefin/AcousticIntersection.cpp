// Copyright 2001-2005 Massachusetts Institute of Technology and Oxford University
// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#ifdef _WIN32
#pragma warning(disable : 4786)
#endif

#include "AcousticIntersection.h"
#include "AcousticNode.h"
#include "AcousticSignal.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CAcousticIntersection::CAcousticIntersection()
{
    m_pNode = NULL;
    m_pSignal = NULL;
    m_dfETA = -1.0;
}

CAcousticIntersection::CAcousticIntersection(double dfETA, CAcousticSignal* pSignal,
                                             CAcousticNode* pNode)
{
    m_pNode = pNode;
    m_pSignal = pSignal;
    m_dfETA = dfETA;
}

CAcousticIntersection::~CAcousticIntersection() {}

bool CAcousticIntersection::operator<(const CAcousticIntersection& Obj) const
{
    return m_dfETA < Obj.m_dfETA;
}
