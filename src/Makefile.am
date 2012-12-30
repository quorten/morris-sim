## Process this file with automake to produce Makefile.in

# Copyright (C) 2012 Andrew Makousky
#
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation files
# (the "Software"), to deal in the Software without restriction,
# including without limitation the rights to use, copy, modify, merge,
# publish, distribute, sublicense, and/or sell copies of the Software,
# and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
# BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
# ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

INCLUDES = \
	-DPACKAGE_PREFIX=\""$(prefix)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	@PACKAGE_CFLAGS@

bin_PROGRAMS = morris-ui morris-sim

morris_ui_SOURCES = \
	morris-ui.c morris-term.c \
	core.h \
	support.c support.h \
	morris.c morris.h \
	tables.h tab_unpack.h

morris_sim_SOURCES = \
	morris-sim.c \
	core.h wpthread.h \
	morris.c morris.h \
	tables.h tab_unpack.h

morris_ui_LDADD = @PACKAGE_LIBS@ $(INTLLIBS)

if WITH_WIN32

morris-ui-rc.o: morris-ui.rc
	$(RC) $(DEFS) $(DEFAULT_INCLUDES) -O coff -o $@ $<

morris_ui_SOURCES += morris-ui.rc
morris_ui_LDADD += morris-ui-rc.o
endif
