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
#include "acphasespace.h"

#include <poll.h>

#include "phasespace_c_types.h"


/* --- Task publish ----------------------------------------------------- */

/** Codel phsp_publish_start of task publish.
 *
 * Triggered by phasespace_start.
 * Yields to phasespace_pause_poll.
 * Throws phasespace_e_sys.
 */
genom_event
phsp_publish_start(phasespace_ids *ids, const genom_context self)
{
  /* init data */
  ids->server = NULL;

  return phasespace_pause_poll;
}


/** Codel phsp_publish_poll of task publish.
 *
 * Triggered by phasespace_poll.
 * Yields to phasespace_pause_poll, phasespace_poll, phasespace_recv,
 *           phasespace_err.
 * Throws phasespace_e_sys.
 */
genom_event
phsp_publish_poll(const phasespace_server_s *server,
                  const genom_context self)
{
  struct pollfd pfd;
  int s;

  /* when there is no server connected, just wait */
  if (server == NULL || server->fd < 0) return phasespace_pause_poll;

  /* check if there is data to read, for at most 500ms */
  pfd.fd = server->fd;
  pfd.events = POLLIN;
  do s = poll(&pfd, 1, 500/*ms*/); while (s < 0 && errno == EINTR);

  /* return appropriate next state depending on the poll results */
  if (s < 0) return phasespace_err;
  if (s == 0) return phasespace_poll;
  if (pfd.revents & POLLHUP) return phasespace_err;

  /* data to read */
  return phasespace_recv;
}


/** Codel phsp_publish_recv of task publish.
 *
 * Triggered by phasespace_recv.
 * Yields to phasespace_poll, phasespace_err.
 * Throws phasespace_e_sys.
 */
genom_event
phsp_publish_recv(const phasespace_server_s *server,
                  phasespace_log_s **log,
                  phasespace_bodies *bodies,
                  const genom_context self)
{
  // 1. Receive next OWL event
  Event *evt = owl_nextEvent(server->ctx, 0);  // libowl2 API

  if (!evt) {
    return phasespace_poll;  // nothing new yet
  }

  // 2. Handle FRAME events (markers + rigids)
  if (evt->type_id == FRAME) {
    /* Copy markers */
    bodies->num_markers = evt->num_markers;
    if (bodies->num_markers > PHASESPACE_MAX_MARKERS)
      bodies->num_markers = PHASESPACE_MAX_MARKERS;

    for (size_t i = 0; i < bodies->num_markers; i++) {
      bodies->markers[i].id    = evt->markers[i].id;
      bodies->markers[i].flags = evt->markers[i].flags;
      bodies->markers[i].time  = evt->markers[i].time;
      bodies->markers[i].x     = evt->markers[i].x;
      bodies->markers[i].y     = evt->markers[i].y;
      bodies->markers[i].z     = evt->markers[i].z;
      bodies->markers[i].cond  = evt->markers[i].cond;
    }

    /* Copy rigids */
    bodies->num_rigids = evt->num_rigids;
    if (bodies->num_rigids > PHASESPACE_MAX_RIGIDS)
      bodies->num_rigids = PHASESPACE_MAX_RIGIDS;

    for (size_t i = 0; i < bodies->num_rigids; i++) {
      bodies->rigids[i].id    = evt->rigids[i].id;
      bodies->rigids[i].flags = evt->rigids[i].flags;
      bodies->rigids[i].time  = evt->rigids[i].time;
      bodies->rigids[i].x     = evt->rigids[i].pose[0];
      bodies->rigids[i].y     = evt->rigids[i].pose[1];
      bodies->rigids[i].z     = evt->rigids[i].pose[2];
      bodies->rigids[i].qw    = evt->rigids[i].pose[3];
      bodies->rigids[i].qx    = evt->rigids[i].pose[4];
      bodies->rigids[i].qy    = evt->rigids[i].pose[5];
      bodies->rigids[i].qz    = evt->rigids[i].pose[6];
      bodies->rigids[i].cond  = evt->rigids[i].cond;
    }
  }
  // 3. Handle ERROR events
  else if (evt->type_id == ERROR) {
    owl_log(*log);
    return phasespace_e_sys(self);
  }

  // 4. Log
  owl_log(*log);

  return phasespace_poll;
}


/** Codel phsp_publish_err of task publish.
 *
 * Triggered by phasespace_err.
 * Yields to phasespace_pause_poll.
 * Throws phasespace_e_sys.
 */
genom_event
phsp_publish_err(phasespace_server_s **server,
                 const genom_context self)
{
  if (*server) {
    owl_disconnect(*server);  // free ctx and close fd
    *server = NULL;
  }
  return phasespace_pause_poll;
}


/* --- Activity connect ------------------------------------------------- */

/** Codel phsp_connect_start of activity connect.
 *
 * Triggered by phasespace_start.
 * Yields to phasespace_ether.
 * Throws phasespace_e_sys.
 */
genom_event
phsp_connect_start(const char host[128], const char host_port[128],
                   phasespace_server_s **server,
                   const genom_context self)
{
  /* disconnect any previous server */
  if (*server) phsp_disconnect(server, self);

  /* connect to designated host */
  *server = owl_connect(host, host_port);
  if (!*server) return phsp_e_sys_error("owl_connect", self);

  return phasespace_ether;
}


/* --- Activity disconnect ---------------------------------------------- */

/** Codel phsp_disconnect of activity disconnect.
 *
 * Triggered by phasespace_start.
 * Yields to phasespace_ether.
 * Throws phasespace_e_sys.
 */
genom_event
phsp_disconnect(phasespace_server_s **server,
                const genom_context self)
{
  if (*server) {
    owl_disconnect(*server);  // free ctx and close fd
    *server = NULL;
  }
  return phasespace_ether;
}
