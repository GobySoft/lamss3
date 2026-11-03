// Copyright 2026 GobySoft, LLC
// SPDX-License-Identifier: GPL-3.0-or-later

// Loads every lamss.sonar DCCL message, with lamss.core's, into one codec and checks it against
// DCCL_IDS.md (see lamss_dccl_test.h), then round-trips a LAMSS_CONTACT to check the encoding end to end.

#include "lamss/core/environment_update.pb.h"
#include "lamss/core/status.pb.h"
#include "lamss/core/status_topside.pb.h"
#include "lamss/core/view_point.pb.h"
#include "lamss/core/view_polygon.pb.h"
#include "lamss/core/view_seglist.pb.h"
#include "lamss/sonar/active_sonar_parameters.pb.h"
#include "lamss/sonar/array_navigation.pb.h"
#include "lamss/sonar/beamforming_vars.pb.h"
#include "lamss/sonar/btr.pb.h"
#include "lamss/sonar/contact.pb.h"
#include "lamss/sonar/pinger.pb.h"
#include "lamss/sonar/processing.pb.h"
#include "lamss/sonar/sonar_processing_command.pb.h"
#include "lamss/sonar/targetsim.pb.h"
#include "lamss/sonar/track.pb.h"
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
             lamss::sonar::ActiveSonarParameters::descriptor(),
             lamss::sonar::ArrayNavigation::descriptor(),
             lamss::sonar::BeamformingVars::descriptor(),
             lamss::sonar::BTR::descriptor(),
             lamss::sonar::LAMSS_CONTACT::descriptor(),
             lamss::sonar::LAMSS_PINGER_COMMAND::descriptor(),
             lamss::sonar::LAMSS_PROCESSING::descriptor(),
             lamss::sonar::SonarProcessingCommand::descriptor(),
             lamss::sonar::LAMSS_SIMULATE_TARGET::descriptor(),
             lamss::sonar::LAMSS_TRACK::descriptor(),
         })
        check.load_file(desc->file());
    LAMSS_CHECK(check.all_registered());
    LAMSS_CHECK(check.loaded_all_of("core"));
    LAMSS_CHECK(check.loaded_all_of("sonar"));

    lamss::sonar::LAMSS_CONTACT contact;
    contact.set_bearing(123.4);
    std::string bytes;
    check.codec().encode(&bytes, contact);
    lamss::sonar::LAMSS_CONTACT decoded;
    check.codec().decode(bytes, &decoded);
    LAMSS_CHECK(decoded.bearing() > 123.3 && decoded.bearing() < 123.5);
    return 0;
}
