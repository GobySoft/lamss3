// Copyright 2026 GobySoft, LLC
// SPDX-License-Identifier: GPL-3.0-or-later

// Loads every lamss.core DCCL message into one codec, which fails on invalid DCCL options or on
// two messages sharing an ID, checks each message's ID against the registry in DCCL_IDS.md (which
// must list it, inside one of lamss3's blocks), then round-trips a VIEW_POINT to check the
// encoding end to end.

#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <string>

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

namespace
{
bool in_lamss_block(unsigned id) { return (id >= 20 && id <= 23) || (id >= 5120 && id <= 5375); }

// "| 5120 | `lamss.core.VIEW_POINT` | core |" -> {"lamss.core.VIEW_POINT", 5120}
std::map<std::string, unsigned> read_registry(const std::string& path)
{
    std::map<std::string, unsigned> registry;
    std::ifstream file(path);
    const std::regex row(R"(^\|\s*(\d+)\s*\|\s*`([\w.]+)`\s*\|)");
    std::smatch match;
    for (std::string line; std::getline(file, line);)
        if (std::regex_search(line, match, row))
            registry[match[2]] = std::stoul(match[1]);
    return registry;
}
} // namespace

int main()
{
    const auto registry = read_registry(LAMSS_DCCL_IDS_MD);
    CHECK(!registry.empty());
    std::set<unsigned> registry_ids;
    for (const auto& [name, id] : registry)
    {
        CHECK(in_lamss_block(id));
        CHECK(registry_ids.insert(id).second);
    }

    dccl::Codec codec;
    std::set<unsigned> ids;
    bool all_registered = true;
    auto load = [&](const google::protobuf::Descriptor* desc)
    {
        codec.load(desc);
        const auto id = static_cast<unsigned>(codec.id(desc));
        ids.insert(id);
        auto it = registry.find(desc->full_name());
        if (it == registry.end() || it->second != id)
        {
            std::cerr << desc->full_name() << " has DCCL ID " << id
                      << ", which DCCL_IDS.md does not list for it" << std::endl;
            all_registered = false;
        }
    };

    load(lamss::core::LAMSS_STATUS::descriptor());
    load(lamss::core::LAMSS_STATUS_NMEA_BUOY::descriptor());
    load(lamss::core::LAMSS_STATUS_NMEA_SHIP::descriptor());
    load(lamss::core::LAMSS_ENVIRONMENT_UPDATE::descriptor());
    load(lamss::core::VIEW_POINT::descriptor());
    load(lamss::core::VIEW_POLYGON::descriptor());
    load(lamss::core::VIEW_SEGLIST::descriptor());
    CHECK(ids.size() == 7);
    CHECK(all_registered);

    lamss::core::VIEW_POINT point;
    point.set__time(1.7e9);
    std::string bytes;
    codec.encode(&bytes, point);
    lamss::core::VIEW_POINT decoded;
    codec.decode(bytes, &decoded);
    CHECK(decoded.has__time());
    return 0;
}
