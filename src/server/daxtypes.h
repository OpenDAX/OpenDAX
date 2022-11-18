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
 
 * This header contains the private type definitions for the opendax server.
 * For definitions common to the server and the library see libcommon.h
 */

#ifndef __DAXTYPES_H
#define __DAXTYPES_H

#include <sys/types.h>
#include <netinet/in.h>
#include <opendax.h>

/* Module Flags */
#define MFLAG_RESTART       0x01
#define MFLAG_OPENPIPES     0x02
#define MFLAG_REGISTER      0x04

/* Flag bits for the tag data groups */
#define GRP_FLAG_NOT_EMPTY  0x01

/* Tag groups are an array of handles in each module */
typedef struct tag_group_t {
    uint8_t flags;    /* option flags for the group */
    unsigned int size; /* amount of memory needed to transfer this group */
    uint8_t count;    /* number of members in this group */
    tag_handle *members;
} tag_group;

/* Modules are implemented as a circular doubly linked list */
typedef struct dax_Module {
    char *name;
    in_addr_t host;     /* the modules host id */
    unsigned int flags; /* Configuration Flags for the module */
    unsigned int state; /* Modules Current Running State */
    int fd;             /* The socket file descriptor for this module */
    tag_index tagindex; /* The index of the tag that represents this module */
    uint32_t timeout;  /* Module communication timeout. */
    time_t starttime;
    int event_count;
    tag_group *tag_groups; /* Array of tag group packet definitions */
    uint32_t groups_size;  /* Current size of the group array */
    struct dax_Module *next, *prev;
} dax_module;

/* These are the compound datatype definitions. */

/* This is the compound datatype member definition.  The 
 * members are represented as a linked list */
struct cdt_member {
    char *name;
    uint32_t type;
    uint32_t count;
    struct cdt_member *next;
};

typedef struct cdt_member cdt_member;

/* This is the structure that represents the container for each
 * datatype. */
struct datatype {
    char *name;
    uint8_t flags;
    unsigned int refcount; /* Number of tags of this type */
    cdt_member *members;
};

typedef struct datatype datatype;

#endif /* !__DAXTYPES_H */
