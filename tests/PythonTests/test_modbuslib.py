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
import signal
import time
import threading
from serial import Serial
import PythonTests.util as util
import PythonTests.util.mbwrapper as mbwrapper

defines = util.read_defines("src/modules/modbus/lib/modbus.h")

class TestRTUMaster(unittest.TestCase):
    def setUp(self):
        # We start socat to give us a couple of ptys to use as our serial ports
        self.socat = subprocess.Popen(["socat",
                                       #"-d","-d"
                                       "pty,raw,echo=0,link=/tmp/serial1",
                                       "pty,raw,echo=0,link=/tmp/serial2"])
        self.master = "/tmp/serial1"
        self.slave = "/tmp/serial2"
        time.sleep(0.1)
        x = self.socat.poll()
        self.assertIsNone(x)
        self.mb = mbwrapper.ModbusWrapper()

    def tearDown(self):
        self.socat.terminate()
        self.socat.wait()

    def openRTUMasterPort(self):
        port = self.mb.mb_new_port("test", 0)
        x = self.mb.mb_set_serial_port(port, self.master, 9600, 8, defines["MB_NONE"], 1)
        self.assertEqual(x, 0)
        x = self.mb.mb_set_protocol(port, defines["MB_MASTER"], defines["MB_RTU"], 1)
        self.assertEqual(x, 0)
        x = self.mb.mb_open_port(port)
        self.assertEqual(x, 0)
        return port

    def closeRTUMasterPort(self, port):
        x = self.mb.mb_close_port(port)
        self.assertEqual(x, 0)
        self.mb.mb_destroy_port(port)

    def test_modbus_RTU_master_connect(self):
        """Initialize, connect and disconnect from the serialport"""
        port = self.openRTUMasterPort()
        self.closeRTUMasterPort(port)

    def test_modbus_RTU_master_good_reads(self):
        """Test RTU Messages that Should Succeed"""
        test_cases = [
            {"fc":1, "node":1, "reg":0, "len":1, "data":[0], "bytes":1},
            {"fc":1, "node":1, "reg":0, "len":1, "data":[1], "bytes":1},
            {"fc":1, "node":1, "reg":0, "len":8, "data":[0], "bytes":1},
            {"fc":1, "node":1, "reg":0, "len":8, "data":[0xAA], "bytes":1},
            {"fc":1, "node":1, "reg":0, "len":9, "data":[0, 0], "bytes":2},
            {"fc":1, "node":1, "reg":0, "len":9, "data":[1, 0x55], "bytes":2},
            {"fc":2, "node":247, "reg":0, "len":1, "data":[0], "bytes":1},
            {"fc":2, "node":247, "reg":0, "len":1, "data":[1], "bytes":1},
            {"fc":2, "node":247, "reg":0, "len":8, "data":[0], "bytes":1},
            {"fc":2, "node":247, "reg":0, "len":8, "data":[0xAA], "bytes":1},
            {"fc":2, "node":247, "reg":0, "len":9, "data":[0, 0], "bytes":2},
            {"fc":2, "node":247, "reg":0, "len":9, "data":[1, 0x55], "bytes":2},
            {"fc":2, "node":247, "reg":0, "len":2000, "data":[0x55]*250, "bytes":250},
            {"fc":3, "node":2, "reg":0, "len":1, "data":[1200], "bytes":2},
            {"fc":3, "node":2, "reg":0, "len":4, "data":[1200, 0, 34000, 10], "bytes":8},
            {"fc":3, "node":2, "reg":0, "len":5, "data":[0x1234, 65535, 12, 100, 200], "bytes":10},
            {"fc":3, "node":2, "reg":0, "len":125, "data":[0xAA55]*125, "bytes":250},
            {"fc":4, "node":2, "reg":0, "len":1, "data":[1200], "bytes":2},
            {"fc":4, "node":2, "reg":0, "len":4, "data":[1200, 0, 34000, 10], "bytes":8},
            {"fc":4, "node":2, "reg":0, "len":5, "data":[0x1234, 65535, 12, 100, 200], "bytes":10},
            {"fc":4, "node":2, "reg":0, "len":125, "data":[0x1234]*125, "bytes":250},
        ]
        port = self.openRTUMasterPort()
        ser = Serial(self.slave, 9600, timeout=0.1)
        cmd = self.mb.mb_new_cmd(port)
        for tc in test_cases:
            #print(tc)
            node = tc["node"]
            fc = tc["fc"]
            reg = tc["reg"]
            count = tc["len"]
            data = tc["data"]
            x = self.mb.mb_set_command(cmd, node, fc, reg, count)
            self.assertEqual(x, 0)
            t = threading.Thread(target=self.mb.mb_send_command, args=[port, cmd])
            t.start()
            x = ser.read(8)
            self.assertIsNotNone(util.validatecrc(x))
            self.assertEqual(x[:-2], bytearray([node, fc, reg>>8, reg & 0xFF, count>>8, count & 0xFF]))
            util.sendRTUResponse(ser, fc, node, count, data=data)
            t.join() # Wait on send_command to return
            x = 0
            if fc in [0x01, 0x02]:
                cmd_data = self.mb.mb_get_cmd_data(cmd)
                for each in data:
                    self.assertEqual(cmd_data[x], each)
                    x += 1
            elif fc in [0x03, 0x04]:
                cmd_data = self.mb.mb_get_cmd_data(cmd)
                for x in range(count):
                    num = cmd_data[x*2] + (cmd_data[x*2+1] << 8)
                    self.assertEqual(data[x], num)
            self.assertEqual(self.mb.retval, 5+tc["bytes"])

        self.closeRTUMasterPort(port)

    def test_modbus_invalid_node_numbers(self):
        """Test that we get a proper error when we try to assign illegal node numbers to commands"""
        port = self.openRTUMasterPort()
        # ser = Serial(self.slave, 9600, timeout=0.1)
        cmd = self.mb.mb_new_cmd(port)
        for node in [0, 248, 249, 250, 251, 252, 253, 254, 255]:
            x = self.mb.mb_set_command(cmd, node, 3, 0, 1)
            self.assertEqual(x, defines["MB_ERR_BAD_ARG"])

    def test_modbus_invalid_register_counts(self):
        """Test that we get a proper error when we try to use illegal lengths"""
        port = self.openRTUMasterPort()
        # ser = Serial(self.slave, 9600, timeout=0.1)
        cmd = self.mb.mb_new_cmd(port)
        for fc in [1, 2, 15]:
            x = self.mb.mb_set_command(cmd, 1, fc, 0, 0)
            self.assertEqual(x, defines["MB_ERR_BAD_ARG"])
            x = self.mb.mb_set_command(cmd, 1, fc, 0, 2001)
            self.assertEqual(x, defines["MB_ERR_BAD_ARG"])
        for fc in [3, 4, 16]:
            x = self.mb.mb_set_command(cmd, 1, fc, 0, 0)
            self.assertEqual(x, defines["MB_ERR_BAD_ARG"])
            x = self.mb.mb_set_command(cmd, 1, fc, 0, 126)
            self.assertEqual(x, defines["MB_ERR_BAD_ARG"])




if __name__ == '__main__':
    unittest.main()
