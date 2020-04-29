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

# Early in the project we wrote a test module that uses the Lua interface to
# run through a bunch of tests.  It works pretty well so until those tests can
# be replaced with proper unit tests we will just manually run them from here
import unittest
import subprocess
import pexpect
import signal
import time
import PythonTests.util.daxwrapper as daxwrapper
import testconfig

daxtest_path = "{}/tests/modules".format(testconfig.build_dir)

class TestTestModule(unittest.TestCase):

    def setUp(self):
        self.server = subprocess.Popen([testconfig.tagserver_file,
                                        "-C",
                                        "tests/config/tagserver_basic.conf"],
                                        stdout=subprocess.DEVNULL)
        time.sleep(0.1)
        x = self.server.poll()
        self.assertIsNone(x)
        self.dax = daxwrapper.LibDaxWrapper()
        # self.ds = self.dax.dax_init("test")
        # x = self.dax.dax_init_config(self.ds, "test")
        # x = self.dax.dax_configure(self.ds, ["test"], 4)
        # x = self.dax.dax_connect(self.ds)

    def tearDown(self):
        # x = self.dax.dax_disconnect(self.ds)
        self.server.terminate()
        self.server.wait()


    def test_run_daxtest_module(self):
        """run the old daxtest module"""
        p = pexpect.spawn("{}/daxtest".format(daxtest_path), cwd=daxtest_path, timeout=1.0)
        p.expect("OpenDAX Test Finished, ")
        p.expect(" tests run, ")
        tests_run = int(p.before)
        p.expect(" tests failed")
        tests_failed = int(p.before)
        p.expect(pexpect.EOF)
        # print("Tests Run = {}".format(tests_run))
        # print("Tests Failed = {}".format(tests_failed))
        self.assertEqual(tests_failed, 0)


if __name__ == '__main__':
    unittest.main()
