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

# This class uses ctypes to wrap the modbus library for testing
from ctypes import *
import testconfig

class ModbusWrapper:
    def __init__(self):
        self.libmodbus = cdll.LoadLibrary(testconfig.libmodbus_file)
        self.retval = None

    def mb_new_port(self, name, flags):
        name = name.encode('utf-8')
        func = self.libmodbus.mb_new_port
        func.restype = c_void_p
        return func(name, flags)

    def mb_destroy_port(self, port):
        self.libmodbus.mb_destroy_port(port)

    def mb_set_serial_port(self, port, device, baudrate, databits, parity, stopbits):
        device = device.encode('utf-8')
        return self.libmodbus.mb_set_serial_port(port, device, baudrate, databits, parity, stopbits)

    def mb_set_network_port(self, port, ipaddress, bindport, socket):
        ipaddress = ipaddress.encode('utf-8')
        return self.libmodbus.mb_set_network_port(port, ipaddress, bindport, socket)

    def mb_set_protocol(self, port, type, protocol, slaveid):
        return self.libmodbus.mb_set_protocol(port, type, protocol, slaveid)

    def mb_set_frame_time(self, port, frame):
        return self.libmodbus.mb_set_frame_time(port, frame)

    def mb_set_delay_time(self, port, delay):
        return self.libmodbus.mb_set_delay_time(port, delay)

    def mb_set_scan_rate(self, port, rate):
        return self.libmodbus.mb_set_scan_rate(port, rate)

    def mb_set_timeout(self, port, timeout):
        return self.libmodbus.mb_set_teimout(timeout)

    def mb_set_retries(self, port, retries):
        return self.libmodbus.mb_set_retries(port, retries)

    def mb_set_maxfailures(self, port, maxfailures, inhibit):
        return self.libmodbus.mb_set_maxfailures(port, maxfailures, inhibit)


    def mb_open_port(self, port):
        return self.libmodbus.mb_open_port(port)

    def mb_close_port(self, port):
        return self.libmodbus.mb_close_port(port)

    def mb_new_cmd(self, port):
        func = self.libmodbus.mb_new_cmd
        func.restype = c_void_p
        return func(port)

    def mb_destroy_cmd(self, cmd):
        self.libmodbus.mb_destroy_cmd(cmd)

    def mb_disable_cmd(self, cmd):
        self.libmodbus.mb_disable_cmd(cmd)

    def mb_enable_cmd(self, cmd):
        self.libmodbus.mb_enable_cmd(cmd)

    def mb_set_command(self, cmd, node, function, reg, length):
        return self.libmodbus.mb_set_command(cmd, node, function, reg, length)

    def mb_get_cmd_data(self, cmd):
        func = self.libmodbus.mb_get_cmd_data
        func.restype = POINTER(c_uint8)
        return func(cmd)

    def mb_get_cmd_datasize(self, cmd):
        return self.libmodbus.mb_get_cmd_datasize(cmd)

    def mb_send_command(self, port, cmd):
        # We store the retval in the object so that we can get it if we run
        # this command in a thread
        self.retval = self.libmodbus.mb_send_command(port, cmd)
        return self.retval
