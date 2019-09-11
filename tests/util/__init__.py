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

#  This file contains functions and classes that are helpful for testing


def read_defines(file):
    """read the given file, find all the #define lines and return the
       defines as a dictionary.  It will also attempt to convert them
       to an int or float if possible."""
    defines = {}
    with open(file) as f:
        lines = f.readlines()
        for line in lines:
            if line.startswith("#define"):
                tokens = line.split()
                if tokens[0] == "#define":
                    if len(tokens) > 2:
                        try: # try to convert it to an int
                            tokens[2] = int(tokens[2])
                        except:
                            pass # just ignore it if it doesn't work
                        if isinstance(tokens[2], str):
                            try: # try to convert it to an float
                                tokens[2] = float(tokens[2])
                            except:
                                pass # just ignore it if it doesn't work
                        defines[tokens[1]] = tokens[2]
    return defines
