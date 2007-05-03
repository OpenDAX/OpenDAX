SUBDIRS = dcs

AUX_DIST = $(ac_aux_dir)/install-sh

dist-hook:
	(cd $(distdir) && mkdir $(ac_aux_dir))
	for file in $(AUX_DIST); do \
	  cp $$file $(distdir)/$$file; \
	done
