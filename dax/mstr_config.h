/*  OpenDAX - An open source data acquisition and control system 
 *  Copyright (c) 2007 Phil Birkelbach
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 
 *  Header file for opendax configuration
 */

#ifndef __MSTR_CONFIG_H
#define __MSTR_CONFIG_H

#include <common.h>
#include <libcommon.h>
#include <process.h>

/* If set to zero the program will run in the foreground
 * by default.  Otherwise it will go to the background */
#ifndef DEFAULT_DAEMONIZE
#  define DEFAULT_DAEMONIZE 0
#endif

#ifndef DEFAULT_PID
#  define DEFAULT_PID "/var/run/opendax.pid"
#endif

#ifndef DEFAULT_SERVER
#  define DEFAULT_SERVER "local";
#endif

#ifndef DEFAULT_PORT
#  define DEFAULT_PORT 7777;
#endif


int opt_configure(int argc, const char *argv[]);

/* These functions return the configuration parameters */
int opt_daemonize(void);    /* Whether or not to go to the background */
char *opt_statustag(void);
char *opt_pidfile(void);
char *opt_socketname(void);


#endif /* !__OPTIONS_H */
