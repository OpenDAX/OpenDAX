#  PyDAX - A Python extension module for OpenDAX
#  OpenDAX - An open source data acquisition and control system
#  Copyright (c) 2011 Phil Birkelbach
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

#
#  This is the setup script for the extension module

# from distutils.core import setup, Extension
from setuptools import setup, Extension

PackageName = 'PyDAX'
Description = 'Python Interface Module to the OpenDAX API'
Version = '0.6'
SourceFiles = ['pydax.c']
IncludeDirs = ['.']
Libraries = ['dax']

pydax = Extension('pydax',
                  include_dirs = IncludeDirs,
                  libraries = Libraries,
                  sources = SourceFiles)

setup(name = PackageName, version = Version,
      description = Description,
      ext_modules = [pydax])
