phoneline - system for managing annoying ringing phones
=======================================================

About
-----
The phoneline project is a system for monitoring a home telephone line, and
remotely managing annoying ringing phones.

The system is made out of the following parts:

 - Some kind of, preferably low power like the raspberry pi, computer running
   UNIX, connected to the home LAN, and also to the telephone line through a
   standard serial port modem.

 - A UNIX daemon (*phonelined*), running on said computer, monitors the phone
   line for ringing events. When a phonecall arrives, it notifies any active
   clients, provides caller-id information, and can hang-up the phone on demand.

 - Command-line and X11 clients (*phoneline* and *xphoneline*) able to connect
   to the aforementioned daemon, wait for calls and notify the user, retreive
   call logs from the daemon, or instruct it to hang up the phone when
   undesirable people are calling.

See the README files of each program for details.

License
-------
Copyright (C) 2019 John Tsiombikas <nuclear@member.fsf.org>

All programs in this project are free software. Feel free to use, modify and/or
redistribute them under the terms of the GNU General Public License version 3,
or at your option any later version published by the Free Software Foundation.
See the COPYING files of each program for details.
