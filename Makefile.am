#
# Copyright (c) 2025 UCL
# All rights reserved.
#
# Redistribution and use  in source  and binary  forms,  with or without
# modification, are permitted provided that the following conditions are
# met:
#
#   1. Redistributions of  source  code must retain the  above copyright
#      notice and this list of conditions.
#   2. Redistributions in binary form must reproduce the above copyright
#      notice and  this list of  conditions in the  documentation and/or
#      other materials provided with the distribution.
#
lib_LTLIBRARIES =

# phasespace codels library
lib_LTLIBRARIES += libphasespace_codels.la

libphasespace_codels_la_SOURCES  =	phasespace_c_types.h
libphasespace_codels_la_SOURCES +=	phsp.h
libphasespace_codels_la_SOURCES +=	phasespace_codels.c
libphasespace_codels_la_SOURCES +=	phasespace_publish_codels.c
libphasespace_codels_la_SOURCES +=	phsp_ports.c

libphasespace_codels_la_SOURCES +=	owl.c
libphasespace_codels_la_SOURCES +=	owl.h

libphasespace_codels_la_CPPFLAGS =	$(requires_CFLAGS)
libphasespace_codels_la_LIBADD   =	$(requires_LIBS)
libphasespace_codels_la_CPPFLAGS+=	$(codels_requires_CFLAGS)
libphasespace_codels_la_LIBADD  +=	$(codels_requires_LIBS)
libphasespace_codels_la_LDFLAGS  =	-release $(PACKAGE_VERSION)


# idl mappings
BUILT_SOURCES=	phasespace_c_types.h
CLEANFILES=	${BUILT_SOURCES}

phasespace_c_types.h: ${top_srcdir}/phasespace.gen
	${GENOM3}  mappings \
	  -MD -MF .deps/$@.d -MT $@ --signature -l c $< >$@

-include .deps/phasespace_c_types.h.d
