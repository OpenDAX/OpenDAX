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

# Test that the handles returned are correct
# Each tag entry contains...
#     the tagname string to parse
#     item count  (N) - requested by test
#  Returned Values
#     byte offset (B)
#     bit offset  (b)
#     item count  (c) - returned by opendax
#     byte size   (s)
#     data type

# These tests should pass
#                    NAME                          N   B   b  c   s   TYPE
handleTestsPass = [("HandleBool1",                 0,  0,  0, 1,  1,  "BOOL"),
                   ("HandleBool7",                 0,  0,  0, 7,  1,  "BOOL"),
                   ("HandleBool7[2]",              4,  0,  2, 4,  1,  "BOOL"),
                   ("HandleBool7[6]",              1,  0,  6, 1,  1,  "BOOL"),
                   ("HandleBool7[5]",              2,  0,  5, 2,  1,  "BOOL"),
                   ("HandleBool8",                 0,  0,  0, 8,  1,  "BOOL"),
                   ("HandleBool9",                 0,  0,  0, 9,  2,  "BOOL"),
                   ("HandleBool9[8]",              0,  1,  0, 1,  1,  "BOOL"),
                   ("HandleBool33",                0,  0,  0, 33, 5,  "BOOL"),
                   ("HandleBool33",                8,  0,  0, 8,  1,  "BOOL"),
                   ("HandleBool33[7]",             1,  0,  7, 1,  1,  "BOOL"),
                   ("HandleBool33[3]",             8,  0,  3, 8,  2,  "BOOL"),
                   ("HandleBool33[3]",             9,  0,  3, 9,  2,  "BOOL"),
                   ("HandleInt2[1]",               1,  2,  0, 1,  2,  "INT"),
                   ("HandleInt2",                  0,  0,  0, 2,  4,  "INT"),
                   ("HandleInt",                   1,  0,  0, 1,  2,  "INT"),
                   ("HandleInt32",                 1,  0,  0, 1,  2,  "INT"),
                   ("HandleInt32",                 0,  0,  0, 32, 64, "INT"),
                   ("HandleTest1",                 0,  0,  0, 1,  28, "Test1"),
                   ("HandleTest1.Dint3",           0,  16, 0, 3,  12, "DINT"),
                   ("HandleTest1.Dint3[0]",        2,  16, 0, 2,  8,  "DINT"),
                   ("HandleTest1.Dint3[1]",        2,  20, 0, 2,  8,  "DINT"),
                   ("HandleTest1.Dint3[2]",        1,  24, 0, 1,  4,  "DINT"),
                   ("HandleTest1.Dint1",           1,  12, 0, 1,  4,  "DINT"),
                   ("HandleTest2[0].Test1[2]",     1,  62, 0, 1,  28, "Test1"),
                   ("HandleTest2[0].Test1[1]",     2,  34, 0, 2,  56, "Test1"),
                   ("HandleTest2[0].Test1[4]",     1,  118,0, 1,  28, "Test1"),
                   ("HandleTest2[1].Test1",        0,  152,0, 5,  140,"Test1"),
                   ("HandleTest2[0].Test1[0].Bool10[4]", 1,  16, 4, 1,  1,  "BOOL"),
                   ("HandleTest2[0].Test1[0].Bool10",    0,  16, 0, 10, 2,  "BOOL"),
                   ("HandleTest2[0].Test1[1].Bool10",    0,  44, 0, 10, 2,  "BOOL"),
                   ("HandleTest2[4].Test1[0].Bool10",    0,  600,0, 10, 2,  "BOOL"),
                   ("HandleTestIV.Bool",           0,  2,  0, 1,  1,  "BOOL"),
                   ("HandleTestIV.reBool",         0,  2,  1, 1,  1,  "BOOL"),
                   ("HandleTestIV.triBool",        0,  2,  2, 1,  1,  "BOOL"),
                   ("HandleTest5",                 0,  0,  0, 1,  2,  "Test5"),
                   ("HandleTest5.Bool1",           0,  0,  0, 10, 2,  "BOOL"),
                   ("HandleTest5.Bool1[1]",        0,  0,  1, 1,  1,  "BOOL"),
                   ("HandleTest5.Bool1[9]",        0,  1,  1, 1,  1,  "BOOL"),
                   ("HandleTest5.Bool2",           0,  1,  2, 1,  1,  "BOOL"),
                   ("HandleTest5.Bool3",           0,  1,  3, 3,  1,  "BOOL"),
                   ("HandleTest5.Bool3[0]",        0,  1,  3, 1,  1,  "BOOL"),
                   ("HandleTest5.Bool3[2]",        0,  1,  5, 1,  1,  "BOOL"),
                   ("HandleInt.0",                 0,  0,  0, 1,  1,  "BOOL"),
                   ("HandleInt.5",                 0,  0,  5, 1,  1,  "BOOL"),
                   ("HandleInt.12",                0,  1,  4, 1,  1,  "BOOL"),
                   ("HandleInt.12",                4,  1,  4, 4,  1,  "BOOL"),
                   ("HandleInt.15",                0,  1,  7, 1,  1,  "BOOL"),
                   ("HandleTest1.Int5[0].5",          0,  0,  5, 1,  1,  "BOOL"),# Should these fail because of an ungiven index?
                   ("HandleTest1.Dint3[0].5",         0,  16, 5, 1,  1,  "BOOL"),
                   ("HandleTest1.Dint3[0].15",        0,  17, 7, 1,  1, "BOOL")]

