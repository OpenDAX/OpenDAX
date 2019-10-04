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
import tests.util.daxwrapper as daxwrapper

daxc_path = "src/modules/daxc/daxc"

class TestDaxc(unittest.TestCase):

    def setUp(self):
        self.dax = daxwrapper.LibDaxWrapper()

    def tearDown(self):
        pass

    def test_val_to_string_basic(self):
        """check the dax_val_to_string() function"""
        #         val       type      index   test
        tests = [
                 (b'\x00', "DAX_BYTE", 0, "0"),
                 (b'\xFF', "DAX_BYTE", 0, "255"),
                 (b'\x80', "DAX_BYTE", 0, "128"),
                 (b'\x80', "DAX_SINT", 0, "-128"),
                 (b'\x00\x01', "DAX_UINT", 0, "256"),
                 (b'\x00\x00', "DAX_UINT", 0, "0"),
                 (b'\xFF\xFF', "DAX_UINT", 0, "65535"),
                 (b'\x00\x01', "DAX_WORD", 0, "256"),
                 (b'\x00\x00', "DAX_WORD", 0, "0"),
                 (b'\xFF\xFF', "DAX_WORD", 0, "65535"),
                 (b'\xFF\xFF', "DAX_INT", 0, "-1"),
                 (b'\xFF\x7F', "DAX_INT", 0, "32767"),
                 (b'\x00\x80', "DAX_INT", 0, "-32768"),
                 (b'\x00\x00', "DAX_INT", 0, "0"),
                 (b'\x00\x00\x00\x00', "DAX_UDINT", 0, "0"),
                 (b'\x00\x01\x00\x00', "DAX_UDINT", 0, "256"),
                 (b'\xFF\xFF\xFF\xFF', "DAX_UDINT", 0, "4294967295"),
                 (b'\x00\x00\x00\x00', "DAX_UDINT", 0, "0"),
                 (b'\x00\x00\x00\x00', "DAX_DWORD", 0, "0"),
                 (b'\x00\x01\x00\x00', "DAX_DWORD", 0, "256"),
                 (b'\xFF\xFF\xFF\xFF', "DAX_DWORD", 0, "4294967295"),
                 (b'\x00\x00\x00\x00', "DAX_DWORD", 0, "0"),
                 (b'\x00\x00\x00\x00', "DAX_DINT", 0, "0"),
                 (b'\x00\x00\x00\x80', "DAX_DINT", 0, "-2147483648"),
                 (b'\xFF\xFF\xFF\x7F', "DAX_DINT", 0, "2147483647"),
                 (b'\xFF\xFF\xFF\xFF', "DAX_DINT", 0, "-1"),
                 (b'\x00\x00\x00\x00', "DAX_REAL", 0, "0"),
                 (b'\x1F\x85\x6B\x3E', "DAX_REAL", 0, "0.23"),
                 (b'\xD8\x0F\x49\x40', "DAX_REAL", 0, "3.141592"),
                 (b'\x20\xbc\xbe\x4c', "DAX_REAL", 0, "1e+08"),
                 (b'\x94\x48\x84\x66', "DAX_REAL", 0, "3.123456e+23"),
                 (b'\x94\x48\x84\xe6', "DAX_REAL", 0, "-3.123456e+23"),
                 (b'\x00\x00\x00\x00\x00\x00\x00\x00', "DAX_ULINT", 0, "0"),
                 (b'\x00\x00\x00\x00\x00\x00\x00\x80', "DAX_ULINT", 0, "9223372036854775808"),
                 (b'\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF', "DAX_ULINT", 0, "18446744073709551615"),
                 (b'\x00\x00\x00\x00\x00\x00\x00\x00', "DAX_LWORD", 0, "0"),
                 (b'\x00\x00\x00\x00\x00\x00\x00\x80', "DAX_LWORD", 0, "9223372036854775808"),
                 (b'\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF', "DAX_LWORD", 0, "18446744073709551615"),
                 (b'\x00\x00\x00\x00\x00\x00\x00\x00', "DAX_LINT", 0, "0"),
                 (b'\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF', "DAX_LINT", 0, "-1"),
                 (b'\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x7F', "DAX_LINT", 0, "9223372036854775807"),
                 (b'\x00\x00\x00\x00\x00\x00\x00\x80', "DAX_LINT", 0, "-9223372036854775808"),
                 (b'\x00\x00\x00\x00\x00\x00\x00\x00', "DAX_LREAL", 0, "0"),
                 (b'\xe0\xdb\x7e\xc3\x12\x89\xd0\x44', "DAX_LREAL", 0, "3.12345678911234e+23"),
                 (b'\xe0\xdb\x7e\xc3\x12\x89\xd0\xc4', "DAX_LREAL", 0, "-3.12345678911234e+23"),
                 (b'\x9a\x99\x99\x99\x99\x99\xb9\x3f', "DAX_LREAL", 0, "0.1"),
        ]
        for test in tests:
            # print(test)
            val = test[0]
            type = daxwrapper.defines[test[1]]
            # index = test[2]
            result = self.dax.dax_val_to_string(type, val, index=test[2])
            self.assertEqual(result, test[3])

    def test_string_to_val_basic(self):
        """check the dax_string_to_val() function"""
        tests = [
                ("DAX_BYTE", "1", b'\x01'),
                ("DAX_BYTE", "128", b'\x80'),
                ("DAX_BYTE", "255", b'\xFF'),
                ("DAX_SINT", "-1", b'\xFF'),
                ("DAX_SINT", "-128", b'\x80'),
                ("DAX_SINT", "1", b'\x01'),
                ("DAX_SINT", "127", b'\x7F'),
                ("DAX_WORD", "0", b'\x00\x00'),
                ("DAX_WORD", "128", b'\x80\x00'),
                ("DAX_WORD", "255", b'\xFF\x00'),
                ("DAX_WORD", "256", b'\x00\x01'),
                ("DAX_WORD", "65535", b'\xFF\xFF'),
                ("DAX_UINT", "0", b'\x00\x00'),
                ("DAX_UINT", "128", b'\x80\x00'),
                ("DAX_UINT", "255", b'\xFF\x00'),
                ("DAX_UINT", "256", b'\x00\x01'),
                ("DAX_UINT", "65535", b'\xFF\xFF'),
                ("DAX_INT", "-1", b'\xFF\xFF'),
                ("DAX_INT", "-128", b'\x80\xFF'),
                ("DAX_INT", "128", b'\x80\x00'),
                ("DAX_INT", "255", b'\xFF\x00'),
                ("DAX_INT", "256", b'\x00\x01'),
                ("DAX_INT", "-32768", b'\x00\x80'),
                ("DAX_INT", "32767", b'\xFF\x7F'),
                ("DAX_DWORD", "128", b'\x80\x00\x00\x00'),
                ("DAX_DWORD", "0", b'\x00\x00\x00\x00'),
                ("DAX_DWORD", "255", b'\xFF\x00\x00\x00'),
                ("DAX_DWORD", "256", b'\x00\x01\x00\x00'),
                ("DAX_DWORD", "4294967295", b'\xFF\xFF\xFF\xFF'),
                ("DAX_UDINT", "128", b'\x80\x00\x00\x00'),
                ("DAX_UDINT", "0", b'\x00\x00\x00\x00'),
                ("DAX_UDINT", "255", b'\xFF\x00\x00\x00'),
                ("DAX_UDINT", "256", b'\x00\x01\x00\x00'),
                ("DAX_UDINT", "4294967295", b'\xFF\xFF\xFF\xFF'),
                ("DAX_DINT", "-1", b'\xFF\xFF\xFF\xFF'),
                ("DAX_DINT", "-128", b'\x80\xFF\xFF\xFF'),
                ("DAX_DINT", "128", b'\x80\x00\x00\x00'),
                ("DAX_DINT", "255", b'\xFF\x00\x00\x00'),
                ("DAX_DINT", "256", b'\x00\x01\x00\x00'),
                ("DAX_DINT", "-2147483648", b'\x00\x00\x00\x80'),
                ("DAX_DINT", "2147483647", b'\xFF\xFF\xFF\x7F'),
                ("DAX_LWORD", "128", b'\x80\x00\x00\x00\x00\x00\x00\x00'),
                ("DAX_LWORD", "0", b'\x00\x00\x00\x00\x00\x00\x00\x00'),
                ("DAX_LWORD", "255", b'\xFF\x00\x00\x00\x00\x00\x00\x00'),
                ("DAX_LWORD", "256", b'\x00\x01\x00\x00\x00\x00\x00\x00'),
                ("DAX_LWORD", "18446744073709551615", b'\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF'),
                ("DAX_ULINT", "128", b'\x80\x00\x00\x00\x00\x00\x00\x00'),
                ("DAX_ULINT", "0", b'\x00\x00\x00\x00\x00\x00\x00\x00'),
                ("DAX_ULINT", "255", b'\xFF\x00\x00\x00\x00\x00\x00\x00'),
                ("DAX_ULINT", "256", b'\x00\x01\x00\x00\x00\x00\x00\x00'),
                ("DAX_ULINT", "18446744073709551615", b'\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF'),
                ("DAX_LINT", "128", b'\x80\x00\x00\x00\x00\x00\x00\x00'),
                ("DAX_LINT", "0", b'\x00\x00\x00\x00\x00\x00\x00\x00'),
                ("DAX_LINT", "-1", b'\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF'),
                ("DAX_LINT", "-128", b'\x80\xFF\xFF\xFF\xFF\xFF\xFF\xFF'),
                ("DAX_LINT", "255", b'\xFF\x00\x00\x00\x00\x00\x00\x00'),
                ("DAX_LINT", "256", b'\x00\x01\x00\x00\x00\x00\x00\x00'),
                ("DAX_LINT", "-9223372036854775808", b'\x00\x00\x00\x00\x00\x00\x00\x80'),
                ("DAX_LINT", "9223372036854775807", b'\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x7F'),
                ("DAX_REAL", "0", b'\x00\x00\x00\x00'),
                ("DAX_REAL", "0.23", b'\x1F\x85\x6B\x3E'),
                ("DAX_REAL", "3.141592", b'\xD8\x0F\x49\x40'),
                ("DAX_REAL", "1e+08", b'\x20\xbc\xbe\x4c'),
                ("DAX_REAL", "3.123456e+23", b'\x94\x48\x84\x66'),
                ("DAX_REAL", "-3.123456e+23", b'\x94\x48\x84\xe6'),
                ("DAX_LREAL", "0", b'\x00\x00\x00\x00\x00\x00\x00\x00'),
                ("DAX_LREAL", "3.12345678911234e+23", b'\xe0\xdb\x7e\xc3\x12\x89\xd0\x44'),
                ("DAX_LREAL", "-3.12345678911234e+23", b'\xe0\xdb\x7e\xc3\x12\x89\xd0\xc4'),
                ("DAX_LREAL", "0.1", b'\x9a\x99\x99\x99\x99\x99\xb9\x3f'),
                ]
        for test in tests:
            datatype = daxwrapper.defines[test[0]]
            instr = test[1]
            check = test[2]
            # print(test)
            buff = bytes(len(check))
            mask = bytes(len(check))

            buff, mask = self.dax.dax_string_to_val(instr, datatype, buff, mask)
            self.assertEqual(buff, check)

    def test_string_to_val_underflow(self):
        """check the dax_string_to_val() function for underflow errors"""
        # These should result in an underflow error
        tests = [
                ("DAX_BYTE", "-1", b'\x00'),
                ("DAX_SINT", "-129", b'\x80'),
                ("DAX_WORD", "-1", b'\x00\x00'),
                ("DAX_UINT", "-1", b'\x00\x00'),
                ("DAX_INT", "-32769", b'\x00\x80'),
                ("DAX_DWORD", "-1", b'\x00\x00\x00\x00'),
                ("DAX_UDINT", "-1", b'\x00\x00\x00\x00'),
                ("DAX_DINT", "-2147483649", b'\x00\x00\x00\x80'),
                ("DAX_LWORD", "-1", b'\x00\x00\x00\x00\x00\x00\x00\x00'),
                ("DAX_ULINT", "-1", b'\x00\x00\x00\x00\x00\x00\x00\x00'),
                ("DAX_LINT", "-9223372036854775809", b'\x00\x00\x00\x00\x00\x00\x00\x80'),
                ]
        for test in tests:
            datatype = daxwrapper.defines[test[0]]
            instr = test[1]
            check = test[2]
            # print(test)
            buff = bytes(len(check))
            mask = bytes(len(check))
            with self.assertRaises(daxwrapper.Underflow):
                buff, mask = self.dax.dax_string_to_val(instr, datatype, buff, mask)
            self.assertEqual(buff, check)

    def test_string_to_val_overflow(self):
        """check the dax_string_to_val() function"""
        tests = [
                ("DAX_BYTE", "256", b'\xFF'),
                ("DAX_SINT", "128", b'\x7F'),
                ("DAX_WORD", "65536", b'\xFF\xFF'),
                ("DAX_UINT", "65536", b'\xFF\xFF'),
                ("DAX_INT", "32768", b'\xFF\x7F'),
                ("DAX_DWORD", "4294967296", b'\xFF\xFF\xFF\xFF'),
                ("DAX_UDINT", "4294967296", b'\xFF\xFF\xFF\xFF'),
                ("DAX_DINT", "2147483648", b'\xFF\xFF\xFF\x7F'),
                ("DAX_LWORD", "18446744073709551616", b'\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF'),
                ("DAX_ULINT", "18446744073709551616", b'\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF'),
                ("DAX_LINT", "9223372036854775808", b'\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x7F'),
                ]
        for test in tests:
            datatype = daxwrapper.defines[test[0]]
            instr = test[1]
            check = test[2]
            # print(test)
            buff = bytes(len(check))
            mask = bytes(len(check))
            with self.assertRaises(daxwrapper.Overflow):
                buff, mask = self.dax.dax_string_to_val(instr, datatype, buff, mask)
            self.assertEqual(buff, check)



if __name__ == '__main__':
    unittest.main()
