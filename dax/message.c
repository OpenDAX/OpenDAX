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

 * This file contains the messaging code
 */


#include <message.h>
#include <tagbase.h>
#include <opendax.h>
#include <module.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <func.h>

static int __msqid;

/* This array holds the functions for each message command */
#define NUM_COMMANDS 10
int (*cmd_arr[NUM_COMMANDS])(dax_message *) = {NULL};

int msg_mod_register(dax_message *msg);
int msg_tag_add(dax_message *msg);
int msg_tag_del(dax_message *msg);
int msg_tag_get(dax_message *msg);
int msg_tag_list(dax_message *msg);
int msg_tag_read(dax_message *msg);
int msg_tag_write(dax_message *msg);
int msg_tag_masked_write(dax_message *msg);
int msg_mod_get(dax_message *msg);

void _send_handle(handle_t handle,pid_t pid);

/* Creates and sets up the main message queue for the program.
   It's a fatal error if we cannot create this queue */
int msg_setup_queue(void) {
    __msqid = msgget(DAX_IPC_KEY, (IPC_CREAT | 0660));
    //__msqid = msgget(DAX_IPC_KEY, (IPC_CREAT | IPC_EXCL | 0660));
    if(__msqid < 0) {
        __msqid=0;
        xfatal("Message Queue Cannot Be Created - %s",strerror(errno));
    }
    xlog(2,"Message Queue Created - id = %d",__msqid);

    cmd_arr[MSG_MOD_REG]    = &msg_mod_register;
    cmd_arr[MSG_TAG_ADD]    = &msg_tag_add;
    cmd_arr[MSG_TAG_DEL]    = &msg_tag_del;
    cmd_arr[MSG_TAG_GET]    = &msg_tag_get;
    cmd_arr[MSG_TAG_LIST]   = &msg_tag_list;
    cmd_arr[MSG_TAG_READ]   = &msg_tag_read;
    cmd_arr[MSG_TAG_WRITE]  = &msg_tag_write;
    cmd_arr[MSG_TAG_MWRITE] = &msg_tag_masked_write;
    cmd_arr[MSG_MOD_GET]    = &msg_mod_get;

    return __msqid;
}

/* This destroys the queue if it was created */
void msg_destroy_queue(void) {
    int result=0;
    
    if(__msqid) {
        result=msgctl(__msqid,IPC_RMID,NULL);
    }
    if(result) {
        xerror("Unable to delete message queue %d",__msqid);
    } else {
        xlog(2,"Message queue %d being destroyed",__msqid);
    }
}

/* This function blocks waiting for a message to be received.  Once a message
   is retrieved from the system the proper handling function is called */
int msg_receive(void) {
    dax_message dcsmsg;
    int result;
    result = msgrcv(__msqid,(struct msgbuff *)(&dcsmsg),sizeof(dax_message),1,0);
    if(result < 0) {
        xerror("msg_receive - %s",strerror(errno));
        return result;
    }
    if(result < (DAX_HDR_SIZE + dcsmsg.size)) {
        xerror("message received is not of the specified size.");
        return -1;
    }
    if(dcsmsg.command > 0 && dcsmsg.command <= NUM_COMMANDS) {
        /* Call the command function from the array */
        (*cmd_arr[dcsmsg.command])(&dcsmsg);
    } else {
        xerror("unknown message command received %d",dcsmsg.command);
        return -2;
    }
    return 0;
}

int msg_mod_register(dax_message *msg) {
    if(msg->size) {
        xlog(4,"Registering Module %s pid = %d",msg->data,msg->pid);
        module_register(msg->data,msg->pid);
    } else {
        xlog(4,"Unregistering Module pid = %d",msg->pid);
        module_unregister(msg->pid);
    }
    return 0;
}

int msg_tag_add(dax_message *msg) {
    dax_tag tag;
    handle_t handle;
    
    xlog(10,"Tag Add Message from %d",msg->pid);
    memcpy(tag.name,msg->data,sizeof(dax_tag)-sizeof(handle_t));
    handle=tag_add(tag.name,tag.type,tag.count);
    if(handle >= 0) {
        _send_handle(handle,msg->pid);
    }
    return 0;
}

int msg_tag_del(dax_message *msg) {
    xlog(10,"Tag Delete Message from %d",msg->pid);
    return 0;
}

int msg_tag_get(dax_message *msg) {
    xlog(10,"Tag Get Message from %d",msg->pid);
    return 0;
}

int msg_tag_list(dax_message *msg) {
    xlog(10,"Tag List Message from %d",msg->pid);
    return 0;
}

int msg_tag_read(dax_message *msg) {
    xlog(10,"Tag Read Message from %d",msg->pid);
    return 0;
}

int msg_tag_write(dax_message *msg) {
    xlog(10,"Tag Write Message from %d",msg->pid);
    return 0;
}

int msg_tag_masked_write(dax_message *msg) {
    xlog(10,"Tag Masked Write Message from %d",msg->pid);
    return 0;
}

int msg_mod_get(dax_message *msg) {
    xlog(10,"Get Module Handle Message from %d",msg->pid);
    return 0;
}

/* Probably should write a generic message handling function or two */
void _send_handle(handle_t handle,pid_t pid) {
    dax_message msg;
    int result;
    if(module_get_pid(pid)) {
        msg.module=pid;
        msg.command=MSG_TAG_ADD;
        msg.pid=0; /* Doesn't really matter what PID we send here */
        msg.size=sizeof(handle_t);
        *((handle_t *)msg.data)=handle; /* Type cast copy thing */
        result=msgsnd(__msqid,(struct msgbuff *)(&msg),DAX_HDR_SIZE + sizeof(handle_t),0);
        if(result)
            xerror("_send_handle() problem sending message");
    }
}
