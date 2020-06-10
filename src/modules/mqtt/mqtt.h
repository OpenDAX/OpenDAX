/*  OpenDAX - An open source data acquisition and control system 
 *  Copyright (c) 2020 Phil Birkelbach
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
 *
 *  Main header file for the OpenDAX MQTT module
 */

#include <common.h>
#include <opendax.h>
#include <signal.h>
#include <MQTTClient.h>

/* Initial size of the arrays */
#define SUB_START_SIZE 16
#define PUB_START_SIZE 16

#define CFG_RAW -1
#define CFG_STR -2
#define CFG_RE -3
#define CFG_WRITE -11
#define CFG_CHANGE -22
#define CFG_ANY_WRITE -11
#define CFG_ANY_CHANGE -22
#define CFG_ALL_WRITE -33

#define ENABLE_UNINIT 0 /* Hasn't been properly initialized */
#define ENABLE_GOOD   1 /* Good and running */
#define ENABLE_FAIL   2 /* Permanent failure cannot be fixed */

/* Contains all the information to identify a subscription */
typedef struct {
   u_int8_t enabled;
   char *topic;
   char **tagnames;  /* Array of tagnames */
   tag_handle *h;   /* Array of handles to the tags */
   int tag_count;    /* Number of tagnames we are watching */
   u_int8_t qos;
   int format_type;
   char *format_str;
   u_int8_t binformat[8];
   int update_mode;
} subscriber_t;

/* Contains all the information to identify a publisher */
typedef struct {
   u_int8_t enabled;
   char *topic;
   char **tagnames;  /* Array of tagnames */
   tag_handle **h;   /* Array of handles to the tags */
   int tag_count;    /* Number of tagnames we are watching */
   u_int8_t qos;
   int format_type;
   char *format_str;
   u_int8_t binformat[8];
   int update_mode;
} publisher_t;


int configure(int argc, char *argv[]);
subscriber_t *get_sub_iter(void);
subscriber_t *get_sub(char *topic);

