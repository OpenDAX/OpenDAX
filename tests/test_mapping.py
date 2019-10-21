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
import pexpect
import signal
import time
import tests.util.daxwrapper as daxwrapper


class TestMapping(unittest.TestCase):

    def setUp(self):
        self.server = subprocess.Popen(["src/server/tagserver",
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

    def test_mapping_add(self):
        t = daxwrapper.defines["DAX_INT"]
        src = self.dax.dax_tag_add(self.ds, "DummyIn", t)
        dest = self.dax.dax_tag_add(self.ds, "DummyOut", t)
        self.dax.dax_map_add(self.ds, src, dest)

    def test_map_single_int(self):
        t = daxwrapper.defines["DAX_INT"]
        src = self.dax.dax_tag_add(self.ds, "DummySrc", t)
        dest = self.dax.dax_tag_add(self.ds, "DummyDest", t)
        self.dax.dax_map_add(self.ds, src, dest)
        b = b'\x00\x10'
        self.dax.dax_write_tag(self.ds, src, b)
        x = self.dax.dax_read_tag(self.ds, dest)
        self.assertEqual(b,x)

    def test_map_array_int(self):
        t = daxwrapper.defines["DAX_INT"]
        self.dax.dax_tag_add(self.ds, "DummySrc", t, 16)
        self.dax.dax_tag_add(self.ds, "DummyDest", t, 16)
        src = self.dax.dax_tag_handle(self.ds, "DummySrc[0]", 1)
        dest = self.dax.dax_tag_handle(self.ds, "DummyDest[3]", 1)
        self.dax.dax_map_add(self.ds, src, dest)
        b = b'\x00\x10'
        self.dax.dax_write_tag(self.ds, src, b)
        x = self.dax.dax_read_tag(self.ds, dest)
        self.assertEqual(b,x)

    def test_map_single_bool(self):
        t = daxwrapper.defines["DAX_BOOL"]
        src = self.dax.dax_tag_add(self.ds, "DummySrc", t)
        dest = self.dax.dax_tag_add(self.ds, "DummyDest", t)
        self.dax.dax_map_add(self.ds, src, dest)
        b = b'\x01'
        self.dax.dax_write_tag(self.ds, src, b)
        x = self.dax.dax_read_tag(self.ds, dest)
        self.assertEqual(b,x)

    def test_map_array_bool(self):
        t = daxwrapper.defines["DAX_BOOL"]
        self.dax.dax_tag_add(self.ds, "DummySrc", t, 16)
        self.dax.dax_tag_add(self.ds, "DummyDest", t, 16)
        src = self.dax.dax_tag_handle(self.ds, "DummySrc[1]", 1)
        dest = self.dax.dax_tag_handle(self.ds, "DummyDest[8]", 1)
        self.dax.dax_map_add(self.ds, src, dest)
        b = b'\x01'
        self.dax.dax_write_tag(self.ds, src, b)
        x = self.dax.dax_read_tag(self.ds, dest)
        self.assertEqual(b,x)

        src = self.dax.dax_tag_handle(self.ds, "DummySrc[8]", 1)
        dest = self.dax.dax_tag_handle(self.ds, "DummyDest[1]", 1)
        self.dax.dax_map_add(self.ds, src, dest)
        b = b'\x01'
        self.dax.dax_write_tag(self.ds, src, b)
        x = self.dax.dax_read_tag(self.ds, dest)
        self.assertEqual(b,x)


# Test single bools
# test array to single bools
# test single to array bools
# test array to array bools (different indexes)
# test single basic types
# test array to single basic types
# test single to array basic types
# test array to array basic types
# test bools to basic types with different bit indexs (this is the doosey)
# test basic types to bools
# test cdt memebers bools, basic types etc.
# test infinite mapping chain fail


if __name__ == '__main__':
    unittest.main()
