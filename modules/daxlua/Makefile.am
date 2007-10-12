if HAVE_LUA
  LIBLUA = -llua
else
  if HAVE_LUA51
    LIBLUA = -llua5.1
  else
    if HAVE_LUA50
	  LIBLUA = -llua50
	else
	  LIBLUA =
	endif
  endif
endif

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
daxlua_LDADD =  ../../lib/libdax.la $(LIBLUA) $(LIBLUALIB)

