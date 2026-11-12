// Copyright 2026 GobySoft, LLC
// SPDX-License-Identifier: GPL-3.0-or-later

// Loads every lamss.autonomy DCCL message, with lamss.core's, into one codec and checks it against
// DCCL_IDS.md (see lamss_dccl_test.h), then round-trips a LAMSS_DEPLOY to check the encoding end to end.

#include "lamss/autonomy/deploy.pb.h"
#include "lamss/autonomy/prosecute.pb.h"
#include "lamss/autonomy/stovepipe.pb.h"
#include "lamss/autonomy/transit.pb.h"
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

    for (const auto* desc : {
             lamss::core::LAMSS_ENVIRONMENT_UPDATE::descriptor(),
             lamss::core::LAMSS_STATUS::descriptor(),
             lamss::core::LAMSS_STATUS_NMEA_SHIP::descriptor(),
             lamss::core::VIEW_POINT::descriptor(),
             lamss::core::VIEW_POLYGON::descriptor(),
             lamss::core::VIEW_SEGLIST::descriptor(),
             lamss::autonomy::LAMSS_DEPLOY::descriptor(),
             lamss::autonomy::LAMSS_PROSECUTE::descriptor(),
             lamss::autonomy::LAMSS_STOVEPIPE::descriptor(),
             lamss::autonomy::LAMSS_TRANSIT::descriptor(),
         })
        check.load_file(desc->file());
    LAMSS_CHECK(check.all_registered());
    LAMSS_CHECK(check.loaded_all_of("core"));
    LAMSS_CHECK(check.loaded_all_of("autonomy"));

    lamss::autonomy::LAMSS_DEPLOY deploy;
    deploy.set_deploy_latitude(42.36);
    deploy.set_deploy_longitude(-71.09);
    std::string bytes;
    check.codec().encode(&bytes, deploy);
    lamss::autonomy::LAMSS_DEPLOY decoded;
    check.codec().decode(bytes, &decoded);
    LAMSS_CHECK(decoded.has_deploy_latitude() && decoded.has_deploy_longitude());
    return 0;
}
