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
import PythonTests.util.daxwrapper as daxwrapper
import testconfig



class TestTagDelete(unittest.TestCase):

    def setUp(self):
        self.server = subprocess.Popen([testconfig.tagserver_file,
                                        "-C",
                                        "tests/config/tagserver_basic.conf"],
                                        stdout=subprocess.DEVNULL
                                        )
        time.sleep(0.1)
        x = self.server.poll()
        self.assertIsNone(x)
        self.dax = daxwrapper.LibDaxWrapper()
        self.ds = self.dax.dax_init("test")
        x = self.dax.dax_init_config(self.ds, "test")
        x = self.dax.dax_configure(self.ds, ["test"], 4)
        x = self.dax.dax_connect(self.ds)

    def tearDown(self):
        x = self.dax.dax_disconnect(self.ds)
        self.server.terminate()
        self.server.wait()

    def test_tag_del_basic(self):
        h = self.dax.dax_tag_add(self.ds, "dummy", "DINT", 1)
        self.dax.dax_write_tag(self.ds, h, bytes([0,0,0,57]))
        x = self.dax.dax_read_tag(self.ds, h)
        self.assertEqual(x, bytes([0,0,0,57]))
        self.dax.dax_tag_del(self.ds, h.index)
        with self.assertRaises(RuntimeError):
            x = self.dax.dax_read_tag(self.ds, h)

    def test_tag_del_with_mapping(self):
        src = self.dax.dax_tag_add(self.ds, "dummy", "INT", 1)
        dest = self.dax.dax_tag_add(self.ds, "dopey", "INT", 1)
        self.dax.dax_map_add(self.ds, src, dest)
        b = b'\x00\x10'
        self.dax.dax_write_tag(self.ds, src, b)
        x = self.dax.dax_read_tag(self.ds, dest)
        self.assertEqual(b,x)
        self.dax.dax_tag_del(self.ds, dest.index)
        with self.assertRaises(RuntimeError):
            x = self.dax.dax_read_tag(self.ds, dest)
        # Make sure this doesn't cause us to crash
        self.dax.dax_write_tag(self.ds, src, b)


if __name__ == '__main__':
    unittest.main()
