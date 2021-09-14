#include <hooks/hooks.h>
#include <pgsql/pgsql_connection.h>

#include <array>

#include "globals.h"
#include "logger.h"

isc::db::PgSqlConnection* g_pg_sql_connection;

extern "C" {
int version() { return KEA_HOOKS_VERSION; }

int load(isc::hooks::LibraryHandle& handle) {
  auto host_ptr{handle.getParameter("host")};
  auto port_ptr{handle.getParameter("port")};
  auto database_ptr{handle.getParameter("database")};
  auto username_ptr{handle.getParameter("username")};
  auto password_ptr{handle.getParameter("password")};

  if (!host_ptr || !port_ptr || !database_ptr || !username_ptr ||
      !password_ptr) {
    LOG_FATAL(kea_hook_logger, KEA_HOOK_MISSING_PARAMETERS);

    return 1;
  }

  g_pg_sql_connection = new isc::db::PgSqlConnection({
      {"host", host_ptr->stringValue()},
      {"port", port_ptr->stringValue()},
      {"name", database_ptr->stringValue()},
      {"user", username_ptr->stringValue()},
      {"password", password_ptr->stringValue()},
  });

  try {
    g_pg_sql_connection->openDatabase();

    // Store prepared statements
    std::array<isc::db::PgSqlTaggedStatement, 9> statements{
        {{1,
          {isc::db::OID_TEXT},
          "check_switch",
          "select count(*) from switch where switch_id = $1"},
         {2,
          {isc::db::OID_TEXT, isc::db::OID_TEXT},
          "mueb_in_room",
          "select room_id from port p join room r using(room_id) where "
          "p.port_id "
          "= $1 and p.switch_id = $2"},
         {2,
          {829, isc::db::OID_INT4},
          "mueb_count_in_room",
          "select count(*) from port p join mueb m using(port_id, switch_id) "
          "where m.mac_address != $1 and p.room_id = $2"},
         {1,
          {829},
          "ip_conflict",
          "select ip_conflict::int from mueb where mac_address = $1"},
         {1,
          {829},
          "ip_override",
          "select ip_override from mueb where mac_address = $1 and "
          "ip_override "
          "is not null"},
         {2,
          {isc::db::OID_TEXT, isc::db::OID_TEXT},
          "ip_address",
          "select ip_address from port p join room r using(room_id) where "
          "p.port_id "
          "= $1 and switch_id = $2"},
         {2,
          {isc::db::OID_TEXT, isc::db::OID_TEXT},
          "clear_port",
          "update mueb set port_id = null, switch_id = null where port_id = $1 "
          "and switch_id = $2"},
         {3,
          {829, isc::db::OID_TEXT, isc::db::OID_TEXT},
          "insert_or_update_mueb",
          "insert into mueb (mac_address) values ($1) on conflict "
          "(mac_address) do update set "
          "port_id = $2, switch_id = $3 where mueb.mac_address = "
          "$1"},
         {1,
          {829},
          "set_ip_conflict",
          "update mueb set ip_conflict = true where mac_address = $1"}}};
    g_pg_sql_connection->prepareStatements(statements.cbegin(),
                                           statements.cend());

  } catch (const std::exception& e) {
    LOG_FATAL(kea_hook_logger, KEA_HOOK_DATABASE_FAILED).arg(e.what());
    return 1;
  }

  LOG_DEBUG(kea_hook_logger, 0, KEA_HOOK_OPEN_DATABASE);

  return 0;
}

int multi_threading_compatible() { return 0; }
}