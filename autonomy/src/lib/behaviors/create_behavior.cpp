// Copyright 2026 GobySoft, LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#include "depth/ConstantAltitude/BHV_ConstantAltitude.h"
#include "depth/ConstantBuoyancy/BHV_ConstantBuoyancy.h"
#include "depth/MinAltitude/BHV_MinAltitude.h"
#include "depth/SmartYoyo/BHV_SmartYoyo.h"
#include "survey/RaceTrack/BHV_RaceTrack.h"
#include "tracking/MaintainBearing/BHV_MaintainBearing.h"
#include "tracking/WiggleTail/BHV_WiggleTail.h"
#include "tracking/ZigZag/BHV_ZigZag.h"
#ifdef LAMSS_AUTONOMY_HAVE_NETSIM
#include "depth/OptAcommsDepth/BHV_OptAcommsDepth.h"
#endif

// The IvP helm loads one `createBehavior` from each libBHV_<name>.so it finds and passes it
// "BHV_<name>"; every libBHV_<name>.so is a link to this library.
extern "C" IvPBehavior* createBehavior(std::string name, IvPDomain domain)
{
    using namespace lamss::autonomy;
    if (name == "BHV_ConstantAltitude")
        return new BHV_ConstantAltitude(domain);
    if (name == "BHV_ConstantBuoyancy")
        return new BHV_ConstantBuoyancy(domain);
    if (name == "BHV_MinAltitude")
        return new BHV_MinAltitude(domain);
    if (name == "BHV_SmartYoyo")
        return new BHV_SmartYoyo(domain);
    if (name == "BHV_RaceTrack")
        return new BHV_RaceTrack(domain);
    if (name == "BHV_MaintainBearing")
        return new BHV_MaintainBearing(domain);
    if (name == "BHV_WiggleTail")
        return new BHV_WiggleTail(domain);
    // MOOS-IvP has its own, unrelated, BHV_ZigZag, which the helm prefers
    if (name == "BHV_LamssZigZag")
        return new BHV_ZigZag(domain);
#ifdef LAMSS_AUTONOMY_HAVE_NETSIM
    if (name == "BHV_OptAcommsDepth")
        return new BHV_OptAcommsDepth(domain);
#endif
    return nullptr;
}
