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


import unittest
import subprocess
import signal
import time
from ctypes import *

import tests.util as util

defines = util.read_defines("src/modules/modbus/lib/modbus.h")

class TestRTUMaster(unittest.TestCase):
    def setUp(self):
        # We start socat to give us a couple of ptys to use as our serial ports
        self.socat = subprocess.Popen(["socat",
                                       "-d","-d",
                                       "pty,raw,echo=0,link=/tmp/serial1",
                                       "pty,raw,echo=0,link=/tmp/serial2"])
        time.sleep(0.1)
        x = self.socat.poll()
        self.assertIsNone(x)
        self.libmodbus = cdll.LoadLibrary("src/modules/modbus/lib/.libs/libmodbus.so")

    def tearDown(self):
        self.socat.terminate()
        self.socat.wait()

    def mb_new_port(self, name, flags):
        name = name.encode('utf-8')
        func = self.libmodbus.mb_new_port
        func.restype = c_void_p
        return func(name, flags)

    def test_modbus_RTU_master_connect(self):
        """Initialize, connect and disconnect from the slave"""
        port = self.mb_new_port("test", 0)



if __name__ == '__main__':
    unittest.main()
