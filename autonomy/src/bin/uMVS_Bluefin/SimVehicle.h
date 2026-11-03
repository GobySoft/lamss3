// Copyright 2001-2005 Massachusetts Institute of Technology and Oxford University
// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#if !defined(AFX_SIMVEHICLE_H__A06CD223_5B8E_407A_9D83_CF5E7C11E347__INCLUDED_)
#define AFX_SIMVEHICLE_H__A06CD223_5B8E_407A_9D83_CF5E7C11E347__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "SimEntity.h"

class CSimVehicle : public CSimEntity
{
  public:
    CSimVehicle();
    virtual ~CSimVehicle();

    bool ConfigureResponder(bool bEnable, AcousticChannel eRx, AcousticChannel eTx, double dfTAT);
};

#endif // !defined(AFX_SIMVEHICLE_H__A06CD223_5B8E_407A_9D83_CF5E7C11E347__INCLUDED_)
