cmake_minimum_required(VERSION 3.16)

project(
  kea-hook
  VERSION 3.0
  DESCRIPTION
    "Hook library for isc kea(https://kea.isc.org/), the library controls IP assignment."
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(
  ${PROJECT_NAME} SHARED src/callouts.cc src/logger.cc src/messages.cc
                         src/globals.h src/framework_functions.cc)

target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE kea-dhcpsrv
          kea-dhcp++
          kea-hooks
          kea-log
          kea-util
          kea-exceptions
          kea-database
          pq)

target_include_directories(${PROJECT_NAME} PRIVATE /usr/include/kea
                                                   /usr/include/postgresql)

install(TARGETS ${PROJECT_NAME}
        LIBRARY DESTINATION /usr/lib/x86_64-linux-gnu/kea/hooks)
install(FILES config/matrix-kea-dhcp4.conf config/matrix-keactrl.conf
        DESTINATION /etc/kea/)
