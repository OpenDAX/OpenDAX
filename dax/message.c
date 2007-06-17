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

 * This file contains the messaging code
 */


#include <message.h>
#include <tagbase.h>
#include <opendcs.h>
#include <module.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <func.h>

static int __msqid;

/* This array holds the functions for each message command */
#define NUM_COMMANDS 10
int (*cmd_arr[NUM_COMMANDS])(dcs_message *) = {NULL};

int msg_mod_register(dcs_message *msg);
int msg_tag_add(dcs_message *msg);
int msg_tag_del(dcs_message *msg);
int msg_tag_get(dcs_message *msg);
int msg_tag_list(dcs_message *msg);
int msg_tag_read(dcs_message *msg);
int msg_tag_write(dcs_message *msg);
int msg_tag_masked_write(dcs_message *msg);
int msg_mod_get(dcs_message *msg);

void _send_handle(handle_t handle,pid_t pid);

/* Creates and sets up the main message queue for the program.
   It's a fatal error if we cannot create this queue */
int msg_setup_queue(void) {
    __msqid = msgget(DCS_IPC_KEY, (IPC_CREAT | 0660));
    //__msqid = msgget(DCS_IPC_KEY, (IPC_CREAT | IPC_EXCL | 0660));
    if(__msqid < 0) {
        __msqid=0;
        xfatal("Message Queue Cannot Be Created - %s",strerror(errno));
    }
    xlog(2,"Message Queue Created - id = %d",__msqid);

    cmd_arr[MSG_MOD_REG] = &msg_mod_register;
    cmd_arr[MSG_TAG_ADD] = &msg_tag_add;
    cmd_arr[MSG_TAG_DEL] = &msg_tag_del;
    cmd_arr[MSG_TAG_GET] = &msg_tag_get;
    cmd_arr[MSG_TAG_LIST]=&msg_tag_list;
    cmd_arr[MSG_TAG_READ]=&msg_tag_read;
    cmd_arr[MSG_TAG_WRITE]=&msg_tag_write;
    cmd_arr[MSG_TAG_MWRITE]=&msg_tag_masked_write;
    cmd_arr[MSG_MOD_GET]=&msg_mod_get;

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
    dcs_message dcsmsg;
    int result;
    result=msgrcv(__msqid,(struct msgbuff *)(&dcsmsg),sizeof(dcs_message),1,0);
    if(result < 0) {
        xerror("msg_receive - %s",strerror(errno));
        return result;
    }
    if(result < (sizeof(int)+sizeof(pid_t)+sizeof(size_t)+sizeof(char)*dcsmsg.size)) {
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

int msg_mod_register(dcs_message *msg) {
    if(msg->size) {
        xlog(4,"Registering Module %s pid = %d",msg->data,msg->pid);
        module_register(msg->data,msg->pid);
    } else {
        xlog(4,"Unregistering Module pid = %d",msg->pid);
        module_unregister(msg->pid);
    }
    return 0;
}

int msg_tag_add(dcs_message *msg) {
    dcs_tag tag;
    handle_t handle;
    memcpy(msg->data,tag.name,sizeof(dcs_tag)-sizeof(handle_t));
    handle=tag_add(tag.name,tag.type,tag.count);
    if(handle >= 0) {
        _send_handle(handle,msg->pid);
    }
    xlog(10,"Tag Add Message from %d",msg->pid);
    return 0;
}

int msg_tag_del(dcs_message *msg) {
    xlog(10,"Tag Delete Message from %d",msg->pid);
    return 0;
}

int msg_tag_get(dcs_message *msg) {
    xlog(10,"Tag Get Message from %d",msg->pid);
    return 0;
}

int msg_tag_list(dcs_message *msg) {
    xlog(10,"Tag List Message from %d",msg->pid);
    return 0;
}

int msg_tag_read(dcs_message *msg) {
    xlog(10,"Tag Read Message from %d",msg->pid);
    return 0;
}

int msg_tag_write(dcs_message *msg) {
    xlog(10,"Tag Write Message from %d",msg->pid);
    return 0;
}

int msg_tag_masked_write(dcs_message *msg) {
    xlog(10,"Tag Masked Write Message from %d",msg->pid);
    return 0;
}

int msg_mod_get(dcs_message *msg) {
    xlog(10,"Get Module Handle Message from %d",msg->pid);
    return 0;
}

void _send_handle(handle_t handle,pid_t pid) {
    dcs_message msg;
    int result;
    if(module_get_pid(pid)) {
        msg.module=pid;
        msg.command=MSG_TAG_GET;
        msg.pid=0; /* Doesn't really matter */
        *((handle_t *)msg.data)=handle; /* Type cast copy thing */
        result=msgsnd(__msqid,(struct msgbuff *)(&msg),MSG_SIZE + sizeof(handle_t),0);
        if(result)
            xerror("_send_handle() problem sending message");
    }
}
