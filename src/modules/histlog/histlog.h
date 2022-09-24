/*  OpenDAX - An open source data acquisition and control system 
 *  Copyright (c) 2022 Phil Birkelbach
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
 *  Main header file for the OpenDAX Historical Logging module
 */

#ifndef __HISTLOG_H_
#define __HISTLOG_H_

#include <common.h>
#include <opendax.h>

#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#define ABS(a)     (((a) < 0) ? -(a) : (a))

typedef void tag_object;

/* Linked list structure for the tags that we will be writing to the logger */
typedef struct tag_config {
    char *name;
    uint8_t status;   /* 0 if we still need to be configured 1 otherwise */
    tag_handle h;
    uint32_t trigger;  /* What triggers a write to the logger plugin */
    tag_object *tag;   /* plugin specific tag object */
    double timeout;    /* length of time that we expect to see tag updates from server */
    const char *attributes; /* string that represents plugin specific attributes */
    double trigger_value; /* value that the on change trigger uses for calculations */
    void *cmpvalue;  /* This is the last value that was written (used for compare) */
    void *lastvalue; /* last value of the tag that we got from the tag server */
    double lasttimestamp; /* The timestamp of the .lastvalue */
    int lastgood;         /* A flag to tell us that we wrote the last value to the database */
    struct tag_config *next;
} tag_config;

#define ON_CHANGE  0x01
#define ON_WRITE   0x02

/* histutil.c - Common utility functions */
double hist_gettime(void);

/* histopt.c - Configuration option functions */
int histlog_configure(int argc,char *argv[]);

/* plugin.c - Plugin functions */
int plugin_load(char *file);

/* These functions should be implemented in the plugin library */
extern tag_object *(*add_tag)(const char *tagname, uint32_t type, const char *attributes);
extern int (*free_tag)(tag_object *tag);
extern int (*write_data)(tag_object *tag, void *value, double timestamp);
extern int (*flush_data)(void);

#endif
