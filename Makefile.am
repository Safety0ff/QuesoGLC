SUBDIRS = lib tests @EXAMPLES@

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(includedir)/GL
	$(INSTALL_DATA) include/GL/glc.h $(DESTDIR)$(includedir)/GL/glc.h
uninstall-local:
	rm -f $(DESTDIR)$(includedir)/GL/glc.h
