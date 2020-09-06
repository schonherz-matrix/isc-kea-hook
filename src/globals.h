#pragma once

#include <QSqlDatabase>
#include <tuple>
#include <vector>
#include <string>

extern QSqlDatabase g_db;
extern std::vector<std::tuple<quint32, std::string>> g_switchData;
extern std::string g_snmpCommunity;