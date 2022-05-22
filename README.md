# isc-kea-hook

Hook library for ISC Kea(https://kea.isc.org/), the library controls MUEB IP assignment.

## Prerequisites

- Linux(**[Ubuntu 20.04](https://ubuntu.com/download/server)**, Debian 10), tested on ubuntu 20.04
- [ISC Kea](https://www.isc.org/download/#Kea) **2.0+**
- [PostgreSQL](https://www.postgresql.org/download/) **14+**
- [CMake](https://cmake.org/download/) **3.16+**
- **C++ 20** compatible compiler

## How to build

### Install PostgreSQL packages from official repository

Follow the instructions here to setup the environment:

https://www.postgresql.org/download/

- postgresql-14
- libpq-dev
- postgresql-server-dev-14

### Install isc kea packages from official repository

Follow the instructions here to setup the environment:

https://cloudsmith.io/~isc/repos/kea-2-0/setup/#formats-deb

After that install the following packages:

- isc-kea-dhcp4-server
- isc-kea-dev

### Install build tools

- [CMake](https://cmake.org/download/)

- build-essential or compatible c++ 20 compiler

After the installations build the project in release mode with cmake.

## How to run

Install the project with cmake, after that:

```bash
sudo kea-dhcp4 -c /etc/kea/kea-dhcp4.conf
```

## How to generate log messages

Use kea-msg-compiler

https://reports.kea.isc.org/dev_guide/d8/d33/logKeaLogging.html#logMessageCompiler

```bash
kea-msg-compiler messages.mes
```

