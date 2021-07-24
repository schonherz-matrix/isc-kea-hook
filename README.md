# kea-hook

Hook library for ISC Kea(https://kea.isc.org/), the library controls MUEB IP assignment.

## Prerequisites

- Linux(**Ubuntu 20.04**, Debian 10)
- ISC Kea **1.6** or above
- MySQL **8.0.11** or above
- CMake **3.16** or above
- **SNMPpp**
- **C++ 20** compatible compiler

## Install kea from repository

Use stable packages from repository

```bash
sudo apt install -y kea-dhcp4-server kea-admin kea-dev
```

## How to build

### Install dependencies

```bash
sudo apt install -y build-essential qt5-default \
libqt5sql5-mysql libboost-dev libsnmp-dev git cmake mysql-server
cd /tmp
git clone --depth=1 https://github.com/choopm/snmppp.git
cd snmppp && cmake -B build && cmake --build build -j 4
sudo cmake --install build
```
### Matrix-kea-hook library

```bash
cd /tmp
git clone --depth=1 https://git.sch.bme.hu/matrix-group/dhcp/matrix-kea-hook.git
cd matrix-kea-hook && cmake -B build && cmake --build build -j 4
sudo cmake --install build
sudo ldconfig
sudo mkdir /var/run/kea/
```

## Kea MySQL database creation

https://kea.readthedocs.io/en/kea-1.6.2/arm/admin.html#mysql-database-create

## How to run

```bash
sudo kea-dhcp4 -c /etc/kea/matrix-kea-hook.conf
```

Or with keactrl

```bash
sudo keactrl start -c /etc/kea/matrix-keactrl.conf
```

## Optional how to build kea

https://kb.isc.org/docs/installing-kea

https://kea.readthedocs.io/en/kea-1.6.2/arm/install.html

```bash
./configure --with-mysql --enable-generate-messages
make -j 4 && sudo make install
```

### How to use kea-msg-compiler

```bash
cd matrix-kea-hook/src
kea-msg-compiler messages.mes
```

