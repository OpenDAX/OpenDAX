bin_PROGRAMS = daxtest badmodule
daxtest_SOURCES = daxtest.c tags.c daxtest.h testlua.c
#daxtest_LDADD = ../../src/func.o @LUALIB@ ../../lib/libdax.la ../../lib/lua/libdaxlua.la
daxtest_LDADD = @LUALIB@ ../../lib/libdax.la ../../lib/lua/libdaxlua.la
badmodule_SOURCES = badmodule.c badmodule.h
badmodule_LDADD = ../../lib/libdax.la