# These tests should fail
#                    NAME                          N   B   b  c   s   TYPE
handleTestsFail = [("HandleBool7[6]",              2,  0,  6, 2,  1,  "BOOL"),
                   ("HandleBool7[7]",              1,  0,  2, 4,  1,  "BOOL"),
                   ("HandleBool33[3a]",            1,  0,  0, 0,  0,  "BOOL"),
                   ("HandleInt[2]",                1,  0,  0, 0,  0,  "INT"),
                   ("HandleInt[2]",                5,  0,  0, 0,  0,  "INT"),
                   ("HandleTest2[0].Test1[1]",     5,  32, 0, 1,  28, "Test1"),
                   ("HandleTest1.Dint3[2]",        2,  24, 0, 1,  4,  "DINT"),
                   ("HandleTest2[0].NotAMember",   0,  0,  0, 0,  0,  "Yup"),
                   ("NoTagName",                   0,  0,  0, 0,  0,  "Duh"),
                   ("HandleInt.16",                0,  1,  4, 1,  1,  "BOOL"),
                   ("HandleInt.15",                2,  1,  4, 1,  1,  "BOOL"),
                   ("HandleInt32[0].15",           2,  1,  4, 1,  1,  "BOOL"),
                   ("HandleInt.6e",                1,  1,  4, 1,  1,  "BOOL"),
                   ("HandleInt2.7",                2,  1,  4, 1,  1,  "BOOL"),
                   ("HandleTest1.Dint3.5",         0,  16, 5, 1,  1,  "BOOL"),
                   ("",                            0,  0,  0, 0,  0,  "Yup")]


