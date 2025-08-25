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

#ifndef H_PHASESPACE_OWL
#define H_PHASESPACE_OWL

#include <sys/time.h>
#include <stdint.h>
#include "phasespace_c_types.h"

typedef struct phasespace_server_s *phasespace_server_s;

phasespace_server_s
    owl_connect(const char *host, const char *port);

int
    owl_poll(phasespace_server_s server, struct timeval *timeout);

uint32_t
    owl_version(phasespace_server_s server);

void
    owl_disconnect(phasespace_server_s *server);

void
    owl_log(struct phasespace_log_s *log, const struct phasespace_bodies *bodies);

#endif /* H_PHASESPACE_OWL */
