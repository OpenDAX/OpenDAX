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
 *

 * This file contains the messaging code for the library
 */
 
#include <common.h>
#include <opendax.h>
#include <dax/message.h>
#include <dax/tagbase.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>

static int __msqid;

int _message_send(long int module,int command,void *payload, size_t size) {
    dax_message outmsg;
    int result;
    outmsg.module=module;
    outmsg.command=command;
    outmsg.pid=getpid();
    outmsg.size=size;
    memcpy(outmsg.data,payload,size);
    result = msgsnd(__msqid,(struct msgbuff *)(&outmsg),MSG_SIZE + size,0);
    /* TODO: Yes this is redundant, the optimizer will destroy result but I may need to handle the
       case where the system call returns because of a signal.  This msgsnd will block if the
       queue is full and a signal will bail us out.  This may be good this may not but it'll
       need to be handled here somehow. */
    return result;
}

/* This function waits for a message with the given command to come in. If
   a message of another command comes in it will send that message out to
   an asynchronous command handler.  This is due to a race condition that could
   happen if someone puts a message in the queue after we send a request but
   before we retrieve the result. */
int _message_recv(int command, void *payload, size_t *size) {
    dax_message inmsg;
    int result,done;
    
    done=0;
    while(!done) {
        result=msgrcv(__msqid,(struct msgbuff *)(&inmsg),MSG_SIZE+DAX_MSG_SZ,(long)getpid(),0);
        if(inmsg.command==command) {
            done=1;
            *size = (size_t)(result-MSG_SIZE);
            /* TODO: Might check the calculated size vs. the size sent in the message.
                Its no checksum but it'll tell if something ain't right. */
            memcpy(payload,inmsg.data,*size);
        } else {
            printf("Some other message received\n");
            /* TODO: Insert the async message handling logic here */
        }
    }
    return 0;
}

/* TODO: We need some kind of asynchronous message handler.  It should not
block if there are no messages for us in the queue.  Now when does it need
to be called?  It might depend on the module. */

/* Connects to the message queue and sends a registration to the core */
static int mod_reg(char *name) {
    size_t size;
    int result;
    
    __msqid=msgget(DAX_IPC_KEY,0660);
    if(__msqid < 0) {
        return ERR_NO_QUEUE;
    }
    if(name) {
        if(strlen(name) > 254) {
            return ERR_2BIG;
        }
        size=strlen(name)+1;
    } else {
        size=0;
    }
    /* TODO: this call will block if queue is full.  Should also check
             for exit status that would be caused by signals EINTR.  Now do
             we return on EINTR or do we wait in a loop here? */
    result=_message_send(1,MSG_MOD_REG,name,size);
    if(result) { 
        return -2;
    }
    return 0;
}

int dax_mod_register(char *name) {
    return mod_reg(name);
}

int dax_mod_unregister(void) {
    return mod_reg(NULL);
}

/* Sends a message to dax to add a tag.  The payload is basically the
   tagname without the handle. */
handle_t dax_tag_add(char *name,unsigned int type, unsigned int count) {
    dax_tag tag;
    size_t size;
    int result;
    
    if(name) {
        if(strlen(name) > DAX_TAGNAME_SIZE) {
            return ERR_2BIG;
        }
        size=sizeof(dax_tag)-sizeof(handle_t);
        /* TODO Need to do some more error checking here */
        strcpy(tag.name,name);
        tag.type=type;
        tag.count=count;
    } else {
        return -1;
    }
    result=_message_send(1,MSG_TAG_ADD,&(tag.name),size);
    if(result) { 
        return -2;
    }
    /*  We're putting a lot of faith in the sending function here.  That
        might be okay in this instance */
    result= _message_recv(MSG_TAG_ADD,&(tag.handle),&size);
    printf("Tag.handle = 0x%x\n",tag.handle);
    return tag.handle;
}

/* This is a type neutral way to just write bytes to the data table */
void dax_tag_read_bytes(handle_t handle, void *data, size_t size) {
    
}