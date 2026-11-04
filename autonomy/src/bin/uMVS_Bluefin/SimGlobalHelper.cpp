// Copyright 2001-2005 Massachusetts Institute of Technology and Oxford University
// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#ifdef _WIN32
#pragma warning(disable : 4786)
#endif

#include "SimGlobalHelper.h"
#include <MOOS/libMOOS/Utils/MOOSUtils.h>
#ifndef PI
#define PI 3.141592653589793
#endif

void MOOSTraceMatrix(Matrix& Mat, const char* Str)
{
    int nRows = Mat.Nrows();
    int nCols = Mat.Ncols();

    MOOSTrace("Matrix %s : [%d x %d]\n", Str, nRows, nCols);

    for (int i = 0; i < nRows; i++)
    {
        for (int j = 0; j < nCols; j++) { MOOSTrace("%09.5f  ", Mat.element(i, j)); }
        MOOSTrace(" \n");
    }
}
