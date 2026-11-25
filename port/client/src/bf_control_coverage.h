// The control socket's `coverage` part (PROTOCOL.md section 3) and the
// coverage report file of M9.5 (port/native/strings, bf_coverage.h): what of
// a session was not English. Port code (bf_compat_flags, in bf_port_main);
// bf_main.cpp calls it.
#ifndef BF_CONTROL_COVERAGE_H
#define BF_CONTROL_COVERAGE_H

// Registers the part; before bf_control_open.
void bf_control_coverage_install(void);

// At exit: writes the report to the file BF_COVERAGE names, if it is set.
void bf_control_coverage_write(void);

#endif
