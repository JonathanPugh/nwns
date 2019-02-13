nwns
=====

Nwns is an Ncurses Wireless Network Scanner which displays nearby wireless network SSIDs, MAC addresses, and channels. It utilizes libnl, a collection of libraries providing APIs to the Netlink protocol based Linux kernel interfaces. Because Netlink is a Linux-specific protocol, nwns is only compatible with Linux operating systems. Nwns is tested on Arch Linux


Getting Started
===============

Nwns can be compiled with the included Makefile:

```
    make
```

Compile and install to /usr/sbin/:

```
    sudo make install
```

Uninstall:

```
    sudo make remove
```

Usage
=====

Nwns must be run as root.

Pass your wireless interface as the first parameter when runnings nwns:

```
    sudo nwns wlan0
```

If no interface is specified, nwns will attempt to use wlp3s0 as the interface.

Example
=======

Below is an example of a completed scan with nwns:

![Alt text](example.png?raw=true "nwns example scan")

License
=======

Nwns is licensed under the GNU LGPLv2.1 - See [LICENSE](LICENSE) for details.
