#pragma once

#include <pgsql/pgsql_connection.h>

#include <map>
#include <string>

extern std::map<std::string, std::string> g_switch_data;
extern isc::db::PgSqlConnection* g_pg_sql_connection;