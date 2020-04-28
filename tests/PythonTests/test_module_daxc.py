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


    def test_create_single_tag(self):
        """add a single tag with and without the count argument"""
        p = pexpect.spawn(daxc_path)
        p.expect('dax>')
        p.sendline('add tag dummy int')
        p.expect('dax>')
        p.sendline('add tag dummy2 int 1')
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
        p.sendline('add tag dummy int 10')
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
            tag = "dummy_{}".format(lc)
            p.sendline("add tag {} {}".format(tag, uc))
            p.expect('dax>')
            t = self.dax.dax_tag_byname(self.ds, tag)
            self.assertEqual(t.type, daxwrapper.defines["DAX_{}".format(uc)])
        p.sendline('exit')
        p.expect(pexpect.EOF)


    def test_write_bool(self):
        p = pexpect.spawn(daxc_path)
        p.expect('dax>')
        p.sendline('add tag dummy bool')
        p.expect('dax>')
        p.sendline('write dummy 1')
        p.expect('dax>')
        h = self.dax.dax_tag_handle(self.ds, "dummy")
        data = self.dax.dax_read_tag(self.ds, h)
        self.assertEqual(data, b'\x01')

        p.sendline('add tag dummy2 bool 16')
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
        p.sendline('exit')
        p.expect(pexpect.EOF)


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
            p.sendline('add tag dummy byte')
            p.expect('dax>')
            p.sendline('write dummy {}'.format(test[0]))
            p.expect('dax>')
            h = self.dax.dax_tag_handle(self.ds, "dummy")
            data = self.dax.dax_read_tag(self.ds, h)
            self.assertEqual(data, test[1])
        p.sendline('exit')
        p.expect(pexpect.EOF)


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
            p.sendline('add tag dummy sint')
            p.expect('dax>')
            p.sendline('write dummy {}'.format(test[0]))
            p.expect('dax>')
            h = self.dax.dax_tag_handle(self.ds, "dummy")
            data = self.dax.dax_read_tag(self.ds, h)
            self.assertEqual(data, test[1])
        p.sendline('exit')
        p.expect(pexpect.EOF)

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
            p.sendline('add tag dummy word')
            p.expect('dax>')
            p.sendline('write dummy {}'.format(test[0]))
            p.expect('dax>')
            h = self.dax.dax_tag_handle(self.ds, "dummy")
            data = self.dax.dax_read_tag(self.ds, h)
            self.assertEqual(data, test[1])
        p.sendline('exit')
        p.expect(pexpect.EOF)

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
            p.sendline('add tag dummy uint')
            p.expect('dax>')
            p.sendline('write dummy {}'.format(test[0]))
            p.expect('dax>')
            h = self.dax.dax_tag_handle(self.ds, "dummy")
            data = self.dax.dax_read_tag(self.ds, h)
            self.assertEqual(data, test[1])
        p.sendline('exit')
        p.expect(pexpect.EOF)

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
            p.sendline('add tag dummy int')
            p.expect('dax>')
            p.sendline('write dummy {}'.format(test[0]))
            p.expect('dax>')
            h = self.dax.dax_tag_handle(self.ds, "dummy")
            data = self.dax.dax_read_tag(self.ds, h)
            self.assertEqual(data, test[1])
        p.sendline('exit')
        p.expect(pexpect.EOF)

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
            p.sendline('add tag dummy dword')
            p.expect('dax>')
            p.sendline('write dummy {}'.format(test[0]))
            p.expect('dax>')
            h = self.dax.dax_tag_handle(self.ds, "dummy")
            data = self.dax.dax_read_tag(self.ds, h)
            self.assertEqual(data, test[1])
        p.sendline('exit')
        p.expect(pexpect.EOF)

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
            p.sendline('add tag dummy udint')
            p.expect('dax>')
            p.sendline('write dummy {}'.format(test[0]))
            p.expect('dax>')
            h = self.dax.dax_tag_handle(self.ds, "dummy")
            data = self.dax.dax_read_tag(self.ds, h)
            self.assertEqual(data, test[1])
        p.sendline('exit')
        p.expect(pexpect.EOF)


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
            p.sendline('add tag dummy dint')
            p.expect('dax>')
            p.sendline('write dummy {}'.format(test[0]))
            p.expect('dax>')
            h = self.dax.dax_tag_handle(self.ds, "dummy")
            data = self.dax.dax_read_tag(self.ds, h)
            self.assertEqual(data, test[1])
        p.sendline('exit')
        p.expect(pexpect.EOF)

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
            p.sendline('add tag dummy lword')
            p.expect('dax>')
            p.sendline('write dummy {}'.format(test[0]))
            p.expect('dax>')
            h = self.dax.dax_tag_handle(self.ds, "dummy")
            data = self.dax.dax_read_tag(self.ds, h)
            self.assertEqual(data, test[1])
        p.sendline('exit')
        p.expect(pexpect.EOF)

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
            p.sendline('add tag dummy ulint')
            p.expect('dax>')
            p.sendline('write dummy {}'.format(test[0]))
            p.expect('dax>')
            h = self.dax.dax_tag_handle(self.ds, "dummy")
            data = self.dax.dax_read_tag(self.ds, h)
            self.assertEqual(data, test[1])
        p.sendline('exit')
        p.expect(pexpect.EOF)

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
            p.sendline('add tag dummy lint')
            p.expect('dax>')
            p.sendline('write dummy {}'.format(test[0]))
            p.expect('dax>')
            h = self.dax.dax_tag_handle(self.ds, "dummy")
            data = self.dax.dax_read_tag(self.ds, h)
            self.assertEqual(data, test[1])
        p.sendline('exit')
        p.expect(pexpect.EOF)

    def test_write_real(self):
        """make sure that we can write real values properly"""
        tests = [
            ("0", b'\x00\x00\x00\x00'),
            ("0.23", b'\x1F\x85\x6B\x3E'),
            ("3.141592", b'\xD8\x0F\x49\x40'),
            ("1e+08", b'\x20\xbc\xbe\x4c'),
            ("3.123456e+23", b'\x94\x48\x84\x66'),
            ("-3.123456e+23", b'\x94\x48\x84\xe6'),
        ]
        p = pexpect.spawn(daxc_path)
        p.expect('dax>')
        for test in tests:
            p.sendline('add tag dummy real')
            p.expect('dax>')
            p.sendline('write dummy {}'.format(test[0]))
            p.expect('dax>')
            h = self.dax.dax_tag_handle(self.ds, "dummy")
            data = self.dax.dax_read_tag(self.ds, h)
            self.assertEqual(data, test[1])
        p.sendline('exit')
        p.expect(pexpect.EOF)

    def test_write_lreal(self):
        """make sure that we can write lreal values properly"""
        tests = [
            ("0", b'\x00\x00\x00\x00\x00\x00\x00\x00'),
            ("3.12345678911234e+23", b'\xe0\xdb\x7e\xc3\x12\x89\xd0\x44'),
            ("-3.12345678911234e+23", b'\xe0\xdb\x7e\xc3\x12\x89\xd0\xc4'),
            ("0.1", b'\x9a\x99\x99\x99\x99\x99\xb9\x3f'),
        ]
        p = pexpect.spawn(daxc_path)
        p.expect('dax>')
        for test in tests:
            p.sendline('add tag dummy lreal')
            p.expect('dax>')
            p.sendline('write dummy {}'.format(test[0]))
            p.expect('dax>')
            h = self.dax.dax_tag_handle(self.ds, "dummy")
            data = self.dax.dax_read_tag(self.ds, h)
            self.assertEqual(data, test[1])
        p.sendline('exit')
        p.expect(pexpect.EOF)

    def test_read_bool(self):
        p = pexpect.spawn(daxc_path)
        p.expect('dax>')
        p.sendline('add tag dummy bool')
        p.expect('dax>')
        p.sendline('read dummy')
        answer = p.readline()
        p.expect('dax>')
        answer = p.before
        self.assertEqual(answer, b'0\r\n')
        p.sendline('add tag dummy2 bool 16')
        p.expect('dax>')
        p.sendline('write dummy2 0 1 0 1 0 1 0 1')
        p.expect('dax>')
        p.sendline('read dummy2')
        p.expect('dax>')
        self.assertEqual(p.before, b'read dummy2\r\n0\r\n1\r\n0\r\n1\r\n0\r\n1\r\n0\r\n1\r\n0\r\n0\r\n0\r\n0\r\n0\r\n0\r\n0\r\n0\r\n')
        p.sendline('write dummy2[8] 1')
        p.expect('dax>')
        p.sendline('read dummy2[8]')
        p.expect('dax>')
        self.assertEqual(p.before, b'read dummy2[8]\r\n1\r\n')
        p.sendline('write dummy2[8] 0')
        p.expect('dax>')
        p.sendline('read dummy2[8]')
        p.expect('dax>')
        self.assertEqual(p.before, b'read dummy2[8]\r\n0\r\n')
        p.sendline('exit')
        p.expect(pexpect.EOF)

    def test_read_byte(self):
        """check that we can read a BYTE"""
        p = pexpect.spawn(daxc_path)
        p.expect('dax>')
        p.sendline('add tag dummy byte')
        p.expect('dax>')
        p.sendline('write dummy 57')
        p.expect('dax>')
        p.sendline('read dummy')
        p.expect('dax>')
        self.assertEqual(p.before, b'read dummy\r\n57\r\n')
        p.sendline('exit')
        p.expect(pexpect.EOF)

    def test_read_sint(self):
        """check that we can read a SINT"""
        p = pexpect.spawn(daxc_path)
        p.expect('dax>')
        p.sendline('add tag dummy sint')
        p.expect('dax>')
        p.sendline('write dummy -57')
        p.expect('dax>')
        p.sendline('read dummy')
        p.expect('dax>')
        self.assertEqual(p.before, b'read dummy\r\n-57\r\n')
        p.sendline('exit')
        p.expect(pexpect.EOF)

    def test_read_word(self):
        """check that we can read a WORD"""
        p = pexpect.spawn(daxc_path)
        p.expect('dax>')
        p.sendline('add tag dummy word')
        p.expect('dax>')
        p.sendline('write dummy 31234')
        p.expect('dax>')
        p.sendline('read dummy')
        p.expect('dax>')
        self.assertEqual(p.before, b'read dummy\r\n31234\r\n')
        p.sendline('exit')
        p.expect(pexpect.EOF)

    def test_read_uint(self):
        """check that we can read a UINT"""
        p = pexpect.spawn(daxc_path)
        p.expect('dax>')
        p.sendline('add tag dummy UINT')
        p.expect('dax>')
        p.sendline('write dummy 31235')
        p.expect('dax>')
        p.sendline('read dummy')
        p.expect('dax>')
        self.assertEqual(p.before, b'read dummy\r\n31235\r\n')
        p.sendline('exit')
        p.expect(pexpect.EOF)

    def test_read_int(self):
        """check that we can read a INT"""
        p = pexpect.spawn(daxc_path)
        p.expect('dax>')
        p.sendline('add tag dummy INT')
        p.expect('dax>')
        p.sendline('write dummy -31235')
        p.expect('dax>')
        p.sendline('read dummy')
        p.expect('dax>')
        self.assertEqual(p.before, b'read dummy\r\n-31235\r\n')
        p.sendline('exit')
        p.expect(pexpect.EOF)

    def test_read_dword(self):
        """check that we can read a DWORD"""
        p = pexpect.spawn(daxc_path)
        p.expect('dax>')
        p.sendline('add tag dummy dword')
        p.expect('dax>')
        p.sendline('write dummy 31235000')
        p.expect('dax>')
        p.sendline('read dummy')
        p.expect('dax>')
        self.assertEqual(p.before, b'read dummy\r\n31235000\r\n')
        p.sendline('exit')
        p.expect(pexpect.EOF)

    def test_read_udint(self):
        """check that we can read a UDINT"""
        p = pexpect.spawn(daxc_path)
        p.expect('dax>')
        p.sendline('add tag dummy udint')
        p.expect('dax>')
        p.sendline('write dummy 312350001')
        p.expect('dax>')
        p.sendline('read dummy')
        p.expect('dax>')
        self.assertEqual(p.before, b'read dummy\r\n312350001\r\n')
        p.sendline('exit')
        p.expect(pexpect.EOF)

    def test_read_dint(self):
        """check that we can read a DINT"""
        p = pexpect.spawn(daxc_path)
        p.expect('dax>')
        p.sendline('add tag dummy dint')
        p.expect('dax>')
        p.sendline('write dummy -312350001')
        p.expect('dax>')
        p.sendline('read dummy')
        p.expect('dax>')
        self.assertEqual(p.before, b'read dummy\r\n-312350001\r\n')
        p.sendline('exit')
        p.expect(pexpect.EOF)

    def test_read_lword(self):
        """check that we can read a LWORD"""
        p = pexpect.spawn(daxc_path)
        p.expect('dax>')
        p.sendline('add tag dummy lword')
        p.expect('dax>')
        p.sendline('write dummy 312350001000')
        p.expect('dax>')
        p.sendline('read dummy')
        p.expect('dax>')
        self.assertEqual(p.before, b'read dummy\r\n312350001000\r\n')
        p.sendline('exit')
        p.expect(pexpect.EOF)

    def test_read_ulint(self):
        """check that we can read a ULINT"""
        p = pexpect.spawn(daxc_path)
        p.expect('dax>')
        p.sendline('add tag dummy ulint')
        p.expect('dax>')
        p.sendline('write dummy 312350001001')
        p.expect('dax>')
        p.sendline('read dummy')
        p.expect('dax>')
        self.assertEqual(p.before, b'read dummy\r\n312350001001\r\n')
        p.sendline('exit')
        p.expect(pexpect.EOF)

    def test_read_lint(self):
        """check that we can read a LINT"""
        p = pexpect.spawn(daxc_path)
        p.expect('dax>')
        p.sendline('add tag dummy lint')
        p.expect('dax>')
        p.sendline('write dummy -312350001001')
        p.expect('dax>')
        p.sendline('read dummy')
        p.expect('dax>')
        self.assertEqual(p.before, b'read dummy\r\n-312350001001\r\n')
        p.sendline('exit')
        p.expect(pexpect.EOF)

    def test_read_real(self):
        """check that we can read a REAL"""
        p = pexpect.spawn(daxc_path)
        p.expect('dax>')
        p.sendline('add tag dummy real')
        p.expect('dax>')
        p.sendline('write dummy 3.141592')
        p.expect('dax>')
        p.sendline('read dummy')
        p.expect('dax>')
        self.assertEqual(p.before, b'read dummy\r\n3.141592\r\n')
        p.sendline('exit')
        p.expect(pexpect.EOF)

    def test_read_lreal(self):
        """check that we can read a LREAL"""
        p = pexpect.spawn(daxc_path)
        p.expect('dax>')
        p.sendline('add tag dummy lreal')
        p.expect('dax>')
        p.sendline('write dummy -312351234567.123456789')
        p.expect('dax>')
        p.sendline('read dummy')
        p.expect('dax>')
        self.assertEqual(p.before, b'read dummy\r\n-312351234567.1235\r\n')
        p.sendline('exit')
        p.expect(pexpect.EOF)

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
            p.sendline('add tag dummy lword')
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
            p.sendline('add tag dummy ulint')
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
            p.sendline('add tag dummy lint')
            p.expect('dax>')
            p.sendline('write dummy {}'.format(test[0]))
            p.expect('dax>')
            h = self.dax.dax_tag_handle(self.ds, "dummy")
            data = self.dax.dax_read_tag(self.ds, h)
            self.assertEqual(data, test[1])


if __name__ == '__main__':
    unittest.main()
