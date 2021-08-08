#include <hooks/hooks.h>
#include <pgsql/pgsql_connection.h>

#include <array>

#include "globals.h"
#include "logger.h"

std::map<std::string, std::string> g_switch_data;
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
    LOG_FATAL(schmatrix_logger, SCHMATRIX_MISSING_PARAMETERS);

    return 1;
  }

  const isc::db::DatabaseConnection::ParameterMap parameterMap{
      {"host", host_ptr->stringValue()},
      {"port", port_ptr->stringValue()},
      {"name", database_ptr->stringValue()},
      {"user", username_ptr->stringValue()},
      {"password", password_ptr->stringValue()},
  };
  g_pg_sql_connection = new isc::db::PgSqlConnection(parameterMap);
  try {
    g_pg_sql_connection->openDatabase();
  } catch (const std::exception& e) {
    LOG_FATAL(schmatrix_logger, SCHMATRIX_DATABASE_FAILED).arg(e.what());
    return 1;
  }

  // Store prepared statements
  std::array<isc::db::PgSqlTaggedStatement, 10> statements{
      {{1,
        {isc::db::OID_TEXT},
        "ip_conflict",
        "select ip_conflict::int from mueb where mac_address = $1"},
       {1,
        {isc::db::OID_TEXT},
        "ip_override",
        "select ip_override from mueb where mac_address = $1 and ip_override "
        "is not null"},
       {2,
        {isc::db::OID_TEXT, isc::db::OID_TEXT},
        "ip_address",
        "select ip_address from port join room r using(room_id) where port_id "
        "= $1 and switch_id = $2"},
       {2,
        {isc::db::OID_TEXT, isc::db::OID_TEXT},
        "clear_port",
        "update mueb set port_id = null, switch_id = null where port_id = $1 "
        "and switch_id = $2"},
       {3,
        {isc::db::OID_TEXT, isc::db::OID_TEXT, isc::db::OID_TEXT},
        "insert_mueb",
        "insert into mueb (mac_address) values ($1) on conflict do update set "
        "port_id = $2, switch_id = $3 where mac_address = $1"},
       {1,
        {isc::db::OID_TEXT},
        "set_ip_conflict",
        "update mueb set ip_conflict = true where mac_address = $1"}}};
  g_pg_sql_connection->prepareStatements(statements.cbegin(),
                                         statements.cend());

  // Cache switch data
  isc::db::PgSqlResult r(
      PQexec(*g_pg_sql_connection, "select name, switch_id from switch"));
  if (r.getRows() <= 0) {
    LOG_FATAL(schmatrix_logger, SCHMATRIX_DATABASE_FAILED)
        .arg("switch table is empty!");

    return 1;
  }

  for (int i = 0; i < r.getRows(); ++i) {
    g_switch_data.emplace(PQgetvalue(r, i, 0), PQgetvalue(r, i, 1));
  }

  LOG_DEBUG(schmatrix_logger, 0, SCHMATRIX_OPEN_DATABASE);

  return 0;
}

int multi_threading_compatible() { return 0; }
}