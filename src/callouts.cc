#include <asiolink/io_address.h>
#include <dhcp/pkt4.h>
#include <dhcpsrv/lease.h>
#include <dhcpsrv/lease_mgr.h>
#include <dhcpsrv/lease_mgr_factory.h>
#include <hooks/hooks.h>
#include <pgsql/pgsql_connection.h>

#include <algorithm>
#include <string>
#include <tuple>

#include "globals.h"
#include "logger.h"

static std::tuple<int, std::string, std::string> parseOption82(
    const isc::dhcp::Pkt4Ptr& query4_ptr) {
  /* Get switch hostname and port name from DHCP option 82
   * Also check if option 82 is valid
   * https://www.cisco.com/c/en/us/td/docs/switches/lan/catalyst4500/12-2/15-02SG/configuration/guide/config/dhcp.html#57094
   */
  const auto& option82{query4_ptr->getOption(82)};
  if (!option82 || !option82->valid() || !option82->getOption(1) ||
      !option82->getOption(1)->valid() || !option82->getOption(2) ||
      !option82->getOption(2)->valid()) {
    LOG_FATAL(kea_hook_logger, KEA_HOOK_DHCP_OPTION_82_ERROR);
    return std::make_tuple(1, nullptr, nullptr);
  }

  const auto& circuit_id{option82->getOption(1)->getData()};
  const auto& remote_id{option82->getOption(2)->getData()};

  // Check suboption ID types
  if (circuit_id[0] != 0 || remote_id[0] != 1) {
    LOG_FATAL(kea_hook_logger, KEA_HOOK_DHCP_OPTION_82_ERROR);
    return std::make_tuple(1, nullptr, nullptr);
  }

  const auto& port_id{std::to_string(circuit_id[4]) + "/" +
                      std::to_string(circuit_id[5])};
  std::string switch_id{remote_id.begin() + 2, remote_id.end()};

  // Check if switch exists
  const char* values[1] = {switch_id.c_str()};
  isc::db::PgSqlResult r(PQexecPrepared(*g_pg_sql_connection, "check_switch", 1,
                                        values, nullptr, nullptr, 0));

  if (r.getRows() <= 0 || std::stoi(PQgetvalue(r, 0, 0)) <= 0) {
    LOG_FATAL(kea_hook_logger, KEA_HOOK_DHCP_OPTION_82_ERROR);
    return std::make_tuple(1, nullptr, nullptr);
  }

  return std::make_tuple(0, std::move(switch_id), port_id);
}

