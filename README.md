nwns
=====

nwns is an Ncurses Wireless Network Scanner which displays nearby wireless network SSIDs, MAC addresses, and channels. It utilizes libnl, a collection of libraries providing APIs to the Netlink protocol based Linux kernel interfaces. Because Netlink is a Linux-specific protocol, nwns will only work in Linux operating systems. nwns is tested on Arch Linux.


Getting Started
===============

nwns can be compiled with the included Makefile:

.. code:: bash

    make

Compile and install to /usr/sbin/:

.. code:: bash

    sudo make install

Uninstall:

.. code:: bash

    sudo make remove

Usage
=====

nwns must be run as root

Pass your wireless interface as the first parameter when runnings nwns:

.. code:: bash

    sudo nwns wlan0

If no interface is specified, nwns will attempt to use wlp3s0 as the interface.

Example
=======

Below is an example of a completed scan with nwns:

![Alt text](example.png?raw=true "nwns example scan")


