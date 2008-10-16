SUBDIRS = . https_test

AM_CFLAGS = $(LIBXML_CFLAGS) $(LIBCURL_CFLAGS)

bin_PROGRAMS =   \
  automatic      \
	automatic-mw

automatic_SOURCES = \
   automatic.c      \
   base64.c         \
   config_parser.c  \
   downloads.c      \
   feed_item.c      \
   file.c           \
   json.c           \
   list.c           \
   output.c         \
   rss_feed.c       \
   state.c          \
   torrent.c        \
   transmission.c   \
   utils.c          \
   web.c            \
   xml_parser.c

noinst_HEADERS =    \
   automatic.h      \
   base64.h         \
   config_parser.h  \
   downloads.h      \
   feed_item.h      \
   file.h           \
   json.h           \
   list.h           \
   output.h         \
   rss_feed.h       \
   state.h          \
   torrent.h        \
   transmission.h   \
   utils.h          \
   web.h            \
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
	echo '#define LONG_VERSION_STRING   "'@VERSION_NUMBER@' ('`svn info | grep "Revision" | awk -F': ' '{print $$2}'`')"' >> version.h

strip:
	$(STRIP) $(bin_PROGRAMS)
