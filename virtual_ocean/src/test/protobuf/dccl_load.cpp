// Copyright 2026 GobySoft, LLC
// SPDX-License-Identifier: GPL-3.0-or-later

// Loads every lamss.virtual_ocean DCCL message, with lamss.core's, into one codec and checks it
// against DCCL_IDS.md (see lamss_dccl_test.h), then round-trips a LAMSS_SIMULATE_TARGET to check
// the encoding end to end.

#include "lamss/core/environment_update.pb.h"
#include "lamss/core/status.pb.h"
#include "lamss/core/status_topside.pb.h"
#include "lamss/core/view_point.pb.h"
#include "lamss/core/view_polygon.pb.h"
#include "lamss/core/view_seglist.pb.h"
#include "lamss/virtual_ocean/modem_sim.pb.h"
#include "lamss/virtual_ocean/pinger.pb.h"
#include "lamss/virtual_ocean/targetsim.pb.h"
#include "lamss_dccl_test.h"

int main()
{
    lamss::test::DCCLRegistryCheck check;
    LAMSS_CHECK(check.registry_valid());

    for (const auto* desc : {
             lamss::core::LAMSS_ENVIRONMENT_UPDATE::descriptor(),
             lamss::core::LAMSS_STATUS::descriptor(),
             lamss::core::LAMSS_STATUS_NMEA_SHIP::descriptor(),
             lamss::core::VIEW_POINT::descriptor(),
             lamss::core::VIEW_POLYGON::descriptor(),
             lamss::core::VIEW_SEGLIST::descriptor(),
             lamss::virtual_ocean::TransmitRequest::descriptor(),
             lamss::virtual_ocean::LAMSS_PINGER_COMMAND::descriptor(),
             lamss::virtual_ocean::LAMSS_SIMULATE_TARGET::descriptor(),
         })
        check.load_file(desc->file());
    LAMSS_CHECK(check.all_registered());
    LAMSS_CHECK(check.loaded_all_of("core"));
    LAMSS_CHECK(check.loaded_all_of("virtual_ocean"));

    lamss::virtual_ocean::LAMSS_SIMULATE_TARGET target;
    target.set_tgt_dep(42.4);
    std::string bytes;
    check.codec().encode(&bytes, target);
    lamss::virtual_ocean::LAMSS_SIMULATE_TARGET decoded;
    check.codec().decode(bytes, &decoded);
    LAMSS_CHECK(decoded.tgt_dep() == 42);
    return 0;
}
