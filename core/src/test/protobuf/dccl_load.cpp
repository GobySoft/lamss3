// Copyright 2026 GobySoft, LLC
// SPDX-License-Identifier: GPL-3.0-or-later

// Loads every lamss.core DCCL message into one codec, which fails on invalid DCCL options or on
// two messages sharing an ID, then round-trips a VIEW_POINT to check the encoding end to end.

#include <iostream>
#include <set>

#include <dccl/codec.h>

#include "lamss/core/environment_update.pb.h"
#include "lamss/core/status.pb.h"
#include "lamss/core/status_topside.pb.h"
#include "lamss/core/view_point.pb.h"
#include "lamss/core/view_polygon.pb.h"
#include "lamss/core/view_seglist.pb.h"

#define CHECK(condition)                                                              \
    do {                                                                              \
        if (!(condition))                                                             \
        {                                                                             \
            std::cerr << __FILE__ << ":" << __LINE__ << ": CHECK failed: " #condition \
                      << std::endl;                                                   \
            return 1;                                                                 \
        }                                                                             \
    } while (false)

int main()
{
    dccl::Codec codec;
    std::set<unsigned> ids;
    auto load = [&](const google::protobuf::Descriptor* desc)
    {
        codec.load(desc);
        ids.insert(codec.id(desc));
    };

    load(lamss::core::LAMSS_STATUS::descriptor());
    load(lamss::core::LAMSS_STATUS_NMEA_BUOY::descriptor());
    load(lamss::core::LAMSS_STATUS_NMEA_SHIP::descriptor());
    load(lamss::core::LAMSS_ENVIRONMENT_UPDATE::descriptor());
    load(lamss::core::VIEW_POINT::descriptor());
    load(lamss::core::VIEW_POLYGON::descriptor());
    load(lamss::core::VIEW_SEGLIST::descriptor());
    CHECK(ids.size() == 7);

    lamss::core::VIEW_POINT point;
    point.set__time(1.7e9);
    std::string bytes;
    codec.encode(&bytes, point);
    lamss::core::VIEW_POINT decoded;
    codec.decode(bytes, &decoded);
    CHECK(decoded.has__time());
    return 0;
}
