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


 *  This file contains the global macros, includes and definitions for
 *  all the files in the entire system.
 */

#ifndef __COMMON_H
#define __COMMON_H

#include <config.h>

/* including all this is just easier. */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <assert.h>
#include <string.h>
#include <strings.h>
#include <sys/select.h>
#ifndef FD_COPY
  #define FD_COPY(f, t) (void)(*(t) = *(f))
#endif


/* Global definitions. These can be overriden with a -D arguments
 * in the Makefile */
#ifndef PID_FILE_PATH
  #define PID_FILE_PATH "/var/run"
#endif /* !PID_FILE_PATH */

#ifndef ETC_DIR
//  #define ETC_DIR "/etc/opendax"
  #define ETC_DIR "."
#endif

#define DAX_64_ONES 0xFFFFFFFFFFFFFFFFULL

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#ifndef MIN
  #define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
  #define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#endif
#ifndef ABS
  #define ABS(a)     (((a) < 0) ? -(a) : (a))
#endif

/* These are conditionally compiled debug statements. */
#ifdef DEBUG
# define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
# define DF(...) printf("%s:%d %s() - ", __FILENAME__, __LINE__, __func__); printf(__VA_ARGS__); printf("\n");
# define DAX_DEBUG(x) dax_debug(5, x);
# define DAX_DEBUG2(x, y) dax_debug(5, x, y);
#else
# define DF(...)
# define DAX_DEBUG(x)
# define DAX_DEBUG2(x, y)
#endif

#endif
