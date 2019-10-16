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
import tests.util as util

defines = util.read_defines("src/opendax.h")

class Underflow(Exception):
    pass

class Overflow(Exception):
    pass

class Handle(Structure):
    _fields_ = [
        ("index", c_uint),
        ("byte", c_uint),
        ("bit", c_ubyte),
        ("count", c_uint),
        ("size", c_uint),
        ("type", c_uint)]

class DaxTag(Structure):
    _fields_ = [
        ("index", c_uint),
        ("type", c_uint),
        ("count", c_uint),
        ("name", ARRAY(c_char, 33))]

def typesize(datatype):
    return (0x0001 << (datatype & 0x0F))


class LibDaxWrapper:
    def __init__(self):
        self.libdax = cdll.LoadLibrary("src/lib/.libs/libdax.so")

    def dax_init(self, name):
        self.name = name.encode('utf-8')
        dax_init = self.libdax.dax_init
        dax_init.restype = c_void_p
        return dax_init(name)

    def dax_init_config(self, ds, name):
        return self.libdax.dax_init_config(ds, name.encode('utf-8'))

    def dax_configure(self, ds, argv, flags):
        arr = (c_char_p * (len(argv)+1))()
        idx = 0
        for each in argv:
            arr[idx] = each.encode('utf-8')
            idx += 1
        arr[idx] = None
        return self.libdax.dax_configure(ds, len(argv), arr, flags)

    def dax_connect(self, ds):
        return self.libdax.dax_connect(ds)

    def dax_disconnect(self, ds):
        return self.libdax.dax_disconnect(ds)

    def dax_tag_byname(self, ds, name):
        t = DaxTag()
        x = self.libdax.dax_tag_byname(ds, byref(t), name.encode('utf-8'))
        if x < 0:
            raise RuntimeError
        return t

    def dax_tag_handle(self, ds, name, count=0):
        h = Handle()
        x = self.libdax.dax_tag_handle(ds, byref(h), name.encode('utf-8'), count)
        if x < 0:
            raise RuntimeError
        return h

    def dax_read_tag(self, ds, h):
        b = bytearray(h.size)
        data = c_char * len(b)
        x = self.libdax.dax_read_tag(ds, h, data.from_buffer(b))
        if x < 0:
            raise RuntimeError
        return b

    def dax_write_tag(self, ds, h, b):
        data = c_char * len(b)
        x = self.libdax.dax_write_tag(ds, h, b)
        if x < 0:
            raise RuntimeError
        return b

    def dax_tag_add(self, ds, name, datatype, count=1):
        h = Handle()
        x = self.libdax.dax_tag_add(ds, byref(h), name.encode('utf-8'), datatype, count)
        if x < 0:
            raise RuntimeError
        return h

    def dax_map_add(self, ds, src, dest):
        x = self.libdax.dax_map_add(ds, byref(src), byref(dest), 0)
        if x < 0:
            raise RuntimeError

    def dax_val_to_string(self, datatype, val, size=32, index=0):
        data = c_char_p()
        data.value = b""
        x = self.libdax.dax_val_to_string(data, size, datatype, val, index)
        return data.value.decode('utf-8')

    def dax_string_to_val(self, instr, datatype, buff=None, mask=None, index=0):
        if datatype == defines["DAX_BOOL"]:
            if buff is None:
                buff = bytes(typesize(datatype)//8+1+(index//8))
            if mask is None:
                mask = bytes(typesize(datatype)//8+1+(index//8))
        else:
            if buff is None:
                buff = bytes(typesize(datatype)//8*(index+1))
            if mask is None:
                mask = bytes(typesize(datatype)//8*(index+1))
        result = self.libdax.dax_string_to_val(instr.encode(), datatype, buff, mask, index)
        if result == defines["ERR_UNDERFLOW"]:
            raise Underflow
        if result == defines["ERR_OVERFLOW"]:
            raise Overflow
        return buff, mask
