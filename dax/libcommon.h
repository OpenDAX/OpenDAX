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
 
 * This header contains all of the type definitions that are common between the
 * opendax server and the library.  It's mostly messaging definitions.
 */

#ifndef __LIBCOMMON_H
#define __LIBCOMMON_H

#include <opendax.h>
#include <sys/types.h>

/* Message functions */
#define MSG_MOD_REG    0x0000 /* Register the module with the server */
#define MSG_TAG_ADD    0x0001 /* Add a tag */
#define MSG_TAG_DEL    0x0002 /* Delete a tag from the database */
#define MSG_TAG_GET    0x0003 /* Get the handle, type, size etc for the tag */
#define MSG_TAG_LIST   0x0004 /* Retrieve a list of all the tagnames */
#define MSG_TAG_READ   0x0005 /* Read the value of a tag */
#define MSG_TAG_WRITE  0x0006 /* Write the value to a tag */
#define MSG_TAG_MWRITE 0x0007 /* Masked Write */
#define MSG_MOD_GET    0x0008 /* Get the module handle by name */
#define MSG_EVNT_ADD   0x0009 /* Add an event to the taglist */
#define MSG_EVNT_DEL   0x000A /* Delete an event */
#define MSG_EVNT_GET   0x000B /* Get an event definition */
/* More to come */

#define MSG_RESPONSE   0x1000000LL /* Flag for defining a response message */
#define MSG_ERROR      0x2000000LL /* Flag for defining an error message */

/* These are flags for the registration command */
#define REGISTER_SYNC  0x01 /* Used to identify the synchronous socket during registration */
#define REGISTER_EVENT 0x02 /* Identifies the asynchronous event socket during registration */

/* These are the values that the registration system uses to 
   determine whether or not the module will have to reformat
   the data because of different machine architectures. */
/* TODO: I need to spend some time on these so that I know that
   they will actually be good tests. Integers are okay but I may
   want to put some thought into the floats.  Got to make sure
   that none of the bytes are the same or they can be mixed up and
   still work?? */
#define REG_TEST_INT    0xBCDE
#define REG_TEST_DINT   0x56789ABC
#define REG_TEST_LINT   0x123456789ABCDEF0LL
#define REG_TEST_REAL   3.14159265
#define REG_TEST_LREAL  -58765463.8766677

/* Subcommands for the MSG_TAG_GET command */
#define TAG_GET_NAME    0x01 /* Retrieve the tag by name */
#define TAG_GET_HANDLE  0x02 /* Retrieve the tag by it's handle */

/* Maximum size allowed for a single message */
#ifndef DAX_MSGMAX
#  define DAX_MSGMAX 4096
#endif

/* This defines the size of the message minus the actual data */
#define MSG_HDR_SIZE (sizeof(u_int32_t) + sizeof(u_int32_t))
#define MSG_DATA_SIZE (DAX_MSGMAX - MSG_HDR_SIZE)
#define MSG_TAG_DATA_SIZE (MSG_DATA_SIZE - sizeof(handle_t))

/* This is a full sized message.  It's the largest message allowed to be sent in the queue */
typedef struct {
    /* Message Header Stuff.  Changes here should be reflected in the 
       MSG_HDR_SIZE definition above */
    u_int32_t size;     /* size of the data sent */
    u_int32_t command;  /* Which function to call */
    /* Main data payload */
    char data[MSG_DATA_SIZE];
    /* The following stuff isn't in the socket message */
    int fd;             /* We'll use the fd to identify the module*/
} dax_message;

/* This datatype can be copied into the data[] area of the main dax_message when
 the first part of the message is a tag hangle. */

typedef struct {
    handle_t handle;
    char data[MSG_TAG_DATA_SIZE];
} dax_tag_message;

typedef struct {
    handle_t handle;
    size_t size;
} dax_event_message;

/* These are the custom datatype definitions. */

/* This is the custom datatype member definition.  The 
 * members are represented as a linked list */
typedef struct CDT_Member {
    char *name;
    unsigned int type;
    size_t count;
    struct CDT_Member *next;
} cdt_member;

/* This is the structure that represents the container for each
 * datatype. */
typedef struct {
    char *name;
    unsigned int refcount; /* Number of tags of this type */
    cdt_member *members;
} datatype;



#endif /* ! __LIBCOMMON_H */
