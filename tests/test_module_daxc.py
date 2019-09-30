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

daxc_path = "src/modules/daxc/daxc"

class TestDaxc(unittest.TestCase):

    def setUp(self):
        self.server = subprocess.Popen(["src/server/tagserver",
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
        self.server.terminate()
        self.server.wait()
        x = self.dax.dax_disconnect(self.ds)


    def test_create_single_tag(self):
        """add a single tag with and without the count argument"""
        p = pexpect.spawn(daxc_path)
        p.expect('dax>')
        p.sendline('add dummy int')
        p.expect('dax>')
        p.sendline('add dummy2 int 1')
        p.expect('dax>')
        p.sendline('list')
        p.expect('dax>')
        l = (p.before.decode('utf-8').split('\n'))
        self.assertIn("dummy", l[2])
        self.assertIn("INT", l[2])
        self.assertNotIn("INT[", l[2])
        t = self.dax.dax_tag_byname(self.ds, "dummy")
        self.assertEqual(t.type, daxwrapper.defines["DAX_INT"])
        self.assertEqual(t.count, 1)
        t = self.dax.dax_tag_byname(self.ds, "dummy2")
        self.assertEqual(t.type, daxwrapper.defines["DAX_INT"])
        self.assertEqual(t.count, 1)
        p.sendline('exit')
        p.expect(pexpect.EOF)

    def test_create_array_tag(self):
        """add an array tag"""
        p = pexpect.spawn(daxc_path)
        p.expect('dax>')
        p.sendline('add dummy int 10')
        p.expect('dax>')
        p.sendline('list')
        p.expect('dax>')
        l = (p.before.decode('utf-8').split('\n'))
        self.assertIn("dummy", l[2])
        self.assertIn("INT[10]", l[2])
        t = self.dax.dax_tag_byname(self.ds, "dummy")
        self.assertEqual(t.type, daxwrapper.defines["DAX_INT"])
        self.assertEqual(t.count, 10)
        p.sendline('exit')
        p.expect(pexpect.EOF)

    def test_create_tag_each_type(self):
        """Create a tag of each type"""
        p = pexpect.spawn(daxc_path)
        p.expect('dax>')
        l = ["BYTE", "SINT", "WORD", "INT", "UINT", "DWORD", "DINT",
             "UDINT", "TIME", "REAL", "LWORD", "LINT", "ULINT", "LREAL"]
        for each in l:
            uc = each
            lc = uc.lower()
            tag = f"dummy_{lc}"
            p.sendline("add {} {}".format(tag, uc))
            p.expect('dax>')
            t = self.dax.dax_tag_byname(self.ds, tag)
            self.assertEqual(t.type, daxwrapper.defines[f"DAX_{uc}"])

    def test_write_bool(self):
        p = pexpect.spawn(daxc_path)
        p.expect('dax>')
        p.sendline('add dummy bool')
        p.expect('dax>')
        p.sendline('write dummy 1')
        p.expect('dax>')
        h = self.dax.dax_tag_handle(self.ds, "dummy")
        data = self.dax.dax_read_tag(self.ds, h)
        self.assertEqual(data, b'\x01')

        p.sendline('add dummy2 bool 16')
        p.expect('dax>')
        p.sendline('write dummy2 0 1 0 1 0 1 0 1')
        p.expect('dax>')
        h = self.dax.dax_tag_handle(self.ds, "dummy2")
        data = self.dax.dax_read_tag(self.ds, h)
        self.assertEqual(data, b'\xaa\x00')

        # This doesn't work yet
        # p.sendline('write dummy2[8] 1 0 1 0 1 0 1 0')
        # p.expect('dax>')
        # h = self.dax.dax_tag_handle(self.ds, "dummy2")
        # data = self.dax.dax_read_tag(self.ds, h)
        # self.assertEqual(data, b'\xaa\x55')

        p.sendline('write dummy2[8] 1')
        p.expect('dax>')
        h = self.dax.dax_tag_handle(self.ds, "dummy2")
        data = self.dax.dax_read_tag(self.ds, h)
        self.assertEqual(data, b'\xaa\x01')

    def test_write_byte(self):
        """check the corner cases for writing to a BYTE"""
        tests = [
            (-1, b'\x00'),
            (1, b'\x01'),
            (128, b'\x80'),
            (255, b'\xFF'),
            (256, b'\xFF')
        ]
        p = pexpect.spawn(daxc_path)
        p.expect('dax>')
        for test in tests:
            p.sendline('add dummy byte')
            p.expect('dax>')
            p.sendline('write dummy {}'.format(test[0]))
            p.expect('dax>')
            h = self.dax.dax_tag_handle(self.ds, "dummy")
            data = self.dax.dax_read_tag(self.ds, h)
            self.assertEqual(data, test[1])

    def test_write_sint(self):
        """check the corner cases for writing to a SINT"""
        tests = [
            (-1, b'\xFF'),
            (-128, b'\x80'),
            (-129, b'\x80'),
            (1, b'\x01'),
            (127, b'\x7F'),
            (128, b'\x7F'),
            (256, b'\x7F')
        ]
        p = pexpect.spawn(daxc_path)
        p.expect('dax>')
        for test in tests:
            p.sendline('add dummy sint')
            p.expect('dax>')
            p.sendline('write dummy {}'.format(test[0]))
            p.expect('dax>')
            h = self.dax.dax_tag_handle(self.ds, "dummy")
            data = self.dax.dax_read_tag(self.ds, h)
            self.assertEqual(data, test[1])

    def test_write_word(self):
        """check the corner cases for writing to a WORD"""
        tests = [
            (-1, b'\x00\x00'),
            (-128, b'\x00\x00'),
            (128, b'\x80\x00'),
            (255, b'\xFF\x00'),
            (256, b'\x00\x01'),
            (65535, b'\xFF\xFF'),
        ]
        p = pexpect.spawn(daxc_path)
        p.expect('dax>')
        for test in tests:
            p.sendline('add dummy word')
            p.expect('dax>')
            p.sendline('write dummy {}'.format(test[0]))
            p.expect('dax>')
            h = self.dax.dax_tag_handle(self.ds, "dummy")
            data = self.dax.dax_read_tag(self.ds, h)
            self.assertEqual(data, test[1])

    def test_write_uint(self):
        """check the corner cases for writing to a UINT"""
        tests = [
            (-1, b'\x00\x00'),
            (-128, b'\x00\x00'),
            (128, b'\x80\x00'),
            (255, b'\xFF\x00'),
            (256, b'\x00\x01'),
            (65535, b'\xFF\xFF'),
        ]
        p = pexpect.spawn(daxc_path)
        p.expect('dax>')
        for test in tests:
            p.sendline('add dummy uint')
            p.expect('dax>')
            p.sendline('write dummy {}'.format(test[0]))
            p.expect('dax>')
            h = self.dax.dax_tag_handle(self.ds, "dummy")
            data = self.dax.dax_read_tag(self.ds, h)
            self.assertEqual(data, test[1])

    def test_write_int(self):
        """check the corner cases for writing to an INT"""
        tests = [
            (-1, b'\xFF\xFF'),
            (-128, b'\x80\xFF'),
            (128, b'\x80\x00'),
            (255, b'\xFF\x00'),
            (256, b'\x00\x01'),
            (-32768, b'\x00\x80'),
            (-32769, b'\x00\x80'),
            (32767, b'\xFF\x7F'),
            (32768, b'\xFF\x7F'),
        ]
        p = pexpect.spawn(daxc_path)
        p.expect('dax>')
        for test in tests:
            p.sendline('add dummy int')
            p.expect('dax>')
            p.sendline('write dummy {}'.format(test[0]))
            p.expect('dax>')
            h = self.dax.dax_tag_handle(self.ds, "dummy")
            data = self.dax.dax_read_tag(self.ds, h)
            self.assertEqual(data, test[1])

    def test_write_dword(self):
        """check the corner cases for writing to a DWORD"""
        tests = [
            (128, b'\x80\x00\x00\x00'),
            (0, b'\x00\x00\x00\x00'),
            (-128, b'\x00\x00\x00\x00'),
            (-1, b'\x00\x00\x00\x00'),
            (255, b'\xFF\x00\x00\x00'),
            (256, b'\x00\x01\x00\x00'),
            (4294967295, b'\xFF\xFF\xFF\xFF'),
            (4294967296, b'\xFF\xFF\xFF\xFF'),
        ]
        p = pexpect.spawn(daxc_path)
        p.expect('dax>')
        for test in tests:
            p.sendline('add dummy dword')
            p.expect('dax>')
            p.sendline('write dummy {}'.format(test[0]))
            p.expect('dax>')
            h = self.dax.dax_tag_handle(self.ds, "dummy")
            data = self.dax.dax_read_tag(self.ds, h)
            self.assertEqual(data, test[1])

    def test_write_udint(self):
        """check the corner cases for writing to a UDINT"""
        tests = [
            (128, b'\x80\x00\x00\x00'),
            (0, b'\x00\x00\x00\x00'),
            (-128, b'\x00\x00\x00\x00'),
            (-1, b'\x00\x00\x00\x00'),
            (255, b'\xFF\x00\x00\x00'),
            (256, b'\x00\x01\x00\x00'),
            (4294967295, b'\xFF\xFF\xFF\xFF'),
            (4294967296, b'\xFF\xFF\xFF\xFF'),
        ]
        p = pexpect.spawn(daxc_path)
        p.expect('dax>')
        for test in tests:
            p.sendline('add dummy udint')
            p.expect('dax>')
            p.sendline('write dummy {}'.format(test[0]))
            p.expect('dax>')
            h = self.dax.dax_tag_handle(self.ds, "dummy")
            data = self.dax.dax_read_tag(self.ds, h)
            self.assertEqual(data, test[1])


    def test_write_dint(self):
        """check the corner cases for writing to a DINT"""
        tests = [
            (-1, b'\xFF\xFF\xFF\xFF'),
            (-128, b'\x80\xFF\xFF\xFF'),
            (128, b'\x80\x00\x00\x00'),
            (255, b'\xFF\x00\x00\x00'),
            (256, b'\x00\x01\x00\x00'),
            (-2147483648, b'\x00\x00\x00\x80'),
            (-2147483649, b'\x00\x00\x00\x80'),
            (2147483647, b'\xFF\xFF\xFF\x7F'),
            (2147483648, b'\xFF\xFF\xFF\x7F'),
        ]
        p = pexpect.spawn(daxc_path)
        p.expect('dax>')
        for test in tests:
            p.sendline('add dummy dint')
            p.expect('dax>')
            p.sendline('write dummy {}'.format(test[0]))
            p.expect('dax>')
            h = self.dax.dax_tag_handle(self.ds, "dummy")
            data = self.dax.dax_read_tag(self.ds, h)
            self.assertEqual(data, test[1])

    def test_write_lword(self):
        """check the corner cases for writing to a LWORD"""
        tests = [
            (128, b'\x80\x00\x00\x00\x00\x00\x00\x00'),
            (0, b'\x00\x00\x00\x00\x00\x00\x00\x00'),
            (-128, b'\x00\x00\x00\x00\x00\x00\x00\x00'),
            (-1, b'\x00\x00\x00\x00\x00\x00\x00\x00'),
            (255, b'\xFF\x00\x00\x00\x00\x00\x00\x00'),
            (256, b'\x00\x01\x00\x00\x00\x00\x00\x00'),
            (18446744073709551615, b'\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF'),
            (18446744073709551616, b'\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF'),
        ]
        p = pexpect.spawn(daxc_path)
        p.expect('dax>')
        for test in tests:
            p.sendline('add dummy lword')
            p.expect('dax>')
            p.sendline('write dummy {}'.format(test[0]))
            p.expect('dax>')
            h = self.dax.dax_tag_handle(self.ds, "dummy")
            data = self.dax.dax_read_tag(self.ds, h)
            self.assertEqual(data, test[1])

    def test_write_ulint(self):
        """check the corner cases for writing to a ULINT"""
        tests = [
            (128, b'\x80\x00\x00\x00\x00\x00\x00\x00'),
            (0, b'\x00\x00\x00\x00\x00\x00\x00\x00'),
            (-128, b'\x00\x00\x00\x00\x00\x00\x00\x00'),
            (-1, b'\x00\x00\x00\x00\x00\x00\x00\x00'),
            (255, b'\xFF\x00\x00\x00\x00\x00\x00\x00'),
            (256, b'\x00\x01\x00\x00\x00\x00\x00\x00'),
            (18446744073709551615, b'\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF'),
            (18446744073709551616, b'\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF'),
        ]
        p = pexpect.spawn(daxc_path)
        p.expect('dax>')
        for test in tests:
            p.sendline('add dummy ulint')
            p.expect('dax>')
            p.sendline('write dummy {}'.format(test[0]))
            p.expect('dax>')
            h = self.dax.dax_tag_handle(self.ds, "dummy")
            data = self.dax.dax_read_tag(self.ds, h)
            self.assertEqual(data, test[1])

    def test_write_lint(self):
        """check the corner cases for writing to a LINT"""
        tests = [
            (128, b'\x80\x00\x00\x00\x00\x00\x00\x00'),
            (0, b'\x00\x00\x00\x00\x00\x00\x00\x00'),
            (-1, b'\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF'),
            (-128, b'\x80\xFF\xFF\xFF\xFF\xFF\xFF\xFF'),
            (255, b'\xFF\x00\x00\x00\x00\x00\x00\x00'),
            (256, b'\x00\x01\x00\x00\x00\x00\x00\x00'),
            (-9223372036854775808, b'\x00\x00\x00\x00\x00\x00\x00\x80'),
            (-9223372036854775809, b'\x00\x00\x00\x00\x00\x00\x00\x80'),
            (9223372036854775807, b'\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x7F'),
            (9223372036854775808, b'\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x7F'),
        ]
        p = pexpect.spawn(daxc_path)
        p.expect('dax>')
        for test in tests:
            p.sendline('add dummy lint')
            p.expect('dax>')
            p.sendline('write dummy {}'.format(test[0]))
            p.expect('dax>')
            h = self.dax.dax_tag_handle(self.ds, "dummy")
            data = self.dax.dax_read_tag(self.ds, h)
            self.assertEqual(data, test[1])


if __name__ == '__main__':
    unittest.main()
