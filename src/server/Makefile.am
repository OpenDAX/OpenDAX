bin_PROGRAMS = tagserver
tagserver_SOURCES = server.c options.c options.h \
    func.c func.h module.c module.h\
    message.c message.h tagbase.c tagbase.h \
    crc.c crc.h daxtypes.h ../libcommon.h buffer.c events.c
#opendax_LDFLAGS = -lpthread
tagserver_LDADD = -lpthread @LUALIB@
tagserver_DEPENDENCIES = ../common.h
