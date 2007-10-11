if HAVE_READLINE
   READLINE = -lreadline
else
   READLINE = 
endif

bin_PROGRAMS = daxc
daxc_SOURCES = daxc.c func.c func.h
daxc_LDADD =  ../../lib/libdax.la $(READLINE)
#daxc_LDADD =  ../../lib/libdax.la

