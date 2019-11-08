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

badnames =  ["1Tag", "-Tag", "Tag-name", "Tag&name", "Tag+name", "tag/name",
             "tag*name", "TAG?NAME", "TagNameIsWayTooLong12345678912345"]
goodnames = ["_Tag", "Tag1", "tAg_name", "t1Ag_name", "TagNameIsBarelyLongEnoughToFit12"]


class TestSingle(unittest.TestCase):

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

    def test_tag_add_bad_names(self):
        for name in badnames:
            with self.assertRaises(RuntimeError):
                self.dax.dax_tag_add(self.ds, name, "DINT", 1)

    def test_tag_add_good_names(self):
        for name in goodnames:
            h = self.dax.dax_tag_add(self.ds, name, "DINT", 1)

    def test_tag_add_bad_type(self):
        with self.assertRaises(RuntimeError):
            self.dax.dax_tag_add(self.ds, "test1", 1, 1)


    def test_tag_add_tag_modify_success(self):
        # Create a tag
        h = self.dax.dax_tag_add(self.ds, "TagModifyTest", "INT", 8)
        # read it tomake sure that it's in the library cache
        x = self.dax.dax_read_tag(self.ds, h)

        # Make it bigger
        h = self.dax.dax_tag_add(self.ds, "TagModifyTest", "INT", 12)
        # see if we get what we expect
        x = self.dax.dax_read_tag(self.ds, h)
        self.assertEqual(len(x), 24)

        # Shrinking it should work as well
        h = self.dax.dax_tag_add(self.ds, "TagModifyTest", "INT", 10)
        # see if we get what we expect
        x = self.dax.dax_read_tag(self.ds, h)
        self.assertEqual(len(x), 20)

        # Create a tag
        h = self.dax.dax_tag_add(self.ds, "TagModifyTest2", "BOOL", 8)
        # read it tomake sure that it's in the library cache
        x = self.dax.dax_read_tag(self.ds, h)

        # Make it bigger
        h = self.dax.dax_tag_add(self.ds, "TagModifyTest2", "BOOL", 32)
        # see if we get what we expect
        x = self.dax.dax_read_tag(self.ds, h)
        self.assertEqual(len(x), 4)

        # Shrinking it should work as well
        h = self.dax.dax_tag_add(self.ds, "TagModifyTest2", "BOOL", 10)
        # see if we get what we expect
        x = self.dax.dax_read_tag(self.ds, h)
        self.assertEqual(len(x), 2)


    # --Create the tag and then access it to make sure that it's in the cache
    # tag_add("TagModifyTest", "INT", 8)
    # tag_read("TagModifyTest", 8)
    #
    # --Modifying tags is allowed if the onlything that is changed is the length
    # --of the tag array gets larger.  Type and name cannot change.
    # --Modifiy it and then access the new part, if the cache has been cleared this should work
    # tag_add("TagModifyTest", "INT", 12)
    # tag_read("TagModifyTest[8]", 4)
    #
    # tag_add("TagModifyBool", "BOOL", 4)
    # tag_write("TagModifyBool[1]", 1)
    #
    # tag_add("TagModifyBool", "BOOL", 8)
    # tag_write("TagModifyBool[5]", 1)
    #
    # tag_add("TagModifyBool", "BOOL", 12)

    def test_tag_add_tag_modify_fail(self):
        # Create a tag
        h = self.dax.dax_tag_add(self.ds, "TagModifyTest", "INT", 8)
        # read it tomake sure that it's in the library cache
        x = self.dax.dax_read_tag(self.ds, h)

        # Changing the type should fail
        with self.assertRaises(RuntimeError):
            h = self.dax.dax_tag_add(self.ds, "TagModifyTest", "DINT", 8)


if __name__ == '__main__':
    unittest.main()
