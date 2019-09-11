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
