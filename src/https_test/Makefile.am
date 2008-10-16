INCLUDES = -I$(top_srcdir)/src/

AM_CFLAGS = $(LIBCURL_CFLAGS)

check_PROGRAMS = https_test

TESTS = $(noinst_PROGRAMS)

https_test_SOURCES =  \
   https_test.c \
   ../output.c  \
   ../utils.c   \
   ../web.c     \
   ../file.c

noinst_HEADERS = \
   ../output.h   \
   ../utils.h    \
   ../web.h      \
   ../file.h

LDADD = $(LIBCURL_LIBS)
