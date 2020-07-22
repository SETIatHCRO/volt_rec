ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS     =

AM_CPPFLAGS += -I"@HASHPIPE_INCDIR@"

# AM_CFLAGS is used for all C compiles
AM_CFLAGS = -ggdb -fPIC -O3 -Wall -Wno-error=cpp -fno-strict-aliasing @X86_FEATURE_CFLAGS@

# Convenience variables to group source files
hpguppi_databuf = hpguppi_databuf.h \
		  hpguppi_databuf.c

hpguppi_support = hpguppi_params.c \
		  hpguppi_mkfeng.h \
		  hpguppi_pksuwl.h \
		  hpguppi_rawspec.h \
		  hpguppi_rawspec.c \
		  hpguppi_time.h   \
		  hpguppi_time.c   \
		  hpguppi_util.h   \
		  hpguppi_util.c   \
		  hpguppi_udp.h    \
		  hpguppi_udp.c

#		  hpguppi_vdif.h   \
#		  polyco_struct.h  \
#		  psrfits.h

libsla_support = slalib.h sla.c f77.h

hpguppi_threads = ata_volt_outer_thread.c

# This is the hpguppi_daq plugin
lib_LTLIBRARIES = hpguppi_daq.la
hpguppi_daq_la_SOURCES  = $(hpguppi_databuf) $(hpguppi_support) $(libsla_support) $(hpguppi_threads)
hpguppi_daq_la_LIBADD   = -lsla -lrt -lrawspec -lhashpipe_ibverbs
hpguppi_daq_la_LDFLAGS  = -avoid-version -module -shared -export-dynamic
hpguppi_daq_la_LDFLAGS += -L"@LIBSLADIR@" -Wl,-rpath,"@LIBSLADIR@"
hpguppi_daq_la_LDFLAGS += -L"@HASHPIPE_LIBDIR@" -Wl,-rpath,"@HASHPIPE_LIBDIR@"

# Installed scripts
dist_bin_SCRIPTS = recv_init.sh

# vi: set ts=8 noet :
