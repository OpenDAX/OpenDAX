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
 * opendax core and the library.  It's mostly messaging definitions.
 */

#ifndef __LIBCOMMON_H
#define __LIBCOMMON_H

#include <opendax.h>

/* Defines the maximum length of a tagname */
/* NOTE: This is duplicated in opendax.h so it should
         be changed in both places. */
/* TODO: This is probably a bad thing.  It should be constant
         here and then the library should recieve the size
         when it registers and then allocate each tag as
         they are needed. */
#ifndef DAX_TAGNAME_SIZE
 #define DAX_TAGNAME_SIZE 32
#endif

/* For now we'll use this as the key for all the SysV IPC stuff */
//--#define DAX_IPC_KEY 0x707070

/* Message functions */
#define MSG_MOD_REG    0x0001 /* Register the module with the core */
#define MSG_TAG_ADD    0x0002 /* Add a tag */
#define MSG_TAG_DEL    0x0003 /* Delete a tag from the database */
#define MSG_TAG_GET    0x0004 /* Get the handle, type, size etc for the tag */
#define MSG_TAG_LIST   0x0005 /* Retrieve a list of all the tagnames */
#define MSG_TAG_READ   0x0006 /* Read the value of a tag */
#define MSG_TAG_WRITE  0x0007 /* Write the value to a tag */
#define MSG_TAG_MWRITE 0x0008 /* Masked Write */
#define MSG_MOD_GET    0x0009 /* Get the module handle by name */
#define MSG_EVNT_ADD   0x000A /* Add an event to the taglist */
#define MSG_EVNT_DEL   0x000B /* Delete an event */
#define MSG_EVNT_GET   0x000C /* Get an event definition */
/* More to come */

#ifndef MSG_RESPONSE
#  define MSG_RESPONSE   0x100000000LL /* Flag for defining a response message */
#endif

/* Maximum size allowed for a single message */
/* TODO: Should this be configurable */
#ifndef DAX_MSGMAX
#  define DAX_MSGMAX 4096
#endif

/* This defines the size of the message minus the actual data */
#define MSG_HDR_SIZE (sizeof(int) + sizeof(size_t))
#define MSG_DATA_SIZE (DAX_MSGMAX - MSG_HDR_SIZE)
#define MSG_TAG_DATA_SIZE (MSG_DATA_SIZE - sizeof(handle_t))

/* This is a full sized message.  It's the largest message allowed to be sent in the queue */
typedef struct {
    u_int32_t size;     /* size of the data sent */
    //long int module; /* Destination module : Do we need this with the sockts??? */
    int command;     /* Which function to call */
    pid_t pid;       /* PID of the module that sent the message : Still needed???*/
    char data[MSG_DATA_SIZE];
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

#endif /* ! __LIBCOMMON_H */
