#include "goby/middleware/group.h"

#ifndef GROUPS_20190710H
#define GROUPS_20190710H

namespace lamss
{
namespace groups
{
constexpr goby::middleware::Group pAcommsHandler_multiplex{"lamss::goby::pAcommsHandler_multiplex"};
constexpr goby::middleware::Group acomms_ack_original{"lamss::moos::ACOMMS_ACK_ORIGINAL"};
constexpr goby::middleware::Group acomms_expire{"lamss::moos::ACOMMS_EXPIRE"};
constexpr goby::middleware::Group acomms_mac_slot_start{"lamss::moos::ACOMMS_MAC_SLOT_START"};

} // namespace groups
} // namespace lamss

#endif
