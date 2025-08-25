/*
 * Copyright (c) 2025 UCL
 * All rights reserved.
 *
 * Redistribution and use  in source  and binary  forms,  with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   1. Redistributions of  source  code must retain the  above copyright
 *      notice and this list of conditions.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice and  this list of  conditions in the  documentation and/or
 *      other materials provided with the distribution.
 */

#ifndef H_PHASESPACE_C_TYPES
#define H_PHASESPACE_C_TYPES

#include <aio.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/* ---------------------------------------------------------------------- */
/* Server connection wrapper                                              */
/* ---------------------------------------------------------------------- */
struct phasespace_server_s {
  /* private data for OWL protocol (e.g. libowl2 socket/context) */
  int fd; /* TCP socket or handle */
  void *ctx; /* opaque OWL context pointer if needed */
};

/* ---------------------------------------------------------------------- */
/* Logging struct                                                         */
/* ---------------------------------------------------------------------- */
struct phasespace_log_s {
  struct aiocb req;
  char path[1024];
  char buffer[4096];
  bool pending, skipped;
  uint32_t decimation;
  size_t missed, total;

# define phsp_log_header \
  "name ts  x y z  roll pitch yaw"
# define phsp_log_line \
  "%s %" PRIu64 ".%09d  %g %g %g  %g %g %g"
};

/* ---------------------------------------------------------------------- */
/* Marker and rigid body definitions                                      */
/* ---------------------------------------------------------------------- */
typedef struct {
  int32_t id;
  int32_t flags;
  int64_t time;          /* system timestamp */
  double x, y, z;        /* position */
  double cond;           /* condition number (<=0 = invalid) */
} phasespace_marker_s;

typedef struct {
  int32_t id;
  int32_t flags;
  int64_t time;          /* system timestamp */
  double x, y, z;        /* position */
  double qw, qx, qy, qz; /* orientation quaternion */
  double cond;           /* condition number (<=0 = invalid) */
} phasespace_rigid_s;

/* ---------------------------------------------------------------------- */
/* Bodies container (published to ports)                                  */
/* ---------------------------------------------------------------------- */
#define PHASESPACE_MAX_MARKERS 128
#define PHASESPACE_MAX_RIGIDS   64

typedef struct {
  size_t num_markers;
  phasespace_marker_s markers[PHASESPACE_MAX_MARKERS];
  size_t num_rigids;
  phasespace_rigid_s rigids[PHASESPACE_MAX_RIGIDS];
} phasespace_bodies;

/* ---------------------------------------------------------------------- */
/* Error helper                                                           */
/* ---------------------------------------------------------------------- */
static inline genom_event
phsp_e_sys_error(const char *s, genom_context self)
{
  phasespace_e_sys_detail d;

  d.code = errno;
  snprintf(d.what, sizeof(d.what), "%s%s%s",
           s ? s : "", s ? ": " : "", strerror(d.code));
  return phasespace_e_sys(&d, self);
}

/* ---------------------------------------------------------------------- */
/* OWL protocol wrappers (to be implemented with libowl2 or bindings)     */
/* ---------------------------------------------------------------------- */
struct phasespace_server_s *
owl_connect(const char *host, const char *port);

int
owl_poll(struct phasespace_server_s server, struct timeval *timeout);

uint32_t
owl_version(struct phasespace_server_s server);

void
owl_disconnect(struct phasespace_server_s *server);

void
owl_log(struct phasespace_log_s *log);

#endif /* H_PHASESPACE_C_TYPES */
