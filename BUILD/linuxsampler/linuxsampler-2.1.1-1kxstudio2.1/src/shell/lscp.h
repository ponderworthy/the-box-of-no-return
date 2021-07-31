/*
 * LSCP Shell
 *
 * Copyright (c) 2014 Christian Schoenebeck
 *
 * This program is part of LinuxSampler and released under the same terms.
 */

#ifndef LSCP_SHELL_H
#define LSCP_SHELL_H

// Turn this on if you need to debug something in the LSCP shell. It will cause
// a debug log file "lscp.log" to be created and written to on calls to
// lscpLog(). Note: the log file will be created / opened in the current working
// directory!
#define DEBUG_LSCP_SHELL 0

void lscpLog(const char* format, ...);

#endif // LSCP_SHELL_H
