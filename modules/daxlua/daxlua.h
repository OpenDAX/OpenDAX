#include <config.h>

#if defined(HAVE_LUA50_LUA_H)
  #include <lua50/lua.h>
#elif defined(HAVE_LUA5_1_LUA_H)
  #include <lua5.1/lua.h>
#elif defined(HAVE_LUA_LUA_H)
  #include <lua/lua.h>
#elif defined(HAVE_LUA_H)
  #include <lua.h>
#else
  #error Missing lua.h
#endif

#if defined(HAVE_LUA50_LAUXLIB_H)
  #include <lua50/lauxlib.h>
#elif defined(HAVE_LUA5_1_LAUXLIB_H)
  #include <lua5.1/lauxlib.h>
#elif defined(HAVE_LUA_LAUXLIB_H)
  #include <lua/lauxlib.h>
#elif defined(HAVE_LAUXLIB_H)
  #include <lauxlib.h>
#else
  #error Missing lauxlib.h
#endif

#if defined(HAVE_LUA50_LUALIB_H)
  #include <lua50/lualib.h>
#elif defined(HAVE_LUA5_1_LUALIB_H)
  #include <lua5.1/lualib.h>
#elif defined(HAVE_LUA_LUALIB_H)
  #include <lua/lualib.h>
#elif defined(HAVE_LUALIB_H)
  #include <lualib.h>
#else
  #error Missing lualib.h
#endif 
