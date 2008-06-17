AM_CFLAGS = $(LIBXML_CFLAGS) $(LIBCURL_CFLAGS)

bin_PROGRAMS = automatic

automatic_SOURCES = \
   automatic.c     \
   config_parser.c \
   list.c          \
   output.c        \
   state.c         \
   web.c

noinst_HEADERS = \
   config_parser.h  \
   list.h           \
   output.h         \
   state.h          \
   web.h

dist_sysconf_DATA = automatic.conf

LDADD = \
    $(LIBCURL_LIBS) \
    $(LIBXML_LIBS)

clean-local:
	if test -d .svn; then rm -f version.h; fi;

EXTRA_automatic_SOURCES = \
    version.h

BUILT_SOURCES = \
    version.h



version.h:
	echo '#define SVN_REVISION          "'`svn info | grep "Revision" | awk -F': ' '{print $$2}'`'"' >> version.h
	echo '#define LONG_VERSION_STRING   "'0.1.2' ('`svn info | grep "Revision" | awk -F': ' '{print $$2}'`')"' >> version.h

strip:
	$(STRIP) $(bin_PROGRAMS)
