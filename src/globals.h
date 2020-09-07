#pragma once

#include <QSqlDatabase>
#include <unordered_map>
#include <string>

extern QSqlDatabase g_db;
extern std::unordered_map<std::string, uint> g_switchData;