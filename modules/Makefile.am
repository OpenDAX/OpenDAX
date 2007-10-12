if MODBUS
  M_MODBUS = modbus
else
  M_MODBUS = 
endif


if HAVE_LUA
  M_DAXLUA = daxlua
endif

if HAVE_LUA50
  M_DAXLUA = daxlua
endif

if HAVE_LUA51
  M_DAXLUA = daxlua
endif

#if HAVE_LUALIB
#  M_DAXLUA = daxlua
#endif

#if HAVE_LUALIB50
#  M_DAXLUA = daxlua
#endif

#if HAVE_LUALIB51
#  M_DAXLUA = daxlua
#endif

SUBDIRS = $(M_DAXLUA) $(M_MODBUS) daxc test 
#AUX_DIST = $(ac_aux_dir)/install-sh

#dist-hook:
#	(cd $(distdir) && mkdir $(ac_aux_dir))
#	for file in $(AUX_DIST); do \
#	  cp $$file $(distdir)/$$file; \
#	done
