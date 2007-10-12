#include <daxlua.h>
#include <stdio.h>
#include <stdlib.h>

/* Function to expose to lua interpreter */
static int l_dummy(lua_State *L) {
  double d = lua_tonumber(L,1); /* Get the argument off the stack */
  lua_pushnumber(L,d*2); /* put the return value/s on the stack */
  return 1; /* return number of retvals */
}

int main(int argc, char *argv[]) {
  int status,func_ref;
  lua_State *L;
  /* Create a lua interpreter object */
  L = lua_open();
  /* send the l_dummy() function to the lua interpreter */
  lua_pushcfunction(L,l_dummy);
  lua_setglobal(L,"dummy");
  /* register the libraries */
  luaL_openlibs(L);
  /* load and compile the file */
  if(status = luaL_loadfile(L, "script.lua")) {
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
  if(status = lua_pcall(L, 0, 0, 0)) {
    printf("--bad, bad script\n");
  }
  
  lua_rawgeti(L, LUA_REGISTRYINDEX, func_ref);
  if(status = lua_pcall(L, 0, 0, 0)) {
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
