/*
 * Copyright (c) 2015-2019,2021-2024 LAAS/CNRS
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
 *                                      Anthony Mallet on Mon Feb 16 2015
 */
#ifndef H_ROTORCRAFT_CODELS
#define H_ROTORCRAFT_CODELS

#include <sys/stat.h>

#include <aio.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>

#include "rotorcraft_c_types.h"

struct rotorcraft_log_s {
  int fd;
  struct aiocb req;
  char buffer[4096];
  bool pending, skipped;
  uint32_t decimation;
  size_t missed, total;

# define rc_log_header_fmt                                                     \
  "ts imu_rate mag_rate motor_rate bat imu_temp "                              \
  "imu_wx imu_wy imu_wz raw_wx raw_wy raw_wz "                                 \
  "imu_ax imu_ay imu_az raw_ax raw_ay raw_az "                                 \
  "mag_x mag_y mag_z raw_mx raw_my raw_mz "                                    \
  "cmd_v0 cmd_v1 cmd_v2 cmd_v3 cmd_v4 cmd_v5 cmd_v6 cmd_v7 "                   \
  "meas_v0 thro0 cons0 meas_v1 thro1 cons1 meas_v2 thro2 cons2 "               \
  "meas_v3 thro3 cons3 meas_v4 thro4 cons4 meas_v5 thro5 cons5 "               \
  "meas_v6 thro6 cons6 meas_v7 thro7 cons7 "                                   \
  "clk0 clk1 clk2 clk3 clk4 clk5 clk6 clk7"
};

enum rc_device {
  RC_NONE,
  RC_MKBL,
  RC_MKFL,
  RC_FLYMU,
  RC_CHIMERA,
  RC_TEENSY,
  RC_TAWAKI
};

struct mk_channel_s {
  enum rc_device device; /* hw details */
  double rev;
  bool imu, mag, motor;
  uint16_t minid, maxid;

  char path[1024];	/* i/o descriptor */
  dev_t st_dev;
  ino_t st_ino;
  int fd;

  uint8_t buf[64], r, w; /* read ring buffer */

  bool start;
  bool escape;
  uint32_t skipped;
  uint8_t msg[64], len; /* last message */
};

struct rotorcraft_conn_s {
  struct mk_channel_s *chan;
  uint32_t n;
};

static inline genom_event
mk_e_sys_error(const char *s, genom_context self)
{
  rotorcraft_e_sys_detail d;
  size_t l = 0;

  d.code = errno;
  if (s) {
    strncpy(d.what, s, sizeof(d.what) - 3);
    l = strlen(s);
    strcpy(d.what + l, ": ");
    l += 2;
  }
  strerror_r(d.code, d.what + l, sizeof(d.what) - l);
  return rotorcraft_e_sys(&d, self);
}

/* Multi-rotor velocity */
genom_event
my_set_all_rotor_velocity(const rotorcraft_conn_s *conn,
                          const rotorcraft_ids_rotor_data_s rotor_data[8],
                          const double velocities[or_rotorcraft_max_rotors],
                          const genom_context self);


/* efficient integer pow() */
static inline double
powi(double x, uint32_t y)
{
  double r = 1.;

  while (1) {
    if (y & 1) r *= x;
    y /= 2;
    if (!y) break;
    x *= x;
  }

  return r;
}

int	mk_open_tty(const char *device, speed_t baud);
int	mk_wait_msg(const rotorcraft_conn_s *conn,
                const struct timeval *deadline);
int	mk_recv_msg(struct mk_channel_s *chan, bool block);
int	mk_send_msg(const struct mk_channel_s *chan, const char *fmt, ...);

#ifdef __cplusplus
extern "C" {
#endif

  int	mk_calibration_init(uint32_t sstill, uint32_t nposes, uint32_t sps,
                double tolerance);
  int	mk_calibration_collect(double temp, or_pose_estimator_state *imu_data,
                or_pose_estimator_state *mag_data, int32_t *still);
  int	mk_calibration_acc(double ascale[9], double abias[3]);
  int	mk_calibration_gyr(double gscale[9], double gbias[3]);
  int	mk_calibration_mag(double mscale[9], double mbias[3]);
  void	mk_calibration_fini(double stddeva[3], double stddevw[3],
                double stddevm[3], double *maxa, double *maxw, double *avgtemp,
                double *avga, double *avgw);
  void	mk_calibration_log(const char *path);

  void	mk_calibration_rotate(double r[9], double s[9]);
  void	mk_calibration_bias(double b1[3], double s[9], double b[3]);

#ifdef __cplusplus
}
#endif

#define rc_neqexts(t, u)                                                \
  (((t).nsec != (u).nsec || (t).sec != (u).sec) ? ((t) = (u), 1) : 0)

#endif /* H_ROTORCRAFT_CODELS */
