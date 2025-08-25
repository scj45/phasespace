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
 *
 * ports.c — OWL hardware access layer
 *
 * Provides functions to fetch frames from real OWL hardware
 * and optionally log them asynchronously.
 */

#include "acphasespace.h"
#include "phasespace_c_types.h"
#include "owl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <math.h>

#define OWL_MAX_MARKERS 128
#define OWL_MAX_RIGIDS  64

/* ---------------------------------------------------------------------- */
/* Initialize hardware connection (calls OWL connect internally)          */
/* ---------------------------------------------------------------------- */
int owl_port_init(struct phasespace_server_s **server,
                  const char *host, const char *port)
{
    if (!server || !host || !port) return -1;

    *server = owl_connect(host, port);
    if (!*server) return -1;

    return 0;
}

/* ---------------------------------------------------------------------- */
/* Shutdown hardware connection                                           */
/* ---------------------------------------------------------------------- */
void owl_port_shutdown(struct phasespace_server_s **server)
{
    if (!server || !*server) return;

    owl_disconnect(*server);
    *server = NULL;
}

/* ---------------------------------------------------------------------- */
/* Fetch latest frame from OWL hardware                                    */
/* ---------------------------------------------------------------------- */

void owl_fetch_frame(struct phasespace_server_s *server, phasespace_bodies *bodies)
{
    if (!server || server->fd < 0 || !bodies) return;

    memset(bodies, 0, sizeof(*bodies));

    /* OWL TCP frame header is typically 8 bytes: numMarkers(2), numRigid(2), etc. */
    uint8_t header[8];
    ssize_t n = recv(server->fd, header, sizeof(header), MSG_WAITALL);
    if (n != sizeof(header)) {
        fprintf(stderr, "Failed to read OWL header\n");
        return;
    }

    uint16_t num_markers = ntohs(*(uint16_t*)&header[0]);
    uint16_t num_rigids = ntohs(*(uint16_t*)&header[2]);

    if (num_markers > OWL_MAX_MARKERS) num_markers = OWL_MAX_MARKERS;
    if (num_rigids > OWL_MAX_RIGIDS) num_rigids = OWL_MAX_RIGIDS;

    bodies->num_markers = num_markers;
    bodies->num_rigids = num_rigids;

    /* Each marker: x,y,z float32 + cond float32 (16 bytes per marker) */
    for (int i = 0; i < num_markers; i++) {
        float xyz[3], cond;
        if (recv(server->fd, xyz, sizeof(xyz), MSG_WAITALL) != sizeof(xyz)) return;
        if (recv(server->fd, &cond, sizeof(cond), MSG_WAITALL) != sizeof(cond)) return;

        bodies->markers[i].id = i+1;
        bodies->markers[i].x = xyz[0];
        bodies->markers[i].y = xyz[1];
        bodies->markers[i].z = xyz[2];
        bodies->markers[i].cond = cond;
    }

    /* Each rigid body: x,y,z + qw,qx,qy,qz + cond (float32 each, 32 bytes per rigid) */
    for (int i = 0; i < num_rigids; i++) {
        float data[8];
        if (recv(server->fd, data, sizeof(data), MSG_WAITALL) != sizeof(data)) return;

        bodies->rigids[i].id = i+1;
        bodies->rigids[i].x  = data[0];
        bodies->rigids[i].y  = data[1];
        bodies->rigids[i].z  = data[2];
        bodies->rigids[i].qw = data[3];
        bodies->rigids[i].qx = data[4];
        bodies->rigids[i].qy = data[5];
        bodies->rigids[i].qz = data[6];
        bodies->rigids[i].cond = data[7];
    }
}

/* ---------------------------------------------------------------------- */
/* Optional: log a frame to file using the async logger                   */
/* ---------------------------------------------------------------------- */
void owl_log_frame(struct phasespace_log_s *log,
                   const struct phasespace_bodies *bodies)
{
    if (!log || !bodies) return;
    owl_log(log, bodies);
}