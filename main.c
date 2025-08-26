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
#include "ports.c"  
#include "phsp.h"
struct phasespace_server_s *server;
struct phasespace_log_s *log = NULL;
struct phasespace_bodies frame;

owl_port_init(&server, "192.168.1.10", "23");
phsp_log_start("motion_log.txt", 1, &log, NULL);

for (int i = 0; i < 1000; i++) {
    owl_fetch_frame(server, &frame);
    owl_log_frame(log, &frame);
}

phsp_log_stop(&log, NULL);
owl_port_shutdown(&server);


int main(void)
{
    struct phasespace_server_s *server=NULL;
    struct phasespace_log_s *log=NULL;
    struct phasespace_bodies frame;

    server = owl_connect("127.0.0.1","23");
    if(!server){fprintf(stderr,"Failed to connect\n"); return 1;}

    phsp_log_start("motion_capture_log.txt",1,&log);

    struct timeval tv={0,100000};
    for(int i=0;i<100;i++){
        int ready=owl_poll(server,&tv);
        if(ready>0){
            owl_fetch_frame(server,&frame);
            owl_log(log,&frame);
        }
        usleep(5000);
    }

    phsp_log_stop(&log);
    owl_disconnect(&server);
    return 0;
}