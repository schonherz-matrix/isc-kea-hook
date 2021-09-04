// File created from messages.mes on Sat Sep 04 2021 16:53

#include <cstddef>
#include <log/message_types.h>
#include <log/message_initializer.h>

extern const isc::log::MessageID KEA_HOOK_DATABASE_FAILED = "KEA_HOOK_DATABASE_FAILED";
extern const isc::log::MessageID KEA_HOOK_DHCP_OPTION_82_ERROR = "KEA_HOOK_DHCP_OPTION_82_ERROR";
extern const isc::log::MessageID KEA_HOOK_DHCP_STATE = "KEA_HOOK_DHCP_STATE";
extern const isc::log::MessageID KEA_HOOK_FAILED = "KEA_HOOK_FAILED";
extern const isc::log::MessageID KEA_HOOK_IP_CONFLICT = "KEA_HOOK_IP_CONFLICT";
extern const isc::log::MessageID KEA_HOOK_IP_OVERRIDDEN = "KEA_HOOK_IP_OVERRIDDEN";
extern const isc::log::MessageID KEA_HOOK_MISSING_PARAMETERS = "KEA_HOOK_MISSING_PARAMETERS";
extern const isc::log::MessageID KEA_HOOK_MULTIPLE_MUEB = "KEA_HOOK_MULTIPLE_MUEB";
extern const isc::log::MessageID KEA_HOOK_NOT_MUEB = "KEA_HOOK_NOT_MUEB";
extern const isc::log::MessageID KEA_HOOK_OPEN_DATABASE = "KEA_HOOK_OPEN_DATABASE";
extern const isc::log::MessageID KEA_HOOK_QUERIED_IP = "KEA_HOOK_QUERIED_IP";
extern const isc::log::MessageID KEA_HOOK_UNKNOWN_ROOM = "KEA_HOOK_UNKNOWN_ROOM";

namespace {

const char* values[] = {
    "KEA_HOOK_DATABASE_FAILED", "%1",
    "KEA_HOOK_DHCP_OPTION_82_ERROR", "Invalid DHCP option 82",
    "KEA_HOOK_DHCP_STATE", "%1",
    "KEA_HOOK_FAILED", "%1",
    "KEA_HOOK_IP_CONFLICT", "Could not lease IP: %1 to MUEB: %2 because of IP conflict",
    "KEA_HOOK_IP_OVERRIDDEN", "MUEB: %1 IP's overridden to: %2",
    "KEA_HOOK_MISSING_PARAMETERS", "Not all parameters are provided",
    "KEA_HOOK_MULTIPLE_MUEB", "Multiple MUEBs in the same room %1",
    "KEA_HOOK_NOT_MUEB", "Device with MAC: %1 is not a MUEB",
    "KEA_HOOK_OPEN_DATABASE", "Opened database successfully",
    "KEA_HOOK_QUERIED_IP", "Queried IP is: %1",
    "KEA_HOOK_UNKNOWN_ROOM", "MUEB: %1 connected to a unknown room, switch: %2, port: %3",
    NULL
};

const isc::log::MessageInitializer initializer(values);

} // Anonymous namespace

