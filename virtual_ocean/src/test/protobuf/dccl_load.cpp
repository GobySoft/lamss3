// Copyright 2026 GobySoft, LLC
// SPDX-License-Identifier: GPL-3.0-or-later

// lamss.virtual_ocean has no DCCL message yet: checks that none of its files declares one that
// DCCL_IDS.md does not list, with lamss.core's messages loaded into the same codec.

#include "lamss/core/environment_update.pb.h"
#include "lamss/core/status.pb.h"
#include "lamss/core/status_topside.pb.h"
#include "lamss/core/view_point.pb.h"
#include "lamss/core/view_polygon.pb.h"
#include "lamss/core/view_seglist.pb.h"
#include "lamss/virtual_ocean/modem_sim.pb.h"
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
         })
        check.load_file(desc->file());
    LAMSS_CHECK(check.all_registered());
    LAMSS_CHECK(check.loaded_all_of("core"));
    LAMSS_CHECK(check.loaded_all_of("virtual_ocean"));
    return 0;
}
