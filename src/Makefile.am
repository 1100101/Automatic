SUBDIRS = . tests
INCLUDES = -I$(top_srcdir)/include/

AM_CFLAGS = $(LIBXML_CFLAGS) $(LIBCURL_CFLAGS) $(PCRE_CFLAGS)

bin_PROGRAMS = automatic

automatic_SOURCES = \
   $(top_srcdir)/src/automatic.c      \
   $(top_srcdir)/src/base64.c         \
   $(top_srcdir)/src/config_parser.c  \
   $(top_srcdir)/src/downloads.c      \
   $(top_srcdir)/src/feed_item.c      \
   $(top_srcdir)/src/file.c           \
   $(top_srcdir)/src/json.c           \
   $(top_srcdir)/src/list.c           \
   $(top_srcdir)/src/output.c         \
   $(top_srcdir)/src/regex.c          \
   $(top_srcdir)/src/rss_feed.c       \
   $(top_srcdir)/src/state.c          \
   $(top_srcdir)/src/torrent.c        \
   $(top_srcdir)/src/transmission.c   \
   $(top_srcdir)/src/utils.c          \
   $(top_srcdir)/src/web.c            \
   $(top_srcdir)/src/xml_parser.c

noinst_HEADERS =    \
   $(top_srcdir)/include/automatic.h      \
   $(top_srcdir)/include/base64.h         \
   $(top_srcdir)/include/config_parser.h  \
   $(top_srcdir)/include/downloads.h      \
   $(top_srcdir)/include/feed_item.h      \
   $(top_srcdir)/include/file.h           \
   $(top_srcdir)/include/json.h           \
   $(top_srcdir)/include/list.h           \
   $(top_srcdir)/include/output.h         \
   $(top_srcdir)/include/regex.h          \
   $(top_srcdir)/include/rss_feed.h       \
   $(top_srcdir)/include/state.h          \
   $(top_srcdir)/include/torrent.h        \
   $(top_srcdir)/include/transmission.h   \
   $(top_srcdir)/include/utils.h          \
   $(top_srcdir)/include/web.h            \
   $(top_srcdir)/include/xml_parser.h


dist_sysconf_DATA = automatic.conf-sample

LDADD = \
    $(LIBCURL_LIBS) \
    $(LIBXML_LIBS)  \
    $(PCRE_LIBS)

clean-local:
	if test -d .svn; then rm -f version.h; fi;

EXTRA_automatic_SOURCES = \
    version.h

BUILT_SOURCES = \
    version.h

version.h:
	echo '#define SVN_REVISION          "'`svn info | grep "Revision" | awk -F': ' '{print $$2}'`'"' > version.h
	echo '#define LONG_VERSION_STRING   "'@VERSION_NUMBER@' ('`svn info | grep "Revision" | awk -F': ' '{print $$2}'`')"' >> version.h

strip:
	$(STRIP) $(bin_PROGRAMS)
