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

# This file is for development of single tests.  Once the test is working
# it would probably be moved to another file.  This is just for convenience

import unittest
import subprocess
import pexpect
import signal
import time
import tests.util.daxwrapper as daxwrapper


class TestSingle(unittest.TestCase):

    def setUp(self):
        self.server = subprocess.Popen(["src/server/tagserver",
                                        "-C",
                                        "tests/config/tagserver_basic.conf"],
                                        #stdout=subprocess.DEVNULL
                                        )
        time.sleep(0.1)
        x = self.server.poll()
        self.assertIsNone(x)
        self.dax = daxwrapper.LibDaxWrapper()
        self.ds = self.dax.dax_init("test")
        x = self.dax.dax_init_config(self.ds, "test")
        x = self.dax.dax_configure(self.ds, ["test"], 4)
        x = self.dax.dax_connect(self.ds)

        members = [("Int5", "INT", 5),     # 10
                   ("Bool10", "BOOL", 10), #  2
                   ("Dint1", "DINT", 1),   #  4
                   ("Dint3", "DINT", 3)]   # 12
                                           # 28 Total
        test1 = self.dax.dax_add_cdt(self.ds, "Test1", members)

    def tearDown(self):
        x = self.dax.dax_disconnect(self.ds)
        self.server.terminate()
        self.server.wait()

    def test_cdt_add(self):
        self.dax.dax_tag_add(self.ds, "TestCDT", "Test1")
        h = self.dax.dax_tag_handle(self.ds, "TestCDT.Int5.4", 1)
        self.assertEqual(h.byte, 0)
        self.assertEqual(h.size, 1)
        self.assertEqual(h.type, daxwrapper.defines["DAX_BOOL"])
        self.assertEqual(h.bit, 4)
        self.assertEqual(h.count, 1)



if __name__ == '__main__':
    unittest.main()
