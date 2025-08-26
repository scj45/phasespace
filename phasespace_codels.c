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

#include <sys/time.h>

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "phasespace_c_types.h"
#include "phsp.h"
#include "owl.h"


/* --- Function phsp_log_start (async version) -------------------------- */

/** Codel phsp_log_start of function log.
 *
 * Initializes a phasespace_log_s struct for asynchronous logging.
 * Writes the CSV header asynchronously.
 *
 * Returns genom_ok on success, or phasespace_e_sys on failure.
 */
genom_event
phsp_log_start(const char path[64], uint32_t decimation,
               phasespace_log_s **log, const genom_context self)
{
    if (!log || !path) return phsp_e_sys_error("Invalid log pointer", self);

    /* Allocate if needed */
    if (*log == NULL) {
        *log = malloc(sizeof(phasespace_log_s));
        if (!*log) return phsp_e_sys_error("Memory allocation failed", self);
    }

    memset(*log, 0, sizeof(phasespace_log_s));

    /* Store path */
    snprintf((*log)->path, sizeof((*log)->path), "%s", path);
    (*log)->decimation = decimation < 1 ? 1 : decimation;

    /* Open file asynchronously */
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) return phsp_e_sys_error(path, self);

    (*log)->req.aio_fildes = fd;
    (*log)->req.aio_buf = (*log)->buffer;
    (*log)->req.aio_offset = 0;
    (*log)->req.aio_nbytes = 0;
    (*log)->req.aio_sigevent.sigev_notify = SIGEV_NONE;
    (*log)->req.aio_lio_opcode = LIO_NOP;
    (*log)->pending = false;
    (*log)->skipped = false;
    (*log)->missed = 0;
    (*log)->total = 0;

    /* Prepare header in buffer */
    int n = snprintf((*log)->buffer, sizeof((*log)->buffer), "%s\n", phsp_log_header);
    if (n <= 0) {
        close(fd);
        return phsp_e_sys_error("Failed to format log header", self);
    }

    (*log)->req.aio_nbytes = n;

    /* Write header asynchronously */
    if (aio_write(&(*log)->req)) {
        close(fd);
        return phsp_e_sys_error("Failed to write log header", self);
    }

    (*log)->pending = true;

    return genom_ok;
}
