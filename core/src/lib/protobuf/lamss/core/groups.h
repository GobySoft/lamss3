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

constexpr goby::middleware::Group lamss_status_in{"lamss::moos::LAMSS_STATUS_IN"};

constexpr goby::middleware::Group transmission_grp_vel{"lamss::moos::TRANSMISSION_GRP_VEL"};

constexpr goby::middleware::Group node_report{"lamss::moos::NODE_REPORT"};
constexpr goby::middleware::Group node_report_out{"lamss::moos::NODE_REPORT::out"};

constexpr goby::middleware::Group view_geometry{"lamss::moos::view_geometry"};

constexpr goby::middleware::Group acomms_raw_in{"lamss::moos::acomms_raw_in"};
constexpr goby::middleware::Group acomms_raw_out{"lamss::moos::acomms_raw_out"};

constexpr goby::middleware::Group hydroman_health{"lamss::groups::hydroman_health"};

constexpr goby::middleware::Group objfunc_request{"lamss::objfunc_request"};
constexpr goby::middleware::Group objfunc_response{"lamss::objfunc_response"};

} // namespace groups
} // namespace lamss

#endif
