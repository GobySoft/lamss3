// Copyright 2026 GobySoft, LLC
// SPDX-License-Identifier: GPL-3.0-or-later

// For lamss_add_dccl_test: one dccl::Codec fails on invalid DCCL options or on two messages
// sharing an ID; DCCL_IDS.md is the registry of every toolbox's IDs.

#ifndef LAMSS_DCCL_TEST_H
#define LAMSS_DCCL_TEST_H

#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <string>

#include <dccl/codec.h>
#include <dccl/option_extensions.pb.h>

#define LAMSS_CHECK(condition)                                                        \
    do {                                                                              \
        if (!(condition))                                                             \
        {                                                                             \
            std::cerr << __FILE__ << ":" << __LINE__ << ": CHECK failed: " #condition \
                      << std::endl;                                                   \
            return 1;                                                                 \
        }                                                                             \
    } while (false)

namespace lamss
{
namespace test
{
struct RegistryRow
{
    unsigned id;
    std::string toolbox;
};

inline bool in_lamss_dccl_block(unsigned id)
{
    return (id >= 20 && id <= 23) || (id >= 5120 && id <= 5375);
}

// "| 5120 | `lamss.core.VIEW_POINT` | core |" -> {"lamss.core.VIEW_POINT", {5120, "core"}}
inline std::map<std::string, RegistryRow> read_dccl_registry(const std::string& path)
{
    std::map<std::string, RegistryRow> registry;
    std::ifstream file(path);
    const std::regex row(R"(^\|\s*(\d+)\s*\|\s*`([\w.]+)`\s*\|\s*(\w+)\s*\|)");
    std::smatch match;
    for (std::string line; std::getline(file, line);)
        if (std::regex_search(line, match, row))
            registry[match[2]] = {static_cast<unsigned>(std::stoul(match[1])), match[3]};
    return registry;
}

class DCCLRegistryCheck
{
  public:
    explicit DCCLRegistryCheck(const std::string& registry_path = LAMSS_DCCL_IDS_MD)
        : registry_(read_dccl_registry(registry_path))
    {
    }

    // non-empty, with unique IDs inside lamss3's blocks
    bool registry_valid() const
    {
        std::set<unsigned> ids;
        for (const auto& [name, row] : registry_)
        {
            if (!in_lamss_dccl_block(row.id) || !ids.insert(row.id).second)
            {
                std::cerr << "DCCL_IDS.md: " << name << " has ID " << row.id
                          << ", which is outside lamss3's blocks or listed twice" << std::endl;
                return false;
            }
        }
        return !registry_.empty();
    }

    void load(const google::protobuf::Descriptor* desc)
    {
        codec_.load(desc);
        const auto id = static_cast<unsigned>(codec_.id(desc));
        loaded_.insert(desc->full_name());
        auto it = registry_.find(desc->full_name());
        if (it == registry_.end() || it->second.id != id)
        {
            std::cerr << desc->full_name() << " has DCCL ID " << id
                      << ", which DCCL_IDS.md does not list for it" << std::endl;
            all_registered_ = false;
        }
    }

    // every top-level message of file that has a DCCL ID
    void load_file(const google::protobuf::FileDescriptor* file)
    {
        for (int i = 0, n = file->message_type_count(); i < n; ++i)
        {
            const auto* desc = file->message_type(i);
            if (desc->options().GetExtension(dccl::msg).has_id())
                load(desc);
        }
    }

    // every loaded message is listed in DCCL_IDS.md with its ID
    bool all_registered() const { return all_registered_ && !loaded_.empty(); }

    // every message DCCL_IDS.md lists for toolbox was loaded
    bool loaded_all_of(const std::string& toolbox) const
    {
        bool all = true;
        for (const auto& [name, row] : registry_)
        {
            if (row.toolbox == toolbox && !loaded_.count(name))
            {
                std::cerr << "DCCL_IDS.md lists " << name << " for " << toolbox
                          << ", but the test did not load it" << std::endl;
                all = false;
            }
        }
        return all;
    }

    dccl::Codec& codec() { return codec_; }

  private:
    std::map<std::string, RegistryRow> registry_;
    std::set<std::string> loaded_;
    bool all_registered_{true};
    dccl::Codec codec_;
};
} // namespace test
} // namespace lamss

#endif
