AM_CPPFLAGS = -I$(top_srcdir)/include/

check_PROGRAMS = list_test base64_test regex_test http_test json_test prowl_test parser_test utils_test toasty_test pushalot_test pushover_test

TESTS = $(check_PROGRAMS)

GLOBAL_SOURCES = \
   $(top_srcdir)/src/output.c     \
   $(top_srcdir)/src/memwatch.c   \
   $(top_srcdir)/src/utils.c

http_test_SOURCES = $(GLOBAL_SOURCES)  \
   $(top_srcdir)/src/file.c            \
   $(top_srcdir)/src/regex.c           \
   $(top_srcdir)/src/urlcode.c         \
   $(top_srcdir)/src/web.c             \
   http_test.c

list_test_SOURCES = $(GLOBAL_SOURCES) \
   $(top_srcdir)/src/list.c           \
   list_test.c

base64_test_SOURCES = $(GLOBAL_SOURCES) \
   $(top_srcdir)/src/base64.c           \
   base64_test.c

utils_test_SOURCES = $(GLOBAL_SOURCES) \
   utils_test.c

regex_test_SOURCES = $(GLOBAL_SOURCES) \
   $(top_srcdir)/src/regex.c           \
   regex_test.c

json_test_SOURCES = $(GLOBAL_SOURCES) \
   $(top_srcdir)/src/base64.c         \
   $(top_srcdir)/src/json.c           \
   $(top_srcdir)/src/regex.c          \
   json_test.c

prowl_test_SOURCES = $(GLOBAL_SOURCES) \
   $(top_srcdir)/src/prowl.c           \
   $(top_srcdir)/src/regex.c           \
   $(top_srcdir)/src/urlcode.c         \
   $(top_srcdir)/src/web.c             \
   prowl_test.c

pushalot_test_SOURCES = $(GLOBAL_SOURCES) \
   $(top_srcdir)/src/pushalot.c           \
   $(top_srcdir)/src/regex.c              \
   $(top_srcdir)/src/urlcode.c            \
   $(top_srcdir)/src/web.c                \
   pushalot_test.c

toasty_test_SOURCES = $(GLOBAL_SOURCES) \
   $(top_srcdir)/src/toasty.c           \
   $(top_srcdir)/src/regex.c            \
   $(top_srcdir)/src/urlcode.c          \
   $(top_srcdir)/src/web.c              \
   toasty_test.c

pushover_test_SOURCES = $(GLOBAL_SOURCES) \
   $(top_srcdir)/src/pushover.c           \
   $(top_srcdir)/src/regex.c              \
   $(top_srcdir)/src/urlcode.c            \
   $(top_srcdir)/src/web.c                \
   pushover_test.c

parser_test_SOURCES = $(GLOBAL_SOURCES) \
   $(top_srcdir)/src/config_parser.c    \
   $(top_srcdir)/src/filters.c          \
   $(top_srcdir)/src/list.c             \
   $(top_srcdir)/src/regex.c            \
   $(top_srcdir)/src/rss_feed.c         \
   parser_test.c

noinst_HEADERS = \
   $(top_srcdir)/include/base64.h   \
   $(top_srcdir)/include/config_parser.h     \
   $(top_srcdir)/include/file.h     \
   $(top_srcdir)/include/json.h     \
   $(top_srcdir)/include/list.h     \
   $(top_srcdir)/include/memwatch.h \
   $(top_srcdir)/include/output.h   \
   $(top_srcdir)/include/prowl.h    \
   $(top_srcdir)/include/regex.h    \
   $(top_srcdir)/include/urlcode.h  \
   $(top_srcdir)/include/utils.h    \
   $(top_srcdir)/include/web.h

http_test_LDADD  = $(LIBCURL_LIBS) $(PCRE_LIBS)
http_test_CFLAGS = $(LIBCURL_CFLAGS) $(PCRE_CFLAGS)

prowl_test_LDADD  = $(LIBCURL_LIBS) $(LIBXML_LIBS)  $(PCRE_LIBS)
prowl_test_CFLAGS = $(LIBCURL_CFLAGS) $(LIBXML_CFLAGS) $(PCRE_CFLAGS)

toasty_test_LDADD  = $(LIBCURL_LIBS) $(LIBXML_LIBS)  $(PCRE_LIBS)
toasty_test_CFLAGS = $(LIBCURL_CFLAGS) $(LIBXML_CFLAGS) $(PCRE_CFLAGS)

pushalot_test_LDADD  = $(LIBCURL_LIBS) $(LIBXML_LIBS)  $(PCRE_LIBS)
pushalot_test_CFLAGS = $(LIBCURL_CFLAGS) $(LIBXML_CFLAGS) $(PCRE_CFLAGS)

pushover_test_LDADD  = $(LIBCURL_LIBS) $(LIBXML_LIBS)  $(PCRE_LIBS)
pushover_test_CFLAGS = $(LIBCURL_CFLAGS) $(LIBXML_CFLAGS) $(PCRE_CFLAGS)

regex_test_LDADD  = $(PCRE_LIBS)
regex_test_CFLAGS = $(PCRE_CFLAGS)

json_test_LDADD  = $(PCRE_LIBS)
json_test_CFLAGS = $(PCRE_CFLAGS)

parser_test_LDADD = $(PCRE_LIBS)
parser_test_CFLAGS = $(PCRE_CFLAGS)

CFLAGS = -g -O0 -DMEMWATCH -DDEBUG
