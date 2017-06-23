if DEBUG
  AM_CFLAGS = -g -O3
else
  AM_CFLAGS = -O3
endif

AM_CFLAGS += -I$(top_srcdir)/src
AM_CFLAGS += $(GLIB_CFLAGS)

AM_CFLAGS += -Wall

bin_PROGRAMS = i2c_access
i2c_access_SOURCES = i2c_access.c
i2c_access_LDADD = $(AM_LDFLAGS)
i2c_access_LDADD += $(GLIB_LIBS)
