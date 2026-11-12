// Copyright 2001-2005 Massachusetts Institute of Technology and Oxford University
// Copyright 2011-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef AcousticDefsh
#define AcousticDefsh

enum AcousticChannel
{
    ACOUSTIC_CHAN_ERROR = -999,
    ACOUSTIC_CHAN_IRF = -4,
    ACOUSTIC_CHAN_IIF = -3,
    ACOUSTIC_CHAN_CRF = -2,
    ACOUSTIC_CHAN_CIF = -1,
    ACOUSTIC_CHAN_1 = 1,
    ACOUSTIC_CHAN_2,
    ACOUSTIC_CHAN_3,
    ACOUSTIC_CHAN_4,
    ACOUSTIC_CHAN_5,
    ACOUSTIC_CHAN_6,
    ACOUSTIC_CHAN_7,
    ACOUSTIC_CHAN_8,
    ACOUSTIC_CHAN_9,
    ACOUSTIC_CHAN_10,
    ACOUSTIC_CHAN_11,
    ACOUSTIC_CHAN_12,
};

enum AcousticMode
{
    ACOUSTIC_MODE_TRANSCEIVER,
    ACOUSTIC_MODE_RESPONDER,
};
#endif
