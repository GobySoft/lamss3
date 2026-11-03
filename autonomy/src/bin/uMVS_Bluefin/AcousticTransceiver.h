// Copyright 2001-2005 Massachusetts Institute of Technology and Oxford University
// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#if !defined(AFX_ACOUSTICTRANSCEIVER_H__0DE9C296_01AE_460B_9163_3B88F35EB99F__INCLUDED_)
#define AFX_ACOUSTICTRANSCEIVER_H__0DE9C296_01AE_460B_9163_3B88F35EB99F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <list>
using namespace std;

#include "AcousticNode.h"

class CAcousticTransceiver : public CAcousticNode
{
  public:
    bool Iterate(double dfTimeNow);
    bool IsReceiving(double dfTimeNow);
    bool Ping(double dfTimeNow);
    CAcousticTransceiver();
    virtual ~CAcousticTransceiver();
    virtual bool OnAcousticHit(CAcousticSignal& Signal, double dfTime);
    double m_dfRxWindow;
    double m_dfLastPingTime;

  protected:
    //local class
    class CPingReply
    {
      public:
        AcousticChannel m_eChannel;
        double m_dfTOF;
        double m_dfRxTime;
        string m_sResponder;
    };
    typedef list<CPingReply> PING_REPLY_LIST;

    PING_REPLY_LIST m_Replies;
    bool m_bIsReceiving;
    bool LogReply(CPingReply& rReply);
};

#endif // !defined(AFX_ACOUSTICTRANSCEIVER_H__0DE9C296_01AE_460B_9163_3B88F35EB99F__INCLUDED_)
