// File created from messages.mes on Mon Sep 07 2020 14:03

#include <cstddef>
#include <log/message_types.h>
#include <log/message_initializer.h>

extern const isc::log::MessageID SCHMATRIX_DATABASE_FAILED = "SCHMATRIX_DATABASE_FAILED";
extern const isc::log::MessageID SCHMATRIX_DHCP_OPTION_82_ERROR = "SCHMATRIX_DHCP_OPTION_82_ERROR";
extern const isc::log::MessageID SCHMATRIX_DHCP_STATE = "SCHMATRIX_DHCP_STATE";
extern const isc::log::MessageID SCHMATRIX_IP_CONFLICT = "SCHMATRIX_IP_CONFLICT";
extern const isc::log::MessageID SCHMATRIX_IP_OVERRIDDEN = "SCHMATRIX_IP_OVERRIDDEN";
extern const isc::log::MessageID SCHMATRIX_MISSING_PARAMETERS = "SCHMATRIX_MISSING_PARAMETERS";
extern const isc::log::MessageID SCHMATRIX_MUEB_NOT_FOUND = "SCHMATRIX_MUEB_NOT_FOUND";
extern const isc::log::MessageID SCHMATRIX_NOT_MUEB = "SCHMATRIX_NOT_MUEB";
extern const isc::log::MessageID SCHMATRIX_OPEN_DATABASE = "SCHMATRIX_OPEN_DATABASE";
extern const isc::log::MessageID SCHMATRIX_PORT_FOUND = "SCHMATRIX_PORT_FOUND";
extern const isc::log::MessageID SCHMATRIX_QUERIED_IP = "SCHMATRIX_QUERIED_IP";
extern const isc::log::MessageID SCHMATRIX_SQL_DONE = "SCHMATRIX_SQL_DONE";
extern const isc::log::MessageID SCHMATRIX_SQL_FAILED = "SCHMATRIX_SQL_FAILED";
extern const isc::log::MessageID SCHMATRIX_UNKNOWN_ROOM = "SCHMATRIX_UNKNOWN_ROOM";

namespace {

const char* values[] = {
    "SCHMATRIX_DATABASE_FAILED", "%1",
    "SCHMATRIX_DHCP_OPTION_82_ERROR", "Invalid DHCP option 82",
    "SCHMATRIX_DHCP_STATE", "%1",
    "SCHMATRIX_IP_CONFLICT", "Could not lease IP: %1 to MUEB: %2 because of IP conflict",
    "SCHMATRIX_IP_OVERRIDDEN", "MUEB: %1 IP's overridden to: %2",
    "SCHMATRIX_MISSING_PARAMETERS", "Not all parameters are provided",
    "SCHMATRIX_MUEB_NOT_FOUND", "MUEB: %1 not found on any switch",
    "SCHMATRIX_NOT_MUEB", "Device with MAC: %1 is not a MUEB",
    "SCHMATRIX_OPEN_DATABASE", "Opened database successfully",
    "SCHMATRIX_PORT_FOUND", "MUEB: %1 is connected to switch: %2 port: %3",
    "SCHMATRIX_QUERIED_IP", "Queried IP is: %1",
    "SCHMATRIX_SQL_DONE", "At line: %1",
    "SCHMATRIX_SQL_FAILED", "SQL error: %1",
    "SCHMATRIX_UNKNOWN_ROOM", "MUEB: %1 connected to a bad room, sw: %2, port: %3",
    NULL
};

const isc::log::MessageInitializer initializer(values);

} // Anonymous namespace

