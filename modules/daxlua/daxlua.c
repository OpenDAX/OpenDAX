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

 * Main source code file for the Lua script interpreter
 */

#include <daxlua.h>
#include <opendax.h>
#include <common.h>
#include <options.h>

/* Function to expose to lua interpreter */
static int l_dummy(lua_State *L) {
  double d = lua_tonumber(L,1); /* Get the argument off the stack */
  lua_pushnumber(L,d*2); /* put the return value/s on the stack */
  return 1; /* return number of retvals */
}

int main(int argc, char *argv[]) {
  int status,func_ref;
  lua_State *L;
  
  configure(argc,argv);
  
  /* Create a lua interpreter object */
  L = lua_open();
  /* send the l_dummy() function to the lua interpreter */
  lua_pushcfunction(L,l_dummy);
  lua_setglobal(L,"dummy");
  /* register the libraries */
  luaL_openlibs(L);
  /* load and compile the file */
  if((status = luaL_loadfile(L, "script.lua"))) {
    printf("bad, bad file\n");
    exit(1);
  }
  /* Basicaly stores the function */
  func_ref = luaL_ref(L, LUA_REGISTRYINDEX);
  /* retrieve the funciton and call it */
  lua_rawgeti(L, LUA_REGISTRYINDEX, func_ref);
  if (lua_pcall(L, 0, 0, 0)) {
    fprintf(stderr, "%s\n", lua_tostring(L, -1));
  }
  
  printf("This line from C\n");
  /* run the lua program */
  lua_rawgeti(L, LUA_REGISTRYINDEX, func_ref);
  if((status = lua_pcall(L, 0, 0, 0))) {
    printf("--bad, bad script\n");
  }
  printf("\nBack to C again\n");
  
  /* tell lua to push these variables onto the stack */
  lua_getglobal(L, "width");
  lua_getglobal(L, "height");
  /* check that the variables pushed onto the stack are numbers */
  if(!lua_isnumber(L,-2))
    luaL_error(L,"width should be a number\n");
  if(!lua_isnumber(L,-1))
    luaL_error(L,"height should be a number\n");
  /* print the globals that we got */
  printf("width = %g, height = %g\n",lua_tonumber(L,-2),lua_tonumber(L,-1));

  /* Clean up and get out */
  lua_close(L);

  exit(0);
}
