if MODBUS
  M_MODBUS = modbus
else
  M_MODBUS = 
endif

if TEST
  M_TEST = test
else
  M_TEST = 
endif

if WIO
  M_WIO = wio
else
  M_WIO = 
endif

if DAXLUA
  M_DAXLUA = daxlua
else
  M_DAXLUA =
endif

SUBDIRS = $(M_DAXLUA) $(M_MODBUS) daxc $(M_TEST) $(M_WIO) skel
#AUX_DIST = $(ac_aux_dir)/install-sh

#dist-hook:
#	(cd $(distdir) && mkdir $(ac_aux_dir))
#	for file in $(AUX_DIST); do \
#	  cp $$file $(distdir)/$$file; \
#	done
