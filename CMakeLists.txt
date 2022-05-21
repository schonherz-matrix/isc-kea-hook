cmake_minimum_required(VERSION 3.16)

project(
  isc-kea-hook
  VERSION 3.0.1
  DESCRIPTION
    "Hook library for isc kea(https://kea.isc.org/), the library controls IP assignment."
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(
  schmatrix-${PROJECT_NAME} SHARED
  src/callouts.cc src/logger.cc src/messages.cc src/globals.h
  src/framework_functions.cc)

target_link_libraries(
  schmatrix-${PROJECT_NAME}
  PRIVATE kea-dhcpsrv
          kea-dhcp++
          kea-hooks
          kea-log
          kea-util
          kea-exceptions
          kea-database
          pq)

target_include_directories(schmatrix-${PROJECT_NAME}
                           PRIVATE /usr/include/kea /usr/include/postgresql)

install(TARGETS schmatrix-${PROJECT_NAME}
        LIBRARY DESTINATION /usr/lib/x86_64-linux-gnu/kea/hooks)
install(FILES config/kea-dhcp4.conf DESTINATION /etc/kea/)
