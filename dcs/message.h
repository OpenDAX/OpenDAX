/*  opendcs - An open source distributed control system
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
 *

 *  Header file for the messaging code
 */
 
#ifndef __MESSAGE_H
#define __MESSAGE_H

#include <common.h>


/* Size for the DCS message buffer.  Do I really need this? */
#define DCS_MSG_SZ 1024
/* Message functions */
#define MSG_MOD_REG   0x0001 /* Register the module with the core */
#define MSG_TAG_ADD   0x0002 /* Add a tag */
#define MSG_TAG_DEL   0x0003 /* Delete a tag from the database */
#define MSG_TAG_GET   0x0004 /* Get the handle, type, size etc for the tag */
#define MSG_TAG_LIST  0x0005 /* Retrieve a list of all the tagnames */
#define MSG_TAG_READ  0x0006 /* Read the value of a tag */
#define MSG_TAG_WRITE 0x0007 /* Write the value to a tag */
#define MSG_MOD_GET   0x0008 /* Get the module handle by name */
/* More to come */

typedef struct Dcs_Message {
    long int handle;
    int function;
    size_t size; 
    char buff[DCS_MSG_SZ];
} dcs_message;

int msg_create_queue(void);
void msg_destroy_queue(void);
void msg_receive(void);

#endif /* !__MESSAGE_H */