extern "C" {
// Check IP conflict
int pkt4_receive(isc::hooks::CalloutHandle& handle) {
  isc::dhcp::Pkt4Ptr query4_ptr;
  handle.getArgument("query4", query4_ptr);

  // Skip non MUEB devices
  const auto& hwaddr_ptr = query4_ptr->getHWAddr();
  const auto& mac_address{hwaddr_ptr->toText(false)};
  if (hwaddr_ptr->hwaddr_[0] != 0x54 || hwaddr_ptr->hwaddr_[1] != 0x10 ||
      hwaddr_ptr->hwaddr_[2] != 0xEC) {
    return 0;
  }

  const auto& [result, switch_id, port_id] = parseOption82(query4_ptr);
  if (result != 0) {
    return 1;
  }

  const char* values[2] = {port_id.c_str(), switch_id.c_str()};

  // Get room id
  isc::db::PgSqlResult r(PQexecPrepared(*g_pg_sql_connection, "mueb_in_room", 2,
                                        values, nullptr, nullptr, 0));
  // Handle incorrect room
  if (r.getRows() <= 0) {
    LOG_ERROR(kea_hook_logger, KEA_HOOK_UNKNOWN_ROOM)
        .arg(mac_address)
        .arg(switch_id)
        .arg(port_id);
    return 1;
  }

  const auto& room_id{PQgetvalue(r, 0, 0)};
  values[0] = mac_address.c_str();
  values[1] = room_id;

  // Check if multiple MUEBs(excluding current) is in the same room
  isc::db::PgSqlResult r2(PQexecPrepared(*g_pg_sql_connection,
                                         "mueb_count_in_room", 2, values,
                                         nullptr, nullptr, 0));

  try {
    if (std::stoi(PQgetvalue(r2, 0, 0)) > 0) {
      LOG_WARN(kea_hook_logger, KEA_HOOK_MULTIPLE_MUEB).arg(room_id);
    }

    isc::db::PgSqlResult r3(PQexecPrepared(*g_pg_sql_connection, "ip_conflict",
                                           1, values, nullptr, nullptr, 0));
    /* Drop packet when a MUEB has IP conflict
     * Needs manual fix
     */
    if (r3.getRows() > 0 && std::stoi(PQgetvalue(r3, 0, 0)) > 0) {
      handle.setStatus(isc::hooks::CalloutHandle::NEXT_STEP_DROP);
    }
  } catch (const std::exception& e) {
    LOG_ERROR(kea_hook_logger, KEA_HOOK_DATABASE_FAILED).arg(e.what());
    return 1;
  }

  return 0;
}

// handle IP lease
int lease4_select(isc::hooks::CalloutHandle& handle) {
  isc::dhcp::Pkt4Ptr query4_ptr;
  handle.getArgument("query4", query4_ptr);

  isc::dhcp::Lease4Ptr lease4_ptr;
  handle.getArgument("lease4", lease4_ptr);

  bool fake_allocation;
  handle.getArgument("fake_allocation", fake_allocation);

  LOG_DEBUG(kea_hook_logger, 0, KEA_HOOK_DHCP_STATE)
      .arg((fake_allocation) ? "---[DISCOVER]---" : "---[REQUEST]---");

  const auto& hwaddr_ptr = query4_ptr->getHWAddr();
  const auto& mac_address = hwaddr_ptr->toText(false);

  // Allocate IP for non MUEB devices
  if (hwaddr_ptr->hwaddr_[0] != 0x54 || hwaddr_ptr->hwaddr_[1] != 0x10 ||
      hwaddr_ptr->hwaddr_[2] != 0xEC) {
    LOG_INFO(kea_hook_logger, KEA_HOOK_NOT_MUEB).arg(mac_address);
    LOG_DEBUG(kea_hook_logger, 0, KEA_HOOK_DHCP_STATE)
        .arg((fake_allocation) ? "---<DISCOVER>---" : "---<REQUEST>---");

    return 0;
  }

  // Critical part begin, it's not safe to lease IP yet
  handle.setStatus(isc::hooks::CalloutHandle::NEXT_STEP_SKIP);
  try {
    isc::db::PgSqlTransaction transaction(*g_pg_sql_connection);

    auto [result, switch_id, port_id] = parseOption82(query4_ptr);
    if (result != 0) {
      return 1;
    }

    // Check for IP override
    const char* values[3] = {mac_address.c_str()};
    isc::db::PgSqlResult r(PQexecPrepared(*g_pg_sql_connection, "ip_override",
                                          1, values, nullptr, nullptr, 0));

    std::string ip_address;
    if (r.getRows() > 0) {
      // IP is overridden
      ip_address = PQgetvalue(r, 0, 0);
      LOG_INFO(kea_hook_logger, KEA_HOOK_IP_OVERRIDDEN)
          .arg(mac_address)
          .arg(ip_address);
    } else {
      values[0] = port_id.c_str();
      values[1] = switch_id.c_str();
      isc::db::PgSqlResult r2(PQexecPrepared(*g_pg_sql_connection, "ip_address",
                                             2, values, nullptr, nullptr, 0));

      // Handle incorrect room
      if (r2.getRows() <= 0) {
        LOG_ERROR(kea_hook_logger, KEA_HOOK_UNKNOWN_ROOM)
            .arg(mac_address)
            .arg(switch_id)
            .arg(port_id);

        return 1;
      }

      ip_address = PQgetvalue(r2, 0, 0);
    }

    // Modify kea's IP lease
    lease4_ptr->addr_ = isc::asiolink::IOAddress(ip_address);
    handle.setArgument("lease4", lease4_ptr);

    // Basic DHCP functionality ends here
    // Critical part end, it's safe to lease IP now
    handle.setStatus(isc::hooks::CalloutHandle::NEXT_STEP_CONTINUE);

    LOG_DEBUG(kea_hook_logger, 0, KEA_HOOK_QUERIED_IP).arg(ip_address);
    LOG_DEBUG(kea_hook_logger, 0, KEA_HOOK_DHCP_STATE)
        .arg((fake_allocation) ? "---<DISCOVER>---" : "---<REQUEST>---");

    // Save to DB when the DHCP state is DHCPREQUEST
    if (!fake_allocation) {
      // Clear switch-port
      isc::db::PgSqlResult r3(PQexecPrepared(*g_pg_sql_connection, "clear_port",
                                             2, values, nullptr, nullptr, 0));

      // Insert MUEB or update switch-port
      values[0] = mac_address.c_str();
      values[1] = port_id.c_str();
      values[2] = switch_id.c_str();
      isc::db::PgSqlResult r4(PQexecPrepared(*g_pg_sql_connection,
                                             "insert_or_update_mueb", 3, values,
                                             nullptr, nullptr, 0));
    } else {
      /* Handle device swap
       * Remove the current IP from the lease DB before DHCPREQUEST
       * This step is required to make the device swap work otherwise the server
       * sends DHCPNAK
       */
      if (isc::dhcp::LeaseMgrFactory::haveInstance()) {
        isc::dhcp::LeaseMgrFactory::instance().deleteLease(lease4_ptr);
      } else {
        /* Should not happen this needs manual fix
         * Proceed as normal
         */
        LOG_ERROR(kea_hook_logger, KEA_HOOK_FAILED)
            .arg(
                "No lease manager available, MUEB device swap will not work! "
                "This should not happen, check lease database configuration!");
      }
    }

    transaction.commit();
    return 0;
  } catch (const std::exception& e) {
    LOG_FATAL(kea_hook_logger, KEA_HOOK_FAILED).arg(e.what());

    return 1;
  }
}

/* Handle lease after one allocation
 * State after DHCPDISCOVER, DHCPREQUEST(renew)
 */
int lease4_renew(isc::hooks::CalloutHandle& handle) {
  isc::dhcp::Pkt4Ptr query4Ptr;
  handle.getArgument("query4", query4Ptr);

  const auto& isRequest = query4Ptr->getType() == isc::dhcp::DHCPREQUEST;
  std::string tmp{(isRequest) ? "---[REQUEST" : "---[DISCOVER"};

  LOG_DEBUG(kea_hook_logger, 0, KEA_HOOK_DHCP_STATE).arg(tmp + "(RENEW)]---");

  // Used for DHCPREQUEST check in lease4_select
  handle.setArgument("fake_allocation", !isRequest);

  if (lease4_select(handle) != 0) {
    return 1;
  }

  return 0;
}

// Handle IP conflict
int lease4_decline(isc::hooks::CalloutHandle& handle) {
  isc::dhcp::Pkt4Ptr query4Ptr;
  handle.getArgument("query4", query4Ptr);

  isc::dhcp::Lease4Ptr lease4Ptr;
  handle.getArgument("lease4", lease4Ptr);

  const auto& mac_address{query4Ptr->getHWAddr()->toText(false)};
  LOG_FATAL(kea_hook_logger, KEA_HOOK_IP_CONFLICT)
      .arg(lease4Ptr->addr_.toText())
      .arg(mac_address);

  // set IP conflict to true
  const char* values[] = {mac_address.c_str()};
  isc::db::PgSqlResult r(PQexecPrepared(*g_pg_sql_connection, "set_ip_conflict",
                                        1, values, nullptr, nullptr, 0));

  return 0;
}
}