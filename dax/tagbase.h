/*  OpenDAX - An open source data acquisition and control system 
 *  Copyright (c) 1997 Phil Birkelbach
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
 
 * This is the header file for the tagname database handling routines
 */

#include <dax/daxtypes.h>
#include <dax/libcommon.h>
#include <opendax.h>

#ifndef __TAGBASE_H
#define __TAGBASE_H

/* This stuff may be better to belong in the configuration */

/* This is the initial size of the tagname list array */
#ifndef DAX_TAGLIST_SIZE
 #define DAX_TAGLIST_SIZE 1024
#endif

/* This is the size that the tagname list array will grow when
   the size is exceeded */
#ifndef DAX_TAGLIST_INC
 #define DAX_TAGLIST_INC 1024
#endif

/* The initial size of the database */
#ifndef DAX_DATABASE_SIZE
 #define DAX_DATABASE_SIZE 1024
#endif

/* This is the increment by which the database will grow when
   the size is exceeded */
#ifndef DAX_DATABASE_INC
 #define DAX_DATABASE_INC 1024
#endif


/* Define Handles for _status register points */
#define STATUS_SIZE   4
#define STAT_MSG_RCVD 0
#define STAT_MSG_SENT 32
#define STAT_DB_SIZE  64
#define STAT_TAG_CNT 96

struct mod_list {
    dax_module *module;
    struct mod_list *next;
};

typedef struct dax_event_t {
    int id;
    handle_t handle;
    size_t size;
    u_int32_t checksum;
    struct mod_list *notify;
    struct dax_event_t *next;
} _dax_event;

/* This is the internal structure for the tag array. */
typedef struct {
    unsigned int type;
    unsigned int count;
    char *name;
    _dax_event *events;
    void *data;
} _dax_tag_db;

typedef struct {
    /* TODO: Name's size is no longer fixed, should it be?
             It still is in the library.  Let's leave it for now?? */
    char *name;
    int handle;
} _dax_tag_index;

void initialize_tagbase(void);
handle_t tag_add(char *name, unsigned int type, unsigned int count);
int tag_del(char *name);
int tag_get_name(char *, dax_tag *);
int tag_get_index(int, dax_tag *);

/*
handle_t tag_get_handle(char *name);
int tag_read_bytes(handle_t handle, void *data, size_t size);
int tag_write_bytes(handle_t handle, void *data, size_t size);
int tag_mask_write(handle_t handle, void *data, void *mask, size_t size);
int event_add(handle_t handle, size_t size, dax_module *module);
int event_del(int id);
*/

#define DAX_DIAG
#ifdef DAX_DIAG
/* Diagnostic functions: should not be compiled in production */
void diag_list_tags(void);

#endif /* DAX_DIAG */

#endif /* !__TAGBASE_H */
