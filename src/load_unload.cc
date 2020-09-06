#include <hooks/hooks.h>

#include <QCoreApplication>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include "globals.h"
#include "logger.h"

using namespace isc::hooks;
using namespace isc::data;

static int argc{0};
static QCoreApplication a{argc, nullptr};
QSqlDatabase g_db;
std::vector<std::tuple<quint32, std::string>> g_switchData;
std::string g_snmpCommunity;

extern "C" {

int load(LibraryHandle& handle) {
  auto hostNamePtr = handle.getParameter("hostName");
  auto databaseNamePtr = handle.getParameter("databaseName");
  auto userNamePtr = handle.getParameter("userName");
  auto passwordPtr = handle.getParameter("password");
  auto snmpCommunity = handle.getParameter("snmpCommunity");

  if (!hostNamePtr || !databaseNamePtr || !userNamePtr || !passwordPtr ||
      !snmpCommunity) {
    LOG_FATAL(schmatrix_logger, SCHMATRIX_MISSING_PARAMETERS);

    return 1;
  }

  g_db = QSqlDatabase::addDatabase("QMYSQL");
  g_db.setHostName(QString::fromStdString(hostNamePtr->stringValue()));
  g_db.setDatabaseName(QString::fromStdString(databaseNamePtr->stringValue()));
  g_db.setUserName(QString::fromStdString(userNamePtr->stringValue()));
  g_db.setPassword(QString::fromStdString(passwordPtr->stringValue()));

  g_snmpCommunity = snmpCommunity->stringValue();

  if (!g_db.open()) {
    LOG_FATAL(schmatrix_logger, SCHMATRIX_DATABASE_FAILED)
        .arg(g_db.lastError().text().toStdString());

    return 1;
  }

  // Cache switch data
  QSqlQuery query;
  query.setForwardOnly(true);
  query.exec("SELECT switch_id, INET_NTOA(switch_ip) FROM switch");

  if (query.size() <= 0) {
    LOG_FATAL(schmatrix_logger, SCHMATRIX_DATABASE_FAILED)
        .arg("switch table is empty!");

    return 1;
  }

  // TODO add checks for other tables

  while (query.next()) {
    auto id = query.value(0).toUInt();
    auto ip = query.value(1).toString().toStdString();

    g_switchData.emplace_back(id, ip);
  }

  LOG_DEBUG(schmatrix_logger, 0, SCHMATRIX_OPEN_DATABASE);

  return 0;
}
}