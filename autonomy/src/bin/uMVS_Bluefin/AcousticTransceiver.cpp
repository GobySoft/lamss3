// Copyright 2001-2005 Massachusetts Institute of Technology and Oxford University
// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#ifdef _WIN32
#pragma warning(disable : 4786)
#endif

#include "AcousticTransceiver.h"
#include "SimEntity.h"
#include "SimGlobalHelper.h"
#include <iomanip>

#define DEFAULT_TCVR_RX_WINDOW 3.0
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CAcousticTransceiver::CAcousticTransceiver()
{
    m_dfRxWindow = DEFAULT_TCVR_RX_WINDOW;
    SetRxChan(ACOUSTIC_CHAN_CRF);
    SetRxChan(ACOUSTIC_CHAN_1);
    SetRxChan(ACOUSTIC_CHAN_2);
    SetRxChan(ACOUSTIC_CHAN_3);
    SetRxChan(ACOUSTIC_CHAN_4);
    SetRxChan(ACOUSTIC_CHAN_5);
    SetRxChan(ACOUSTIC_CHAN_6);
    SetRxChan(ACOUSTIC_CHAN_7);
    SetRxChan(ACOUSTIC_CHAN_8);
    SetRxChan(ACOUSTIC_CHAN_9);
    SetRxChan(ACOUSTIC_CHAN_10);
    SetRxChan(ACOUSTIC_CHAN_11);
    SetRxChan(ACOUSTIC_CHAN_12);

    SetTxChan(ACOUSTIC_CHAN_CIF);
}

CAcousticTransceiver::~CAcousticTransceiver() {}

bool CAcousticTransceiver::OnAcousticHit(CAcousticSignal& Signal, double dfTime)
{
#ifdef VERBOSE

    MOOSTrace("CAcousticTransceiver::OnAcousticHit on %s Signal[%d] from %s on Channel %d\n",
              GetFullName().c_str(), Signal.m_nID, Signal.GetSrcName().c_str(),
              Signal.GetChannel());
#endif
    if (IsReceiving(dfTime) && Listening(Signal.GetChannel()))
    {
        //this is a catch for auto-excitation
        std::string sTmp = Signal.GetSrcName();
        std::string sSrcVeh = MOOSChomp(sTmp, "/");
        if (sSrcVeh == GetParent()->GetName())
            return false;

        double dfTOF = dfTime - m_dfLastPingTime;

        //here we corrupt the reply..
        dfTOF += MOOSWhiteNoise(1) * m_pParams->m_dfTOFStd;

        //and occasionally add multipath
        double pMultiPath = m_pParams->m_dfProbMultiPath;

        if (pMultiPath > 0)
        {
            double dfRV = MOOSWhiteNoise(1);
            double dfThreshold = MOOSNormalInv(pMultiPath);
            if (dfRV < dfThreshold)
            {
                dfTOF += (50.0 + (20.0 * dfRV * dfRV)) / 1500.0;

                MOOSTrace("Making outlier TOF = %f @ %f\n", dfTOF, dfTime);
            }
        }

        if (dfTOF < m_dfRxWindow)
        {
            CPingReply Reply;

            Reply.m_dfTOF = dfTOF;
            Reply.m_eChannel = Signal.GetChannel();
            Reply.m_sResponder = Signal.GetSrcName();
            Reply.m_dfRxTime = dfTime;

            Matrix NodePos;

            GetParent()->GetNodePosition(*this, 0, NodePos);

#ifdef VERBOSE
            MOOSTrace("HIT! T = %f\n", dfTime);
            MOOSTraceMatrix(NodePos, " Received @ \n");
#endif

            if (m_pParams->m_bImmediateAcousticLog)
            {
                LogReply(Reply);
            }
            else
            {
                MOOSTrace("%s Gets a 2WR from %s on Channel %d TOF %f\n", GetFullName().c_str(),
                          Reply.m_sResponder.c_str(), Reply.m_eChannel, Reply.m_dfTOF);

                m_Replies.push_back(Reply);
            }
        }
    }
    return true;
}

bool CAcousticTransceiver::Ping(double dfTimeNow)
{
    m_dfLastPingTime = dfTimeNow;

    m_Replies.clear();

    Matrix NodePos;

    GetParent()->GetNodePosition(*this, 0, NodePos);

    CAcousticSignal Ping(NodePos(1, 1), NodePos(2, 1), NodePos(3, 1), dfTimeNow);

#ifdef VERBOSE
    MOOSTrace("\n*********\nPING! T = %f\n", dfTimeNow);
    MOOSTraceMatrix(NodePos, "Pinged from \n");
#endif

    Ping.SetSrcName(GetFullName());

    Ping.SetChannel(GetTxChannel());
#ifdef VERBOSE

    MOOSTrace("%s launches ping[%d] on Channel %d\n", GetFullName().c_str(), Ping.m_nID,
              GetTxChannel());
#endif

    return m_pEnvironment->AddSignal(Ping);
}

bool CAcousticTransceiver::IsReceiving(double /*dfTimeNow*/) { return m_bIsReceiving; }

bool CAcousticTransceiver::Iterate(double dfTimeNow)
{
    if (dfTimeNow - m_dfLastPingTime < m_dfRxWindow && m_dfLastPingTime != -1)
    {
        m_bIsReceiving = true;
    }
    else
    {
        //window has closed - time to publish results
        PING_REPLY_LIST::iterator p;

        for (p = m_Replies.begin(); p != m_Replies.end(); p++)
        {
            CPingReply& rReply = *p;

            if (!m_pParams->m_bImmediateAcousticLog)
            {
                LogReply(rReply);
            }
        }

        m_bIsReceiving = false;

        //now ping again
        Ping(dfTimeNow);
    }

    return true;
}

bool CAcousticTransceiver::LogReply(CAcousticTransceiver::CPingReply& rReply)
{
    //    MOOSTrace("Reply on chan[%d] TOF = %10.5f\n",rReply.m_eChannel,rReply.m_dfTOF);

    ostringstream os;

    //default to 3 dp
    os.setf(ios::fixed, ios::floatfield);
    os << setprecision(3);

    //begin output...firslt what kind of record is this?
    os << "TwoWayRange"
       << ","

       //when
       << "T=" << m_pEnvironment->GetElapsedTime(rReply.m_dfRxTime)
       << ","
       //"Tx="<<rReply.m_dfRxTime-rReply.m_dfTOF<<","

       //who recieved (ourselves)
       << "RxNode=" << GetFullName().c_str()
       << ","

       //higher precision for TOF
       << setprecision(5) << "TOF=" << rReply.m_dfTOF << ","
       << setprecision(3)

       //what channel
       << "Ch=" << rReply.m_eChannel
       << ","

       //a bit of cheat information - tcvr only really gives above information
       //but we also say what beacon was involved to help simulator user
       << "[" << rReply.m_sResponder.c_str()
       << "]"

       //terminate
       << endl
       << ends;

    Log(os);

    //finally add to MOOS Messages (if required?)
    //simple single vehicle format
    ostringstream osSimple;

    osSimple.setf(ios::fixed);
    osSimple.precision(3);
    osSimple << "Tx=" << rReply.m_dfRxTime - rReply.m_dfTOF;

    osSimple.precision(6);
    osSimple << ",Ch[" << rReply.m_eChannel << "]=" << rReply.m_dfTOF;
    osSimple << ends;

    //send report
    m_pEnvironment->AddReport(GetParent()->m_sOutputPrefix + "LBL_TOF", osSimple.str());

    return true;
}
