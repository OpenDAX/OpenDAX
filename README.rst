***************************************************************
OpenDAX - An Open Source Data Acquisition and Control Framework
***************************************************************

----------------
Overview
----------------

OpenDAX is not a DCS, or a PLC or a SCADA system.  Rather it's a
framework that can be used to build all or parts of these things.  The idea
is to centralize and abstract real time data handling, logging and
messaging in a way that allows other developers to build the control
system that meets their needs without trying too hard to put a label on
it.

OpenDAX is made up of four primary parts.

 * Tag Server
 * libdax Client Library
 * Client Modules
 * Master Daemon

We call each unit of data in this system a "Tag".  The tag server is a
database of these tags.  Tags can be created, read and written to from
any of the client modules.

The client library abstracts the communication to the tag server for
the client modules.

The client modules are where the real work gets done.  These can communicate
to hardware I/O devices, communicate to other systems, log data or implement
control logic.  The modules communicate to the tag server through either a
local domain socket or a TCP/IP connection.  The server and the modules can
all run on the same machine or all on separate machines.  It is up to the
application developer to determine what works best for them.

The master program is used to start and stop the other parts of the system.
It is not required but has the ability to monitor processes and start / stop
them if they misbehave.  Since OpenDAX is meant to be distributed, client modules
can run on separate machines, the master program can be used on each of these
machines to start and manage these clients.

OpenDAX is meant to be flexible and scalable, it should utilize the
strength of the underlying operating system as much as possible.  The modern
Open Source operating system kernels are fantastic pieces of software that
do their jobs very well.  There is no sense in duplicating any of the
functions that they perform.

Modules should also be dynamic, able to be started and stopped at runtime
with no requirement for any central configuration.

---------------------
Features
---------------------

The system can handle all the basic data types, including
arrays and custom designed datatypes.

The database is intended to be dynamic, able to grow and shrink
while the tag server is running.

Tag data retention the stores data to disk when the tag server
exits and reloads that data when the server restarts.

Event generation allows client modules to be notified by the tag
server when certain changes are made to tags.  This frees the module
from constantly having to poll the tag server for tag changes and
makes the system very responsive.

Lua is used as the primary configuration language for all of the
OpneDAX components.  This allows a massive amount of flexibility in
how the different processes are configured.

Client modules that are part of the base distribution include...

* Modbus Communication Module - Modbus TCP Client/Server and Modbus
  RTU Master/Slave functionality.  ASCII Master/Slave capability is planned.
* MQTT Communication Module - Uses the Eclipse Paho library for
  connection to MQTT broker and uses Lua as filter language so
  that any arbitrary type of data format can be used.
* PLCTAG Communication Module - Uses the lipplctag library to communicate
  to PLCs.  This library usees a tag based abstraction that makes reading
  and writing to various PLCs consistent.  The main purpose for this
  module is to cummunicate to Allen Bradley PLCs using EIP/CIP.  The library
  includes Modbus TCP client capability as well.
* Lua Scripting Module - Lua is the primary scripting language for OpenDAX.
  This module is the primary 'Logic' module included in the distribution.
* Historical Data Logging - Plug-in based historical logging module that
  can store data to disk files or database servers.  The module is not
  complete.
* Command Line Module - Allows the user to connect to the tag server
  and create, view and edit tags.  Most of the features of the tag
  server are included in this module.  This module is also the
  interface to the rest of the system.  It is a properly behaved
  *nix process that can act interactively as well as be used within
  shell scripts.  It can also take instructions via STDIN so that it
  can function as a command interpreter.

There is a Qt based GUI module that is not included in this distribution.

There are also Lua and Python language bindings so that custom client modules
can be written in those languages.  The Lua bindings are included in this
distribution but the Python bindings are in a separate repository.

A C++ library is planned.

See the OpenDAX GitHub page for more information. https://github.com/OpenDAX

---------------------
Licensing
---------------------

OpenDAX is Open Source software.  It is completely open source and currently
there are no 'enterprise' or 'premium' features that are not included in the
code base.

The core programs of OpenDAX are licensed with the GPL v2 and the libraries are
licensed with the LGPL v2.  See the COPYING and COPYING.LIB files for details.

---------------------
Status
---------------------

At this point the code is, at best, alpha quality and it is not ready to be
installed on mission critical production systems.

All parts of the system are open for modification at this time.  This includes
the ABI of the libraries as well as the communication protocol itself.  The
ABI will not be changed unless absolutely necessary.

The tag server and the module interface library are fairly feature complete
at this point and a lot of testing has been done.  Basic functionality has
been tested but the edge cases still need to be identified and tested.

The master daemon will launch the processes properly and seems to do a good
job of restarting failed processes when necessary.  There are still quite
a few features that need to be added and a lot of testing needs to be done.

The 'daxlua' Lua scripting module is feature complete and seems to work
well.  It's been used in the wild and a lot of bugs have been found and
fixed.  More tesing needs to be done.

The Modbus module is feature complete except for the Modbus ASCII protocol.
There has been a lot of testing on the Modbus module.  It has also been
used on some fairly large projects and has performed well.

The MQTT module contains basic pub/sub capability but proper QOS handling
and encrypted connections still have not been implemented.  Very little
testing has been done on this module.

The plctag module is very young.  It seems to work well on tags of basic
data types (including BOOLs) but strings and UDTs have not been done yet
and only ControlLogix PLCs have been tested.

The 'histlog' historical logging module has the basic structure of the
program done and a couple of simple plugins have been written.  writing
data to the database seems to work well but there is no code for querying
the databases.  We are still trying to decide the best way to do this.
The plugins that are currently working are the SQLite plugin and a comma
delimited text file plugin.  So the data is still available but the
user will have to imlement those functions themselves, until the
other tools are built to abstract the data retrieval.

---------------------
Installation
---------------------

OpenDAX uses the CMake build system generator.  You'll need to install CMake
on your system.

You will also need the Lua development libraries installed.  Most
distributions have versions of Lua that will work.  The currently supported
versions of Lua are 5.3 and 5.4.
If you install Lua from the source files you will need to add -FPIC
compiler flag to the build.

make MYCFLAGS="-fPIC" linux

Lua is the only required library dependecy for the system to compile but some
modules require other libraries be installed on the system.  They are detected
during configuation and messages will be given if these libraries are not found.
The main ones are SQLite for tag retention and some historical logging
module plugins. Eclipse Paho is required for the MQTT module and
libplctag is required for the daxplctag module.

Once you have CMake and the Lua libraries installed you can download and build
OpenDAX.  First clone the repository...

git clone https://github.com/OpenDAX/OpenDAX.git

This should create the OpenDAX directory.  Now do the following...

| mkdir build
| cd build
| cmake ..
| make
| make test

If all the tests pass you can install with...

| sudo make install
| sudo ldconfig


