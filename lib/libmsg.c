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

 * This file contains the messaging code for the library
 */
 
#include <common.h>
#include <dcs/tagbase.h>
#include <opendcs.h>
#include <dcs/message.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>

static int __msqid;

/* Connects to the message queue and sends a registration to the core */
static int mod_reg(char *name) {
    dcs_message outmsg;
    int result;
    
    __msqid=msgget(DCS_IPC_KEY,0660);
    if(__msqid < 0) {
        return ERR_NO_QUEUE;
    }
    outmsg.module=1;
    outmsg.command=MSG_MOD_REG;
    outmsg.pid=getpid();
    if(name) {
        if(strlen(name) > 254) {
            return ERR_2BIG;
        }
        outmsg.size=strlen(name)+1;
        strcpy(outmsg.data,name);
    } else {
        outmsg.size=0;
    }
    /* TODO: this call will block if queue is full.  Should also check
             for exit status that would be caused by signals EINTR.  Now do
             we return on EINTR or do we wait in a loop here? */
    result=msgsnd(__msqid,(struct msgbuff *)(&outmsg),MSG_SIZE + sizeof(char)*outmsg.size,0);
    if(result) { 
        return -2;
    }
    return 0;
}

int dcs_mod_register(char *name) {
    return mod_reg(name);
}

int dcs_mod_unregister(void) {
    return mod_reg(NULL);
}

handle_t dcs_tag_add(char *name,unsigned int type, unsigned int count) {
    dcs_message outmsg;
    dcs_tag tag;
    int result;
    
    outmsg.module=1;
    outmsg.command=MSG_TAG_ADD;
    outmsg.pid=getpid();
    if(name) {
        if(strlen(name) > DCS_TAGNAME_SIZE) {
            return ERR_2BIG;
        }
        outmsg.size=sizeof(dcs_tag)-sizeof(handle_t);
        /* TODO Need to do some more error checking here */
        strcpy(tag.name,name);
        tag.type=type;
        tag.count=count;
    } else {
        return -1;
    }
    memcpy(outmsg.data,&(tag.name),outmsg.size);
    result=msgsnd(__msqid,(struct msgbuff *)(&outmsg),MSG_SIZE + sizeof(char)*outmsg.size,0);
    if(result) { 
        return -2;
    }
    /* Might need to centralize the receipt of messages */
    result=msgrcv(__msqid,&outmsg,MSG_SIZE+sizeof(handle_t),(long)getpid(),0);
    return 0;
}
