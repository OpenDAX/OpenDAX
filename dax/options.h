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

#ifndef __OPTIONS_H
#define __OPTIONS_H

#ifndef MAX_LINE_LENGTH
  #define MAX_LINE_LENGTH 100
#endif

#ifndef DEFAULT_PID
  #define DEFAULT_PID "/var/run/opendax.pid"
#endif

struct Config {
    char *pidfile;
    //--char ipaddress[16];
    //--unsigned short port;  /* TCP Port */ //There is probably a better datatype
    char *configfile;
    char *tagname;
    u_int8_t daemonize;
    unsigned int tablesize;
};


int dax_configure(int argc, const char *argv[]);

#endif /* !__OPTIONS_H */
