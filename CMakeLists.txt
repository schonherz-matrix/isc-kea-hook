cmake_minimum_required(VERSION 3.16)

project(
  matrix-kea-hook
  VERSION 2.0
  DESCRIPTION
    "Hook library for isc kea(https://kea.isc.org/), the library controls IP assignment."
  HOMEPAGE_URL "kszk.bme.hu"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(KEA_PREFIX
    /usr
    CACHE PATH "Define isc-kea installation location (the default is /usr)")

find_package(
  Qt5
  COMPONENTS Core Sql
  REQUIRED)

add_library(
  ${PROJECT_NAME} SHARED src/callouts.cc src/load_unload.cc src/logger.cc
                         src/messages.cc src/version.cc src/globals.h)

target_link_libraries(
  ${PROJECT_NAME}
  kea-dhcpsrv
  kea-dhcp++
  kea-hooks
  kea-log
  kea-util
  kea-exceptions
  kea-database
  snmppp
  Qt5::Core
  Qt5::Sql)

target_include_directories(${PROJECT_NAME} PRIVATE ${KEA_PREFIX}/include/kea)

install(TARGETS ${PROJECT_NAME}
        LIBRARY DESTINATION ${KEA_PREFIX}/lib/x86_64-linux-gnu/kea/hooks)
install(FILES config/matrix-kea-dhcp4.conf config/matrix-keactrl.conf
        DESTINATION /etc/kea/)
