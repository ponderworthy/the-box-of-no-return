/************************************************************************/
/* Copyright (C) 2009 Lieven Moors                                      */
/*                                                                      */
/* This file is part of aj-snapshot.                                    */
/*                                                                      */
/* aj-snapshot is free software: you can redistribute it and/or modify  */
/* it under the terms of the GNU General Public License as published by */
/* the Free Software Foundation, either version 3 of the License, or    */
/* (at your option) any later version.                                  */
/*                                                                      */
/* aj-snapshot is distributed in the hope that it will be useful,       */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU General Public License for more details.                         */
/*                                                                      */
/* You should have received a copy of the GNU General Public License    */
/* along with aj-snapshot.  If not, see <http://www.gnu.org/licenses/>. */
/************************************************************************/

#ifndef AJ_SNAPSHOT_H
#define AJ_SNAPSHOT_H


#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <fnmatch.h>
#include <alsa/asoundlib.h>
#include <jack/jack.h>
#include <mxml.h>
#include <signal.h>
#include <pthread.h>

#include "aj-alsa.h"
#include "aj-file.h"
#include "aj-jack.h"
#include "aj-remove.h"

#define IGNORED_CLIENTS_MAX 50

#define VERBOSE(str) if(verbose) fprintf(stdout, str)

int is_ignored_client(const char *name); // function to check if string is name of ignored client.

#endif

