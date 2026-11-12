// Copyright 2001-2005 Massachusetts Institute of Technology and Oxford University
// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#include "MOOSTerrain.h"

#include <fstream>
#include <iostream>
using namespace std;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMOOSTerrain::CMOOSTerrain()
{
    m_dfMinZ = 1e10;
    m_dfMaxZ = -1e10;
}

CMOOSTerrain::~CMOOSTerrain() {}

bool CMOOSTerrain::Load(const char* sFileName)
{
    ifstream In;

    In.open(sFileName);

    if (In.is_open())
    {
        double dfRows;
        double dfCols;

        In >> dfRows;
        In >> dfCols;
        //        m_Data.ReSize(dfRows,dfCols);

        m_Data.resize((int)dfRows + 1);

        STLMATRIX::iterator p;

        for (p = m_Data.begin(); p != m_Data.end(); p++) { p->resize((int)(dfCols + 1)); }

        m_X.resize((int)dfCols + 1);
        m_Y.resize((int)dfRows + 1);

        double dfTmp;
        int i, j;
        for (i = 1; i <= dfCols; i++)
        {
            In >> dfTmp;
            m_X[i] = dfTmp;
        }
        for (i = 1; i <= dfRows; i++)
        {
            In >> dfTmp;
            m_Y[i] = dfTmp;
        }
        for (i = 1; i <= dfRows; i++)
        {
            for (j = 1; j <= dfCols; j++)
            {
                In >> dfTmp;

                m_dfMinZ = dfTmp < m_dfMinZ ? dfTmp : m_dfMinZ;
                m_dfMaxZ = dfTmp > m_dfMaxZ ? dfTmp : m_dfMaxZ;

                m_Data[i][j] = dfTmp;
            }
        }

        return true;
    }
    else
    {
        return false;
    }
}

double CMOOSTerrain::GetAltitude(double dfX, double dfY, double dfZ)
{
    double dfAltitude = -1;

    //find x and y
    int n = m_X.size() - 1;
    int I = -1;
    double dfAlphaI = 0;
    for (int i = 2; i <= n; i++)
    {
        if (m_X[i - 1] <= dfX && m_X[i] >= dfX)
        {
            I = i;
            if (m_X[i - 1] != m_X[i])
            {
                dfAlphaI = (dfX - m_X[i - 1]) / (m_X[i] - m_X[i - 1]);
            }
            break;
        }
    }

    n = m_Y.size() - 1;
    double dfAlphaJ = 0;
    int J = -1;
    for (int j = 2; j <= n; j++)
    {
        if (m_Y[j - 1] <= dfY && m_Y[j] >= dfY)
        {
            J = j;
            if (m_Y[j - 1] != m_Y[j])
            {
                dfAlphaJ = (dfY - m_Y[j - 1]) / (m_Y[j] - m_Y[j - 1]);
            }
            break;
        }
    }

    if (J != -1 && I != -1)
    {
        double Z0 = m_Data[I - 1][J - 1];
        double Z1 = m_Data[I][J - 1];
        double Z2 = m_Data[I][J];
        double Z3 = m_Data[I - 1][J];

        dfAltitude = dfZ - ((1 - dfAlphaI) * (1 - dfAlphaJ) * Z0 + (1 - dfAlphaI) * dfAlphaJ * Z3 +
                            dfAlphaI * (1 - dfAlphaJ) * Z1 + dfAlphaI * dfAlphaJ * Z2);
    }

    return dfAltitude;
}
