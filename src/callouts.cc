#include <asiolink/io_address.h>
#include <dhcp/pkt4.h>
#include <dhcpsrv/lease.h>
#include <dhcpsrv/lease_mgr.h>
#include <dhcpsrv/lease_mgr_factory.h>
#include <hooks/hooks.h>
#include <pgsql/pgsql_connection.h>

#include <cstdlib>
#include <string>

#include "globals.h"
#include "logger.h"

extern "C" {
// Check IP conflict
int pkt4_receive(isc::hooks::CalloutHandle &handle) {
  isc::dhcp::Pkt4Ptr query4_ptr;
  handle.getArgument("query4", query4_ptr);

  const std::string &mac_address{query4_ptr->getHWAddr()->toText(false)};
  const char *values[]{mac_address.c_str()};
  isc::db::PgSqlResult r(PQexecPrepared(*g_pg_sql_connection, "ip_conflict", 1,
                                        values, nullptr, nullptr, 0));
  /* Drop packet
   * MUEB with IP conflict needs manual fix
   */
  if (r.getRows() > 0 && atoi(PQgetvalue(r, 0, 0)) > 0) {
    handle.setStatus(isc::hooks::CalloutHandle::NEXT_STEP_DROP);
  }

  return 0;
}

// handle IP lease
int lease4_select(isc::hooks::CalloutHandle &handle) {
  isc::dhcp::Pkt4Ptr query4_ptr;
  handle.getArgument("query4", query4_ptr);
  isc::dhcp::Lease4Ptr lease4_ptr;
  handle.getArgument("lease4", lease4_ptr);
  bool fake_allocation;
  handle.getArgument("fake_allocation", fake_allocation);

  LOG_DEBUG(schmatrix_logger, 0, SCHMATRIX_DHCP_STATE)
      .arg((fake_allocation) ? "---[DISCOVER]---" : "---[REQUEST]---");

  auto hwaddr_ptr = query4_ptr->getHWAddr();
  auto mac_address = hwaddr_ptr->toText(false);

  // Allocate IP for non MUEB devices
  if (hwaddr_ptr->hwaddr_[0] != 0x54 || hwaddr_ptr->hwaddr_[1] != 0x10 ||
      hwaddr_ptr->hwaddr_[2] != 0xEC) {
    LOG_INFO(schmatrix_logger, SCHMATRIX_NOT_MUEB).arg(mac_address);
    LOG_DEBUG(schmatrix_logger, 0, SCHMATRIX_DHCP_STATE)
        .arg((fake_allocation) ? "---<DISCOVER>---" : "---<REQUEST>---");

    return 0;
  }

  // Critical part begin, it's not safe to lease IP yet
  handle.setStatus(isc::hooks::CalloutHandle::NEXT_STEP_SKIP);

  // Get switch hostname and port name from DHCP option 82
  // https://www.cisco.com/c/en/us/td/docs/switches/lan/catalyst4500/12-2/15-02SG/configuration/guide/config/dhcp.html#57094
  auto option82{query4_ptr->getOption(82)};
  auto circuit_id{option82->getOption(1)->getData()};
  auto remote_id{option82->getOption(2)->getData()};

  // Check suboption ID types
  if (circuit_id[0] != 0 || remote_id[0] != 1) {
    LOG_FATAL(schmatrix_logger, SCHMATRIX_DHCP_OPTION_82_ERROR);
    return 1;
  }

  std::string port_id{std::to_string(circuit_id[4]) + "/" +
                      std::to_string(circuit_id[5])};
  std::string switch_name{remote_id.begin() + 2, remote_id.end()};
  auto switch_id{g_switch_data.at(switch_name)};

  // Check for IP override
  const char *values[3] = {mac_address.c_str()};
  isc::db::PgSqlResult r(PQexecPrepared(*g_pg_sql_connection, "ip_override", 1,
                                        values, nullptr, nullptr, 0));
  bool ip_overriden{r.getRows() > 0};

  // Query DB to get IP address
  std::string ip_address;
  if (!ip_overriden) {
    values[0] = port_id.c_str();
    values[1] = switch_id.c_str();
    isc::db::PgSqlResult r2(PQexecPrepared(*g_pg_sql_connection, "ip_address",
                                           2, values, nullptr, nullptr, 0));

    // Handle incorrect room
    if (r2.getRows() < 0) {
      LOG_ERROR(schmatrix_logger, SCHMATRIX_UNKNOWN_ROOM)
          .arg(mac_address)
          .arg(switch_name)
          .arg(port_id);

      return 1;
    }

    ip_address = PQgetvalue(r2, 0, 0);
  } else {
    ip_address = PQgetvalue(r, 0, 0);
    LOG_INFO(schmatrix_logger, SCHMATRIX_IP_OVERRIDDEN)
        .arg(mac_address)
        .arg(ip_address);
  }

  // Modify kea's IP lease
  lease4_ptr->addr_ = isc::asiolink::IOAddress(ip_address);
  handle.setArgument("lease4", lease4_ptr);

  // Basic DHCP functionality ends here
  // Critical part end, it's safe to lease IP now
  handle.setStatus(isc::hooks::CalloutHandle::NEXT_STEP_CONTINUE);

  LOG_DEBUG(schmatrix_logger, 0, SCHMATRIX_QUERIED_IP).arg(ip_address);

  LOG_DEBUG(schmatrix_logger, 0, SCHMATRIX_DHCP_STATE)
      .arg((fake_allocation) ? "---<DISCOVER>---" : "---<REQUEST>---");

  // Save to DB when the DHCP state is DHCPREQUEST For debugging purposes
  if (!fake_allocation) {
    // Clear switch-port
    values[0] = port_id.c_str();
    values[1] = switch_id.c_str();
    isc::db::PgSqlResult r3(PQexecPrepared(*g_pg_sql_connection, "clear_port",
                                           2, values, nullptr, nullptr, 0));

    // Insert MUEB or update switch-port
    values[0] = mac_address.c_str();
    values[1] = port_id.c_str();
    values[2] = switch_id.c_str();
    isc::db::PgSqlResult r4(PQexecPrepared(*g_pg_sql_connection, "insert_mueb",
                                           3, values, nullptr, nullptr, 0));
  } else {
    /* Handle device swap
     * Remove the current IP from the lease DB before DHCPREQUEST
     * This step is required to make the device swap work otherwise the server
     * sends DHCPNAK
     */
    if (isc::dhcp::LeaseMgrFactory::haveInstance()) {
      isc::dhcp::LeaseMgrFactory::instance().deleteLease(lease4_ptr);
    }
  }

  return 0;
}

/* Handle lease after one allocation
 * State after DHCPDISCOVER, DHCPREQUEST(renew)
 */
int lease4_renew(isc::hooks::CalloutHandle &handle) {
  isc::dhcp::Pkt4Ptr query4Ptr;
  handle.getArgument("query4", query4Ptr);
  auto isRequest = query4Ptr->getType() == isc::dhcp::DHCPREQUEST;

  std::string tmp = (isRequest) ? "---[REQUEST" : "---[DISCOVER";

  LOG_DEBUG(schmatrix_logger, 0, SCHMATRIX_DHCP_STATE).arg(tmp + "(RENEW)]---");

  // Used for DHCPREQUEST check in lease4_select
  handle.setArgument("fake_allocation", !(isRequest));

  if (lease4_select(handle) != 0) {
    return 1;
  }

  return 0;
}

// Handle IP conflict
int lease4_decline(isc::hooks::CalloutHandle &handle) {
  isc::dhcp::Pkt4Ptr query4Ptr;
  handle.getArgument("query4", query4Ptr);
  isc::dhcp::Lease4Ptr lease4Ptr;
  handle.getArgument("lease4", lease4Ptr);

  const std::string &mac_address = query4Ptr->getHWAddr()->toText(false);
  LOG_FATAL(schmatrix_logger, SCHMATRIX_IP_CONFLICT)
      .arg(lease4Ptr->addr_.toText())
      .arg(mac_address);

  // set IP conflict to true
  const char *values[] = {mac_address.c_str()};
  isc::db::PgSqlResult r(PQexecPrepared(*g_pg_sql_connection, "set_ip_conflict",
                                        2, values, nullptr, nullptr, 0));

  return 0;
}
}
