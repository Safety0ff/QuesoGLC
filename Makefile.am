SUBDIRS = include build tests @EXAMPLES@

EXTRA_DIST = Changelog \
             INSTALL \
             INSTALL.win \
             LICENSE \
             makefile.mgw \
             database/buildDB.py \
             $(DOCS)

DOCS = docs/body_comparison.eps \
       docs/body_comparison.png \
       docs/Doxyfile \
       docs/glclogo.png \
       docs/Image1.eps \
       docs/Image1.png \
       docs/Image2.eps \
       docs/Image2.png \
       docs/Image3.eps \
       docs/Image3.png \
       docs/Image4.eps \
       docs/Image4.png \
       docs/mainpage.dox \
       docs/measure.eps \
       docs/measure.png \
       docs/qfooter.html \
       docs/qheader.html \
       docs/quesoglc.css \
       docs/screenshot.eps \
       docs/screenshot.png \
       docs/title.png \
       docs/tutorial2.eps \
       docs/tutorial2.png \
       docs/tutorial2_wrong.eps \
       docs/tutorial2_wrong.png \
       docs/tutorial.eps \
       docs/tutorial.png

dist-hook:
	mkdir -p -- $(distdir)/build/m4

.PHONY: doc

doc: $(DOCS)
	cd docs && doxygen && cd ..
