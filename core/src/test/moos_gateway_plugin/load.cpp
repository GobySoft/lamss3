// Copyright 2026 GobySoft, LLC
// SPDX-License-Identifier: GPL-3.0-or-later

// Opens the plugin the way goby_moos_gateway does (dlopen, then dlsym of the load/unload entry
// points) and checks that loading it registers the [lamss.core.goby_gateway_config] extension of
// GobyMOOSGatewayConfig, which the gateway needs before it reads its configuration file.

#include <dlfcn.h>
#include <iostream>

#include <google/protobuf/descriptor.h>

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
    void* handle = dlopen(LAMSS_MOOS_GATEWAY_PLUGIN, RTLD_LAZY | RTLD_NODELETE);
    if (!handle)
        std::cerr << dlerror() << std::endl;
    CHECK(handle);
    CHECK(dlsym(handle, "goby3_moos_gateway_load"));
    CHECK(dlsym(handle, "goby3_moos_gateway_unload"));

    const auto* extension = google::protobuf::DescriptorPool::generated_pool()->FindExtensionByName(
        "lamss.core.goby_gateway_config");
    CHECK(extension);
    CHECK(extension->containing_type()->full_name() ==
          "goby.apps.moos.protobuf.GobyMOOSGatewayConfig");
    CHECK(extension->number() == 1000);

    std::cout << "loaded " << LAMSS_MOOS_GATEWAY_PLUGIN << std::endl;
    return 0;
}
