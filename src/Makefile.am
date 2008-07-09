AM_CFLAGS = $(LIBXML_CFLAGS) $(LIBCURL_CFLAGS)

bin_PROGRAMS =   \
  automatic      \
	automatic-mw

automatic_SOURCES = \
   automatic.c      \
   config_parser.c  \
   list.c           \
   output.c         \
   state.c          \
   web.c            \
   feed_item.c      \
   rss_feed.c       \
   downloads.c      \
   utils.c          \
	xml_parser.c

noinst_HEADERS =    \
   automatic.h      \
   config_parser.h  \
   list.h           \
   output.h         \
   state.h          \
   web.h            \
   feed_item.h      \
   rss_feed.h       \
   downloads.h      \
   utils.h          \
   xml_parser.h

automatic_mw_SOURCES =  \
   $(automatic_SOURCES) \
	 memwatch.c

automatic_mw_HEADERS =  \
   $(noinst_HEADERS)    \
   memwatch.h


automatic_mwdir = .

automatic_mw_CPPFLAGS = -DMEMWATCH -DDEBUG

dist_sysconf_DATA = automatic.conf-sample

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
	echo '#define LONG_VERSION_STRING   "'0.1.3' ('`svn info | grep "Revision" | awk -F': ' '{print $$2}'`')"' >> version.h

strip:
	$(STRIP) $(bin_PROGRAMS)
