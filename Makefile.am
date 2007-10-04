SUBDIRS = lib dax modules

AUX_DIST = $(ac_aux_dir)/install-sh

EXTRA_DIST = common.h config/config.* config/depcomp config/install-sh config/ltmain.sh config/missing config/mkinstalldirs etc/*.conf 

include_HEADERS = opendax.h

confdir = $(sysconfdir)/$(PACKAGE)
conf_DATA = etc/opendax.conf etc/modbus.conf


#dist-hook:
#	(cd $(distdir) && mkdir $(ac_aux_dir))
#	for file in $(AUX_DIST); do \
#	  cp $$file $(distdir)/$$file; \
#	done
