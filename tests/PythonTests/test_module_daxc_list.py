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
import PythonTests.util.daxwrapper as daxwrapper
import testconfig

daxc_path = "{}/src/modules/daxc/daxc".format(testconfig.build_dir)

class TestDaxc(unittest.TestCase):

    def setUp(self):
        self.server = subprocess.Popen([testconfig.tagserver_file,
                                        "-C",
                                        "tests/config/tagserver_basic.conf"],
                                        stdout=subprocess.DEVNULL)
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


    def test_list_tag_index(self):
        """list a tag by it's index"""
        p = pexpect.spawn(daxc_path)
        p.expect('dax>')
        p.sendline('add tag dummy int 1')
        p.expect('dax>')
        p.sendline('add tag dopey int 1')
        p.expect('dax>')
        p.sendline('add tag dipey int 1')
        p.expect('dax>')
        p.sendline('list tag 1')
        p.expect('dax>')
        #l = (p.before.decode('utf-8').split('\n'))
        #self.assertIn("inner", l[2])
        #p.sendline('list type inner')
        #p.expect('dax>')
        #l = (p.before.decode('utf-8').split('\n'))
        #self.assertIn("dummy", l[1])
        #self.assertIn("INT[1]", l[1])
        #self.assertIn("dopey", l[2])
        #self.assertIn("DINT[4]", l[2])
        p.sendline('exit')
        p.expect(pexpect.EOF)


if __name__ == '__main__':
    unittest.main()
