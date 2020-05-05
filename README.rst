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

The database is intended to be dynamic,
able to grow and shrink as the program runs.

---------------------
Licensing
---------------------


---------------------
Status
---------------------

At this point the code is at best, pre-alpha quality and it is not ready to be 
installed on production systems.

---------------------
Installation
---------------------

OpenDAX uses the CMake build system generator.  You'll need to install CMake
on your system.

You will also need the Lua development libraries installed.  Most 
distributions have versions of Lua that will work.  The currently supported
versions of Lua are 5.1, 5.2 and 5.3 at the moment.
If you install Lua from the source files you will need to add -FPIC 
compiler flag to the build.

make MYCFLAGS="-fPIC" linux

Once you have CMake and the Lua libraries installed you can download and build
OpenDAX.  First clone the repository...

git clone https://github.com/OpenDAX/OpenDAX.git

This should create the OpenDAX directory.  Now do the following...

mkdir build
cd build
cmake ..
make
make test

If all the tests pass you can install with...

sudo make install
sudo ldconfig


