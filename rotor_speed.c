/** Codel my_set_all_rotor_velocity.
 *
 * Manually set velocity of all rotors at once.
 *
 * @param conn        Rotorcraft connection (channels to ESCs).
 * @param rotor_data  Rotor states.
 * @param velocities  Array of desired velocities (length = or_rotorcraft_max_rotors).
 * @param self        Genom context.
 *
 * Throws rotorcraft_e_connection, rotorcraft_e_rotor_failure.
 */
genom_event
my_set_all_rotor_velocity(const rotorcraft_conn_s *conn,
                          const rotorcraft_ids_rotor_data_s rotor_data[8],
                          const double velocities[or_rotorcraft_max_rotors],
                          const genom_context self)
{
  size_t i, m;
  double vbuf[or_rotorcraft_max_rotors];

  if (!conn) return rotorcraft_e_connection(self);

  // Copy and clamp velocities
  for (i = 0; i < or_rotorcraft_max_rotors; i++) {
    if (rotor_data[i].state.disabled || rotor_data[i].state.emerg) {
      rotorcraft_e_rotor_failure_detail e;
      e.id = 1 + i;
      return rotorcraft_e_rotor_failure(&e, self);
    }

    // Clamp (example: ±2000 rad/s ~ ±20k RPM)
    if (velocities[i] < -2000.0) vbuf[i] = -2000.0;
    else if (velocities[i] > 2000.0) vbuf[i] = 2000.0;
    else vbuf[i] = velocities[i];
  }

  // Send per channel
  for (m = 0; m < conn->n; m++) {
    uint16_t n = conn->chan[m].maxid - conn->chan[m].minid + 1;

    // Slice velocities for this channel’s rotor IDs
    mk_send_msg(&conn->chan[m], "v%@", &vbuf[conn->chan[m].minid - 1], n);
  }

  return rotorcraft_ether;
}
