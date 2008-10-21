INCLUDES = -I$(top_srcdir)/include/

check_PROGRAMS = https_test list_test base64_test

TESTS = $(check_PROGRAMS)

GLOBAL_SOURCES = \
   $(top_srcdir)/src/output.c     \
   $(top_srcdir)/src/memwatch.c   \
   $(top_srcdir)/src/utils.c


https_test_SOURCES = $(GLOBAL_SOURCES) \
   https_test.c                    \
   $(top_srcdir)/src/web.c         \
   $(top_srcdir)/src/file.c
   
list_test_SOURCES = $(GLOBAL_SOURCES) \
    $(top_srcdir)/src/list.c      \
    list_test.c
      
base64_test_SOURCES = $(GLOBAL_SOURCES) \
   $(top_srcdir)/src/base64.c       \
   base64_test.c
   

noinst_HEADERS = \
   $(top_srcdir)/include/output.h   \
   $(top_srcdir)/include/list.h     \
   $(top_srcdir)/include/utils.h    \
   $(top_srcdir)/include/memwatch.h \
   $(top_srcdir)/include/web.h      \
   $(top_srcdir)/include/file.h

https_test_LDADD  = $(LIBCURL_LIBS)
https_test_CFLAGS = $(LIBCURL_CFLAGS)


https_test_CPPFLAGS  = -DMEMWATCH -DDEBUG
base64_test_CPPFLAGS = -DMEMWATCH -DDEBUG
list_test_CPPFLAGS   = -DMEMWATCH -DDEBUG