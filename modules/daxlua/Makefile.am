#if HAVE_LUA
#  LIBLUA = -llua
#else
#  LIBLUA =
#  if HAVE_LUA51
#    LIBLUA51 = -llua5.1
#  else
#    LIBLUA51 = 
#    if HAVE_LUA50
#      LIBLUA50 = -llua50
#    else
#      LIBLUA50 =
#    endif
#  endif
#endif

#if HAVE_LUA50
#  LIBLUA = -llua50
#else
#  LIBLUA = 
#endif

#if HAVE_LUA51
#  LIBLUA = -llua5.1
#else
#  LIBLUA = 
#endif

#if HAVE_LUALIB
#  LIBLUALIB = -llualib
#else
#  LIBLUALIB = 
#endif

#if HAVE_LUALIB50
#  LIBLUALIB = -llualib50
#else
#  LIBLUALIB = 
#endif

#if HAVE_LUALIB51
#  LIBLUALIB = -llualib5.1
#else
#  LIBLUALIB = 
#endif

bin_PROGRAMS = daxlua
daxlua_SOURCES = daxlua.c daxlua.h
daxlua_LDADD =  ../../lib/libdax.la @LUALIB@

