// Copyright 2026 GobySoft, LLC
// SPDX-License-Identifier: GPL-3.0-or-later

// Loads every lamss.core DCCL message into one codec and checks it against DCCL_IDS.md (see
// lamss_dccl_test.h), then round-trips a VIEW_POINT to check the encoding end to end.

#include "lamss/core/environment_update.pb.h"
#include "lamss/core/status.pb.h"
#include "lamss/core/status_topside.pb.h"
#include "lamss/core/view_point.pb.h"
#include "lamss/core/view_polygon.pb.h"
#include "lamss/core/view_seglist.pb.h"
#include "lamss_dccl_test.h"

int main()
{
    lamss::test::DCCLRegistryCheck check;
    LAMSS_CHECK(check.registry_valid());

    check.load(lamss::core::LAMSS_STATUS::descriptor());
    check.load(lamss::core::LAMSS_STATUS_NMEA_BUOY::descriptor());
    check.load(lamss::core::LAMSS_STATUS_NMEA_SHIP::descriptor());
    check.load(lamss::core::LAMSS_ENVIRONMENT_UPDATE::descriptor());
    check.load(lamss::core::VIEW_POINT::descriptor());
    check.load(lamss::core::VIEW_POLYGON::descriptor());
    check.load(lamss::core::VIEW_SEGLIST::descriptor());
    LAMSS_CHECK(check.all_registered());
    LAMSS_CHECK(check.loaded_all_of("core"));

    lamss::core::VIEW_POINT point;
    point.set__time(1.7e9);
    std::string bytes;
    check.codec().encode(&bytes, point);
    lamss::core::VIEW_POINT decoded;
    check.codec().decode(bytes, &decoded);
    LAMSS_CHECK(decoded.has__time());
    return 0;
}
