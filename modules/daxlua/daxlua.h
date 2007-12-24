/*  OpenDAX - An open source data acquisition and control system
 *  Copyright (c) 2007 Phil Birkelbach
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *

 *  Header file for the Lua script interpreter
 */

#ifndef __DAXLUA_H
#define __DAXLUA_H

#include <common.h>

/* All this silliness is because the different distributions have the libraries
   and header files for lua in different places with different names.
   There has got to be a better way. */
#if defined(HAVE_LUA5_1_LUA_H)
  #include <lua5.1/lua.h>
#elif defined(HAVE_LUA51_LUA_H)
  #include <lua51/lua.h>
#elif defined(HAVE_LUA_LUA_H)
  #include <lua/lua.h>
#elif defined(HAVE_LUA_H)
  #include <lua.h>
#else
  #error Missing lua.h
#endif

#if defined(HAVE_LUA51_LAUXLIB_H)
  #include <lua51/lauxlib.h>
#elif defined(HAVE_LUA5_1_LAUXLIB_H)
  #include <lua5.1/lauxlib.h>
#elif defined(HAVE_LUA_LAUXLIB_H)
  #include <lua/lauxlib.h>
#elif defined(HAVE_LAUXLIB_H)
  #include <lauxlib.h>
#else
  #error Missing lauxlib.h
#endif

#if defined(HAVE_LUA51_LUALIB_H)
  #include <lua51/lualib.h>
#elif defined(HAVE_LUA5_1_LUALIB_H)
  #include <lua5.1/lualib.h>
#elif defined(HAVE_LUA_LUALIB_H)
  #include <lua/lualib.h>
#elif defined(HAVE_LUALIB_H)
  #include <lualib.h>
#else
  #error Missing lualib.h
#endif 

#endif /* !__DAXLUA_H */
