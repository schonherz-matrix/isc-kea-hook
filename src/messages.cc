// File created from messages.mes on Sun Aug 08 2021 19:54

#include <cstddef>
#include <log/message_types.h>
#include <log/message_initializer.h>

extern const isc::log::MessageID SCHMATRIX_DATABASE_FAILED = "SCHMATRIX_DATABASE_FAILED";
extern const isc::log::MessageID SCHMATRIX_DHCP_OPTION_82_ERROR = "SCHMATRIX_DHCP_OPTION_82_ERROR";
extern const isc::log::MessageID SCHMATRIX_DHCP_STATE = "SCHMATRIX_DHCP_STATE";
extern const isc::log::MessageID SCHMATRIX_IP_CONFLICT = "SCHMATRIX_IP_CONFLICT";
extern const isc::log::MessageID SCHMATRIX_IP_OVERRIDDEN = "SCHMATRIX_IP_OVERRIDDEN";
extern const isc::log::MessageID SCHMATRIX_MISSING_PARAMETERS = "SCHMATRIX_MISSING_PARAMETERS";
extern const isc::log::MessageID SCHMATRIX_NOT_MUEB = "SCHMATRIX_NOT_MUEB";
extern const isc::log::MessageID SCHMATRIX_OPEN_DATABASE = "SCHMATRIX_OPEN_DATABASE";
extern const isc::log::MessageID SCHMATRIX_QUERIED_IP = "SCHMATRIX_QUERIED_IP";
extern const isc::log::MessageID SCHMATRIX_UNKNOWN_ROOM = "SCHMATRIX_UNKNOWN_ROOM";

namespace {

const char* values[] = {
    "SCHMATRIX_DATABASE_FAILED", "%1",
    "SCHMATRIX_DHCP_OPTION_82_ERROR", "Invalid DHCP option 82",
    "SCHMATRIX_DHCP_STATE", "%1",
    "SCHMATRIX_IP_CONFLICT", "Could not lease IP: %1 to MUEB: %2 because of IP conflict",
    "SCHMATRIX_IP_OVERRIDDEN", "MUEB: %1 IP's overridden to: %2",
    "SCHMATRIX_MISSING_PARAMETERS", "Not all parameters are provided",
    "SCHMATRIX_NOT_MUEB", "Device with MAC: %1 is not a MUEB",
    "SCHMATRIX_OPEN_DATABASE", "Opened database successfully",
    "SCHMATRIX_QUERIED_IP", "Queried IP is: %1",
    "SCHMATRIX_UNKNOWN_ROOM", "MUEB: %1 connected to a unknown room, switch: %2, port: %3",
    NULL
};

const isc::log::MessageInitializer initializer(values);

} // Anonymous namespace

