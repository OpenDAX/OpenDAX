#  Copyright (c) 2019 Phil Birkelbach
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

# This script starts the tagserver then launches the Lua script given in
# in the first argument.  The second argument will cause a helper script
# to be executed.

import subprocess
import pexpect
import time
import sys
import os

build_dir = "@CMAKE_BINARY_DIR@"
tagserver_file = "{}/src/server/tagserver".format(build_dir)
helper = None

server = subprocess.Popen([tagserver_file,
                           "-C",
                           "PythonTests/config/tagserver_basic.conf"],
                           stdout=subprocess.DEVNULL
                           )
time.sleep(0.1)
x = server.poll()
if(x): raise Exception("Tagserver did not start")

# Start the helper script if one is given
if len(sys.argv) >= 3:
    helper = subprocess.Popen(["lua", sys.argv[2]],stdout=subprocess.DEVNULL)
    time.sleep(0.1)
    x = helper.poll()
    if x: raise Exception("Helper script did not start")


x = os.system("lua {}".format(sys.argv[1]))

server.terminate()
server.wait()
if helper is not None:
    helper.terminate()
    helper.wait()
if(x): raise Exception("Lua Script returned {}".format(x))