class TestHandles_LoopTests(unittest.TestCase):
    """Tests all of the above handles in a couple of loop tests.  It would be
       better if these tests were flattened out, but these will work for now"""
    def setUp(self):
        self.server = subprocess.Popen([testconfig.tagserver_file,
                                        "-C",
                                        "tests/config/tagserver_basic.conf"],
#                                        stdout=subprocess.DEVNULL
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

        members = [("Int3", "INT", 3),     #   6
                   ("Test1", "Test1", 5)]  # 140
                                           # 146 Total
        test2 = self.dax.dax_add_cdt(self.ds, "Test2", members)

        members = [("Int7", "INT", 7),      #  14
                   ("Bool30", "BOOL", 30),  #   1
                   ("Bool32", "BOOL", 32),  #   1
                   ("Test1", "Test1", 1),   #  28
                   ("Test2", "Test2", 2)]   # 292
                                            # 336 Total

        test3 = self.dax.dax_add_cdt(self.ds, "Test3", members)

        members = [("Int", "INT", 1),
                   ("Bool",  "BOOL",  1),
                   ("reBool", "BOOL", 1),
                   ("triBool", "BOOL",1),
                   ("Dint",  "DINT",  1)]

        test4 = self.dax.dax_add_cdt(self.ds, "Test4", members)

        members = [("Bool1", "BOOL", 10),
                   ("Bool2",  "BOOL",  1),
                   ("Bool3", "BOOL", 3)]

        test5 = self.dax.dax_add_cdt(self.ds, "Test5", members)

        self.dax.dax_tag_add(self.ds, "HandleBool1", "BOOL", 1)
        self.dax.dax_tag_add(self.ds, "HandleBool7", "BOOL", 7)
        self.dax.dax_tag_add(self.ds, "HandleBool8", "BOOL", 8)
        self.dax.dax_tag_add(self.ds, "HandleBool9", "BOOL", 9)
        self.dax.dax_tag_add(self.ds, "HandleBool33", "BOOL", 33)
        self.dax.dax_tag_add(self.ds, "HandleInt", "INT", 1)
        self.dax.dax_tag_add(self.ds, "HandleInt2", "INT", 2)
        self.dax.dax_tag_add(self.ds, "HandleInt32", "INT", 32)
        self.dax.dax_tag_add(self.ds, "HandleReal", "REAL", 1)
        self.dax.dax_tag_add(self.ds, "HandleReal16", "REAL", 16)
        self.dax.dax_tag_add(self.ds, "HandleTest1", "Test1", 1)
        self.dax.dax_tag_add(self.ds, "HandleTest2", test2, 5)
        self.dax.dax_tag_add(self.ds, "HandleTest3", test3, 1)
        self.dax.dax_tag_add(self.ds, "HandleTest4", test3, 10)
        self.dax.dax_tag_add(self.ds, "HandleTestIV", test4, 1)
        self.dax.dax_tag_add(self.ds, "HandleTest5", test5, 1)
        self.dax.dax_tag_add(self.ds, "HandleTest5_10", test5, 10)



    def tearDown(self):
        x = self.dax.dax_disconnect(self.ds)
        self.server.terminate()
        self.server.wait()

    def test_handles_pass(self):
        for test in handleTestsPass:
            # Just for convenience
            name = test[0]
            N = test[1]
            B = test[2]
            b = test[3]
            c = test[4]
            s = test[5]
            if isinstance(test[6], str):
                dt = self.dax.dax_string_to_type(self.ds, test[6])
            else:
                dt = test[6]
            h = self.dax.dax_tag_handle(self.ds, name, N)
            self.assertEqual(B, h.byte)
            self.assertEqual(b, h.bit)
            self.assertEqual(c, h.count)
            self.assertEqual(s, h.size)
            self.assertEqual(dt, h.type)

    def test_handles_fail(self):
        for test in handleTestsFail:
            # Just for convenience
            name = test[0]
            N = test[1]
            B = test[2]
            b = test[3]
            c = test[4]
            s = test[5]
            dt = test[6]
            with self.assertRaises(RuntimeError):
                h = self.dax.dax_tag_handle(self.ds, name, N)

class TestHandles_FlatTests(unittest.TestCase):
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

    def test_handels_wrong_type(self):
        members = [("Int5", "INT", 5),     # 10
                   ("Bool10", "BOOL", 10), #  2
                   ("Dint1", "DINT", 1),   #  4
                   ("Dint3", "DINT", 3)]   # 12
                                           # 28 Total
        test1 = self.dax.dax_add_cdt(self.ds, "Test1", members)

        members = [("Int3", "INT", 3),     #   6
                   ("Test1", "Test1", 5)]  # 140
                                           # 146 Total
        test2 = self.dax.dax_add_cdt(self.ds, "Test2", members)

        self.dax.dax_tag_add(self.ds, "HandleBool8", "BOOL", 8)
        self.dax.dax_tag_add(self.ds, "HandleReal", "REAL", 1)
        self.dax.dax_tag_add(self.ds, "HandleReal16", "REAL", 16)
        self.dax.dax_tag_add(self.ds, "HandleTest2", test2, 5)
        with self.assertRaises(RuntimeError):
            h = self.dax.dax_tag_handle(self.ds, "HandleBool8.3")
        with self.assertRaises(RuntimeError):
            h = self.dax.dax_tag_handle(self.ds, "HandleReal.3")
        with self.assertRaises(RuntimeError):
            h = self.dax.dax_tag_handle(self.ds, "HandleReal16[2].3")
        with self.assertRaises(RuntimeError):
            h = self.dax.dax_tag_handle(self.ds, "HandleTest2[0].Test1[0].4")




if __name__ == '__main__':
    unittest.main()
