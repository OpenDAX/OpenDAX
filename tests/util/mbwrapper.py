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

class ModbusWrapper:
    def __init__(self):
        self.libmodbus = cdll.LoadLibrary("src/modules/modbus/lib/.libs/libmodbus.so")

    def mb_new_port(self, name, flags):
        name = name.encode('utf-8')
        func = self.libmodbus.mb_new_port
        func.restype = c_void_p
        return func(name, flags)
