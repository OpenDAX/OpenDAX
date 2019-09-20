ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = foreign
SUBDIRS = src tests

AUX_DIST = $(ac_aux_dir)/install-sh

EXTRA_DIST = src/common.h \
			 config/config.* \
			 config/depcomp \
			 config/install-sh \
			 config/ltmain.sh \
			 config/missing \
			 config/mkinstalldirs \
			 src/modules/test/daxtest.lua \
			 src/modules/test/tests/* \
			 src/modules/daxlua/*.lua \
			 etc/*.conf.sample \
			 doc/*.tex \
			 src/modules/arduino/sketch/*

include_HEADERS = src/opendax.h

confdir = $(sysconfdir)/$(PACKAGE)
conf_DATA = etc/opendax.conf.sample \
			etc/modbus.conf.sample \
			etc/daxlua.conf.sample \
                        etc/arduino.conf.sample \
                        etc/tagserver.conf.sample
