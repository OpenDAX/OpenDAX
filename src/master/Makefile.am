bin_PROGRAMS =  opendax
opendax_SOURCES = master.c process.c process.h daemon.c daemon.h \
    mstr_config.c mstr_config.h logger.c logger.h
opendax_LDADD = -lpthread @LUALIB@ @UTILLIB@
opendax_DEPENDENCIES = ../common.h
