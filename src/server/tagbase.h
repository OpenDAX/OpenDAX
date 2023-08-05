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

#include <libcommon.h>
#include <opendax.h>
#include "daxtypes.h"
#include "virtualtag.h"


#ifndef __TAGBASE_H
#define __TAGBASE_H

/* This stuff may be better to belong in the configuration */

/* This is the initial size of the tagname list array */
#ifndef DAX_TAGLIST_SIZE
 #define DAX_TAGLIST_SIZE 1024
#endif

/* This is the amount that the tagname list array will grow when
 * the size is exceeded. */
#ifndef DAX_TAGLIST_INC
 #define DAX_TAGLIST_INC 1024
#endif

/* The initial size of the database */
#ifndef DAX_DATABASE_SIZE
 #define DAX_DATABASE_SIZE 1024
#endif


#ifndef DAX_DATATYPE_SIZE
# define DAX_DATATYPE_SIZE 10
#endif


/* This is the maximum number of mapping hops that we'll make before we print an
 * error and do some otheR drastic action. */
#define MAX_MAP_HOPS 128

/* database indexes for status tags. */
#define INDEX_TAGCOUNT   0
#define INDEX_LASTINDEX  1
#define INDEX_DBSIZE     2
#define INDEX_STARTED    3
#define INDEX_LASTMODULE 4

/* Offset for the Module tag */
#define MOD_ID_OFFSET 8
#define MOD_STAT_COMMAND_OFFSET 77  /* _module.xxx where xxx are the commands */

#define CDT_FLAGS_RETAINED 0x80;

typedef struct dax_event_t {
    int id;              /* Unique identifier for this event definition */

    int byte;            /* The byte offset where the data block starts */
    unsigned char bit;   /* The bit offset */
    int count;           /* The number of items represented by the handle */
    uint32_t size;      /* The total size of the data block in bytes */
    uint32_t options;   /* Options for the event */
    tag_type datatype;   /* The data type of the block */
    int eventtype;       /* The type of event */
    void *data;          /* Data given by module */
    void *test;          /* Internal data, depends on event type */
    dax_module *notify;  /* Module to be notified of this event */
    struct dax_event_t *next;
} _dax_event;

typedef struct dax_datamap_t {
    int id;
    tag_handle source;
    tag_handle dest;
    uint8_t *mask;
    struct dax_datamap_t *next;
} _dax_datamap;

/* This is the internal structure for the tag array. */
typedef struct {
    tag_type type;
    uint16_t attr;
    unsigned int count;
    char *name;
    int nextevent;           /* Counter for keeping track of event IDs */
    int nextmap;             /* Counter for keeping track of map IDs */
    _dax_event *events;      /* Linked list of events */
    _dax_datamap *mappings;  /* Linked list of mappings */
    uint8_t *data;
    uint8_t *omask;        /* Override mask pointer */
    uint8_t *odata;        /* Override data pointer */
    uint32_t ret_file_pointer; /* Pointer to the data area of the tag retention file */
} _dax_tag_db;

typedef struct {
    /* TODO: Name's size is no longer fixed, should it be?
             It still is in the library.  Let's leave it for now?? */
    char *name;
    int tag_idx;
} _dax_tag_index;

/* Tag Database Handling Functions */
void initialize_tagbase(void);
tag_index tag_add(char *name, tag_type type, uint32_t count, uint32_t attr);
int tag_set_attribute(tag_index index, uint32_t attr);
int tag_clr_attribute(tag_index index, uint32_t attr);

tag_index virtual_tag_add(char *name, tag_type type, unsigned int count, vfunction *rf, vfunction *wf);
int tag_del(tag_index idx);
int tag_get_name(char *, dax_tag *);
int tag_get_index(int, dax_tag *);
tag_index get_tagindex(void);
int is_tag_readonly(tag_index idx);
int is_tag_virtual(tag_index idx);
int is_tag_queue(tag_index idx);
int tag_get_size(tag_index idx);

/* Database reading and writing functions */
int tag_read(tag_index handle, int offset, void *data, int size);
int tag_write(tag_index handle, int offset, void *data, int size);
int tag_mask_write(tag_index handle, int offset, void *data, void *mask, int size);

/* Perform an atomic operation on the data */
int atomic_op(tag_handle h, void *data, uint16_t op);

/* Custom DataType functions */
tag_type cdt_create(char *str, int *error);
tag_type cdt_get_type(char *name);
char *cdt_get_name(tag_type type);
datatype *cdt_get_entry(tag_type type);
int type_size(tag_type type);
int serialize_datatype(tag_type type, char **str);

/* The event stuff is defined in events.c */
void event_check(tag_index idx, int offset, int size);
void event_del_check(tag_index idx);
int event_add(tag_handle h, int event_type, void *data, dax_module *module);
int event_del(int index, int id, dax_module *module);
int events_del_all(_dax_event *head);
int event_opt(int index, int id, uint32_t options, dax_module *module);
int events_cleanup(dax_module *module);

int map_add(tag_handle src, tag_handle dest);
int map_del(tag_index index, int id);
int map_del_all(_dax_datamap *head);
int map_check(tag_index idx, int offset, uint8_t *data, int size);

int override_add(tag_index idx, int offset, void *data, void *mask, int size);
int override_del(tag_index idx, int offset, void *mask, int size);
int override_get(tag_index idx, int offset, int size, void *data, void *mask);
int override_set(tag_index idx, uint8_t flag);


#define DAX_DIAG
#ifdef DAX_DIAG
/* Diagnostic functions: should not be compiled in production */
void diag_list_tags(void);

#endif /* DAX_DIAG */

#endif /* !__TAGBASE_H */
