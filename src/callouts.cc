#include <dhcp/pkt4.h>
#include <dhcpsrv/lease.h>
#include <dhcpsrv/lease_mgr.h>
#include <dhcpsrv/lease_mgr_factory.h>
#include <hooks/hooks.h>

#include <QSqlQuery>
#include <QVariant>
#include <QtEndian>
#include <SNMPpp/SNMPpp.hpp>
#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

#include "globals.h"
#include "logger.h"

using namespace isc::dhcp;
using namespace isc::hooks;
using namespace isc::asiolink;

// Kea HWAddr to 64 bit/bigint magic
// Implementation may change, current solution could be unsafe
static quint64 eightBitsTo64bit(const std::vector<uint8_t> &vector) {
  union {
    quint64 hwaddr64;
    quint8 hwaddr8[sizeof(quint64)];
  } bigint;

  bigint = {0};
  std::copy(vector.cbegin(), vector.cend(),
            bigint.hwaddr8 + (sizeof(quint64) - HWAddr::ETHERNET_HWADDR_LEN));

  return qToBigEndian(bigint.hwaddr64);
}

extern "C" {
// Check IP conflict
int pkt4_receive(CalloutHandle &handle) {
  Pkt4Ptr query4Ptr;

  handle.getArgument("query4", query4Ptr);
  auto hwaddrPtr{query4Ptr->getHWAddr()};

  QSqlQuery query;
  query.prepare(
      "SELECT ip_conflict FROM mueb JOIN mueb_details USING (mueb_id) WHERE "
      "mac_address = ? AND ip_conflict IS NOT NULL");
  query.addBindValue(eightBitsTo64bit(hwaddrPtr->hwaddr_));
  query.exec();

  /* Drop packet
  MUEB with IP conflict needs manual fix
  */
  if (query.first() && query.value(0).toBool())
    handle.setStatus(CalloutHandle::NEXT_STEP_DROP);

  return 0;
}

// handle IP lease
int lease4_select(CalloutHandle &handle) {
  Pkt4Ptr query4Ptr;
  Lease4Ptr lease4Ptr;
  bool fakeAllocation;

  handle.getArgument("query4", query4Ptr);
  handle.getArgument("lease4", lease4Ptr);
  handle.getArgument("fake_allocation", fakeAllocation);

  LOG_DEBUG(schmatrix_logger, 0, SCHMATRIX_DHCP_STATE)
      .arg((fakeAllocation) ? "---[DISCOVER]---" : "---[REQUEST]---");

  auto hwaddrPtr = query4Ptr->getHWAddr();
  auto hwaddr = hwaddrPtr->toText(false);

  // Allocate IP for non MUEB devices
  if (hwaddrPtr->hwaddr_[0] != 0x54 || hwaddrPtr->hwaddr_[1] != 0x10 ||
      hwaddrPtr->hwaddr_[2] != 0xec) {
    LOG_INFO(schmatrix_logger, SCHMATRIX_NOT_MUEB).arg(hwaddr);
    LOG_DEBUG(schmatrix_logger, 0, SCHMATRIX_DHCP_STATE)
        .arg((fakeAllocation) ? "---<DISCOVER>---" : "---<REQUEST>---");

    return 0;
  }

  // Critical part begin, it's not safe to lease IP
  handle.setStatus(CalloutHandle::NEXT_STEP_SKIP);

  // create SNMP session
  SNMPpp::SessionHandle sessionHandle = nullptr;

  // MAC address in decimal format
  std::stringstream ss;
  for (size_t i = 0; i < HWAddr::ETHERNET_HWADDR_LEN; i++) {
    ss << '.' << (int)hwaddrPtr->hwaddr_[i];
  }

  // iterate through the switches
  quint32 switchId{0};
  std::string switchIp;
  std::string portName;
  for (auto &switchData : g_switchData) {
    switchIp = std::get<1>(switchData);

    try {
      SNMPpp::openSession(sessionHandle, "udp:" + switchIp + ":161",
                          g_snmpCommunity);

      // 1. Get bridge port number || INTEGER
      SNMPpp::OID o = "1.3.6.1.2.1.17.4.3.1.2" + ss.str();
      SNMPpp::PDU pdu = SNMPpp::get(sessionHandle, o);
      auto bridgeportNumber = pdu.varlist().getLong(o);

      // always remember to free the PDU once done with it
      pdu.free();

      // 2. Get ifIndex || INTEGER
      o = "1.3.6.1.2.1.17.1.4.1.2." + std::to_string(bridgeportNumber);
      pdu = SNMPpp::get(sessionHandle, o);
      auto ifIndex = pdu.varlist().getLong(o);
      pdu.free();

      // 3. Get ifName || STRING
      o = "1.3.6.1.2.1.31.1.1.1.1." + std::to_string(ifIndex);
      pdu = SNMPpp::get(sessionHandle, o);
      portName = pdu.varlist().getString(o);
      pdu.free();

      // If the port name is correct we're done
      if (!portName.empty() && portName.substr(0, 2) == "Gi") {
        switchId = std::get<0>(switchData);

        LOG_DEBUG(schmatrix_logger, 0, SCHMATRIX_PORT_FOUND)
            .arg(hwaddr)
            .arg(switchIp)
            .arg(portName);

        SNMPpp::closeSession(
            sessionHandle);  // Don't forget to close the session
        break;
      } else {
        // Log incorrect port name e.g. Po1
        LOG_DEBUG(schmatrix_logger, 0, SCHMATRIX_BAD_PORT)
            .arg(portName)
            .arg(switchIp);
      }
    }
    // Fatal error no sense to continue
    // SNMPpp::openSession throws this
    catch (const std::runtime_error &e) {
      LOG_FATAL(schmatrix_logger, SCHMATRIX_SNMP_EXCEPT)
          .arg(switchIp)
          .arg(e.what());
      SNMPpp::closeSession(sessionHandle);

      return 1;
    }
    // Not a big issue, e.g. PDU is empty => MUEB not connected to the switch
    catch (const std::exception &e) {
      LOG_DEBUG(schmatrix_logger, 0, SCHMATRIX_SNMP_ERROR).arg(e.what());
    }

    // Close session before the next iteration
    SNMPpp::closeSession(sessionHandle);
  }

  // Handle incorrect port name after all switch
  if (portName.empty() || portName.substr(0, 2) != "Gi") {
    LOG_ERROR(schmatrix_logger, SCHMATRIX_MUEB_NOT_FOUND).arg(hwaddr);

    return 1;
  }

  QSqlQuery query;

  // Get MUEB id
  auto macAddress = eightBitsTo64bit(hwaddrPtr->hwaddr_);
  bool isIpOverriden;

  query.prepare("SELECT mueb_id FROM mueb WHERE mac_address = ?");
  query.addBindValue(macAddress);
  query.exec();

  if (query.first()) {
    auto muebId = query.value(0).toUInt();

    // Check for IP override
    query.prepare(
        "SELECT ip_override FROM mueb_details WHERE mueb_id = ? AND "
        "ip_override IS NOT NULL");
    query.addBindValue(muebId);
    query.exec();

    isIpOverriden = (query.size() == 1) ? true : false;
  }

  if (!isIpOverriden) {
    // Query DB to get IP address
    query.prepare(
        "SELECT ip_address FROM port JOIN room USING(room_id) "
        "WHERE port_name = :port_name AND switch_id = "
        ":switch_id");
    query.bindValue(":port_name", QString::fromStdString(portName));
    query.bindValue(":switch_id", switchId);
    query.exec();

    // Handle incorrect room
    if (query.first() == false) {
      LOG_ERROR(schmatrix_logger, SCHMATRIX_UNKNOWN_ROOM)
          .arg(hwaddr)
          .arg(switchIp)
          .arg(portName);

      return 1;
    }
  }

  // Modify kea's IP lease
  lease4Ptr->addr_ = IOAddress(query.value(0).toUInt());
  handle.setArgument("lease4", lease4Ptr);

  if (isIpOverriden)
    LOG_INFO(schmatrix_logger, SCHMATRIX_IP_OVERRIDDEN)
        .arg(hwaddr)
        .arg(lease4Ptr->addr_.toText());

  // Basic DHCP functionality ends here
  // Critical part end, it's safe to lease IP now
  handle.setStatus(CalloutHandle::NEXT_STEP_CONTINUE);

  LOG_DEBUG(schmatrix_logger, 0, SCHMATRIX_QUERIED_IP)
      .arg(lease4Ptr->addr_.toText());

  LOG_DEBUG(schmatrix_logger, 0, SCHMATRIX_DHCP_STATE)
      .arg((fakeAllocation) ? "---<DISCOVER>---" : "---<REQUEST>---");

  // save to DB when the DHCP state is DHCPREQUEST
  // For debugging purposes used for matrix-dhcp-debug
  if (fakeAllocation == false) {
    // Create MUEB
    query.prepare("INSERT IGNORE INTO mueb(mac_address) VALUES(?)");
    query.addBindValue(macAddress);
    query.exec();

    // Handle port swap
    query.prepare("CALL update_port(:mac_address, :port_name, :switch_id)");
    query.bindValue(":mac_address", macAddress);
    query.bindValue(":port_name", QString::fromStdString(portName));
    query.bindValue(":switch_id", switchId);
    query.exec();
  } else {
    /* Handle device swap
    Remove the current IP from the lease DB before DHCPREQUEST
    This step is required to make the device swap work otherwise the server
    sends DHCPNAK
    */
    if (LeaseMgrFactory::haveInstance())
      LeaseMgrFactory::instance().deleteLease(lease4Ptr->addr_);
  }

  return 0;
}

/* Handle lease after one allocation
State after DHCPDISCOVER, DHCPREQUEST(renew)
*/
int lease4_renew(CalloutHandle &handle) {
  Pkt4Ptr query4Ptr;
  handle.getArgument("query4", query4Ptr);
  auto isRequest = query4Ptr->getType() == DHCPREQUEST;

  std::string tmp = (isRequest) ? "---[REQUEST" : "---[DISCOVER";

  LOG_DEBUG(schmatrix_logger, 0, SCHMATRIX_DHCP_STATE).arg(tmp + "(RENEW)]---");

  // Used for DHCPREQUEST check in lease4_select
  handle.setArgument("fake_allocation", (isRequest) ? false : true);

  lease4_select(handle);

  return 0;
}

// Handle IP conflict
int lease4_decline(CalloutHandle &handle) {
  Pkt4Ptr query4Ptr;
  Lease4Ptr lease4Ptr;

  handle.getArgument("query4", query4Ptr);
  handle.getArgument("lease4", lease4Ptr);

  auto ip = lease4Ptr->addr_.toText();

  LOG_FATAL(schmatrix_logger, SCHMATRIX_IP_CONFLICT)
      .arg(ip)
      .arg(query4Ptr->getHWAddr()->toText(false));

  // set IP conflict to true
  auto macAddress = eightBitsTo64bit(query4Ptr->getHWAddr()->hwaddr_);
  QSqlQuery query;
  query.prepare("CALL set_ip_conflict(?, TRUE)");
  query.addBindValue(macAddress);
  query.exec();

  return 0;
}
}