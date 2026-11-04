// Copyright 2001-2005 Massachusetts Institute of Technology and Oxford University
// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#if !defined(AFX_MOOSTERRAIN_H__E6A556D5_E919_4B46_9EDF_506259A8A7EF__INCLUDED_)
#define AFX_MOOSTERRAIN_H__E6A556D5_E919_4B46_9EDF_506259A8A7EF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <vector>
//using namespace std;

typedef std::vector<std::vector<double>> STLMATRIX;

class CMOOSTerrain
{
  public:
    double GetAltitude(double dfX, double dfY, double dfZ);
    CMOOSTerrain();
    virtual ~CMOOSTerrain();
    bool Load(const char* sFileName);
    STLMATRIX m_Data;

    std::vector<double> m_X;
    std::vector<double> m_Y;

  protected:
    double m_dfMinZ;
    double m_dfMaxZ;
};

#endif // !defined(AFX_MOOSTERRAIN_H__E6A556D5_E919_4B46_9EDF_506259A8A7EF__INCLUDED_)
