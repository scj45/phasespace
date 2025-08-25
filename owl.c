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

#include "phasespace_c_types.h"

#include <arpa/inet.h>
#include <assert.h>
#include <aio.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <netdb.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

/* ---------------------------------------------------------------------- */
/* OWL connection -------------------------------------------------------- */
struct phasespace_server_s *
owl_connect(const char *host, const char *port)
{
    struct phasespace_server_s *server;
    struct addrinfo hints, *res, *res0;
    int sfd;

    server = malloc(sizeof(*server));
    if (!server) return NULL;
    server->fd = -1;
    server->ctx = NULL; /* store SDK context if needed */

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, port, &hints, &res0) != 0) {
        free(server);
        return NULL;
    }

    for (res = res0; res; res = res->ai_next) {
        sfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sfd < 0) continue;
        if (connect(sfd, res->ai_addr, res->ai_addrlen) == 0) {
            server->fd = sfd;
            break;
        }
        close(sfd);
    }

    freeaddrinfo(res0);
    if (server->fd < 0) {
        free(server);
        return NULL;
    }

    /* TODO: initialize SDK context if using libowl2
       server->ctx = owl2_create_context(host, port);
       if (!server->ctx) { close(server->fd); free(server); return NULL; }
    */

    return server;
}

/* ---------------------------------------------------------------------- */
/* Poll for data --------------------------------------------------------- */
int
owl_poll(struct phasespace_server_s server, struct timeval *timeout)
{
    assert(server.fd >= 0);

    struct pollfd fds[1];
    fds[0].fd = server.fd;
    fds[0].events = POLLIN;

    int t_ms = timeout ? (timeout->tv_sec * 1000 + timeout->tv_usec / 1000) : -1;

    int ret;
    do {
        ret = poll(fds, 1, t_ms);
    } while (ret < 0 && errno == EINTR);

    if (ret < 0) return -1;
    if (ret == 0) return 0; /* timeout */
    if (fds[0].revents & POLLIN) return 1;
    return 0;
}

/* ---------------------------------------------------------------------- */
/* OWL version (dummy if SDK does not provide) --------------------------- */
uint32_t
owl_version(struct phasespace_server_s server)
{
    (void)server;
    return 2*1000 + 12; /* example: 2.12 */
}

/* ---------------------------------------------------------------------- */
/* Disconnect ------------------------------------------------------------ */
void
owl_disconnect(struct phasespace_server_s *server)
{
    if (!server) return;

    /* TODO: destroy SDK context if used
       if (server->ctx) owl2_destroy_context(server->ctx);
    */
    if (server->fd >= 0) close(server->fd);
    free(server);
}

/* ---------------------------------------------------------------------- */
/* Initialize async logging ---------------------------------------------- */
void
owl_log_init(struct phasespace_log_s *log, const char *path, uint32_t decimation)
{
    if (!log || !path) return;

    memset(log, 0, sizeof(*log));
    strncpy(log->path, path, sizeof(log->path)-1);
    log->decimation = decimation;
    log->req.aio_fildes = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (log->req.aio_fildes < 0) {
        warnx("Failed to open log file: %s", path);
        return;
    }

    log->req.aio_buf = log->buffer;
    log->req.aio_offset = 0;
    log->req.aio_nbytes = 0;
    log->pending = false;
    log->skipped = false;
    log->missed = 0;
    log->total = 0;

    /* Write header */
    int n = snprintf(log->buffer, sizeof(log->buffer),
                     "name ts_sec ts_nsec x y z roll pitch yaw cond noise\n");
    if (n > 0) {
        log->req.aio_nbytes = n;
        if (aio_write(&log->req)) {
            warnx("Failed to write log header: %s", path);
        } else {
            log->pending = true;
        }
    }
}

/* ---------------------------------------------------------------------- */
/* Log a frame with condition and noise ---------------------------------- */
void
owl_log(struct phasespace_log_s *log, const phasespace_bodies *bodies)
{
    if (!log || !bodies || log->req.aio_fildes < 0) return;

    log->total++;
    if (log->total % log->decimation != 0) return;

    if (log->pending) {
        if (aio_error(&log->req) != EINPROGRESS) {
            log->pending = false;
            if (aio_return(&log->req) <= 0) {
                warnx("log %s", log->path);
                close(log->req.aio_fildes);
                log->req.aio_fildes = -1;
            }
        } else {
            log->skipped = true;
            log->missed++;
            return;
        }
    }

    char *bufptr = log->buffer;
    size_t bufrem = sizeof(log->buffer);

    /* Markers */
    for (size_t i = 0; i < bodies->num_markers; i++) {
        const phasespace_marker_s *m = &bodies->markers[i];
        double noise = 0.0;
        if (i < log->prev_bodies.num_markers) {
            const phasespace_marker_s *prev = &log->prev_bodies.markers[i];
            double dx = m->x - prev->x;
            double dy = m->y - prev->y;
            double dz = m->z - prev->z;
            noise = sqrt(dx*dx + dy*dy + dz*dz);
        }
        int n = snprintf(bufptr, bufrem,
                         "marker %" PRIu64 ".%09d %g %g %g 0 0 0 %g %g\n",
                         (uint64_t)m->time, 0,
                         m->x, m->y, m->z,
                         m->cond, noise);
        if (n <= 0 || (size_t)n >= bufrem) break;
        bufptr += n;
        bufrem -= n;
    }

    /* Rigid bodies */
    for (size_t i = 0; i < bodies->num_rigids; i++) {
        const phasespace_rigid_s *r = &bodies->rigids[i];
        double qw = r->qw, qx = r->qx, qy = r->qy, qz = r->qz;
        double roll = atan2(2*(qw*qx + qy*qz), 1 - 2*(qx*qx + qy*qy));
        double pitch = asin(fmax(fmin(2*(qw*qy - qz*qx), 1.0), -1.0));
        double yaw = atan2(2*(qw*qz + qx*qy), 1 - 2*(qy*qy + qz*qz));

        double noise = 0.0;
        if (i < log->prev_bodies.num_rigids) {
            const phasespace_rigid_s *prev = &log->prev_bodies.rigids[i];
            double dx = r->x - prev->x;
            double dy = r->y - prev->y;
            double dz = r->z - prev->z;
            noise = sqrt(dx*dx + dy*dy + dz*dz);
        }

        int n = snprintf(bufptr, bufrem,
                         "rigid %" PRIu64 ".%09d %g %g %g %g %g %g %g %g\n",
                         (uint64_t)r->time, 0,
                         r->x, r->y, r->z,
                         roll, pitch, yaw,
                         r->cond, noise);
        if (n <= 0 || (size_t)n >= bufrem) break;
        bufptr += n;
        bufrem -= n;
    }

    log->req.aio_nbytes = bufptr - log->buffer;

    if (aio_write(&log->req)) {
        warnx("log %s", log->path);
        close(log->req.aio_fildes);
        log->req.aio_fildes = -1;
    } else {
        log->pending = true;
        log->skipped = false;
    }

    /* Save frame for next noise calculation */
    log->prev_bodies = *bodies;
}

