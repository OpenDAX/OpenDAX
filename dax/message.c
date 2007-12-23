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

 * This file contains the messaging code for the OpenDAX core
 */


#include <message.h>
#include <func.h>
#include <module.h>
#include <tagbase.h>

#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>

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
int msg_tag_mask_write(dax_message *msg);
int msg_mod_get(dax_message *msg);


/* Generic message sending function.  If size is zero then it is assumed that an error
   is being sent to the module.  In that case payload should point to a single int that
   indicates the error */
static int _message_send(long int module, int command, void *payload, size_t size) {
    static u_int32_t count = 0;
    dax_message outmsg;
    size_t newsize;
    int result;
    
    outmsg.module = module;
    outmsg.command = command;
    outmsg.pid = getpid();
    outmsg.size = size;
    if(size) { /* Sending a normal response */
        memcpy(outmsg.data, payload, size);
        newsize = size;
    } else { /* Send error response */
        memcpy(outmsg.data, payload, sizeof(int));
        newsize = sizeof(int);
    }
    /* Only send messages to modules that we know about */
    if(module_get_pid(module)) {
        /* TODO: need to handle the case where the system call returns because of a signal.
        This msgsnd will block if the queue is full and a signal will bail us out.
        This may be good this may not but it'll need to be handled here somehow.
        also need to handle errors returned here*/
        result = msgsnd(__msqid, (struct msgbuff *)(&outmsg), MSG_HDR_SIZE + newsize, 0);
        
        count++;
        tag_write_bytes(STAT_MSG_SENT, &count, sizeof(u_int32_t));
    } else {
        result = -1;
    }
    return result;
}


/* Creates and sets up the main message queue for the program.
   It's a fatal error if we cannot create this queue */
int msg_setup_queue(void) {
    /* TODO: Do we bail if the queue exists or use an existing one?  The latter for debugging. */
    __msqid = msgget(DAX_IPC_KEY, (IPC_CREAT | 0660));
    //__msqid = msgget(DAX_IPC_KEY, (IPC_CREAT | IPC_EXCL | 0660));
    if(__msqid < 0) {
        __msqid=0;
        xfatal("Message Queue Cannot Be Created - %s",strerror(errno));
    }
    xlog(2,"Message Queue Created - id = %d",__msqid);
    /* The functions are added to an array of function pointers with their
        messsage type used as the index.  This makes it really easy to call
        the handler functions from the messaging thread. */
    cmd_arr[MSG_MOD_REG]    = &msg_mod_register;
    cmd_arr[MSG_TAG_ADD]    = &msg_tag_add;
    cmd_arr[MSG_TAG_DEL]    = &msg_tag_del;
    cmd_arr[MSG_TAG_GET]    = &msg_tag_get;
    cmd_arr[MSG_TAG_LIST]   = &msg_tag_list;
    cmd_arr[MSG_TAG_READ]   = &msg_tag_read;
    cmd_arr[MSG_TAG_WRITE]  = &msg_tag_write;
    cmd_arr[MSG_TAG_MWRITE] = &msg_tag_mask_write;
    cmd_arr[MSG_MOD_GET]    = &msg_mod_get;

    return __msqid;
}

/* This destroys the queue if it was created */
void msg_destroy_queue(void) {
    int result=0;
    
    if(__msqid) {
        result = msgctl(__msqid,IPC_RMID,NULL);
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
    static u_int32_t count = 0;
    dax_message daxmsg;
    int result;
    result = msgrcv(__msqid,(struct msgbuff *)(&daxmsg),sizeof(dax_message),1,0);
    if(result < 0) {
        xerror("msg_receive - %s",strerror(errno));
        return result;
    }
    count++;
    tag_write_bytes(STAT_MSG_RCVD, &count, sizeof(u_int32_t));
    if(result < (MSG_HDR_SIZE + daxmsg.size)) {
        xerror("message received is not of the specified size.");
        return -1;
    }
    if(daxmsg.command > 0 && daxmsg.command <= NUM_COMMANDS) {
        /* Call the command function from the array */
        (*cmd_arr[daxmsg.command])(&daxmsg);
    } else {
        xerror("unknown message command received %d",daxmsg.command);
        return -2;
    }
    return 0;
}

/* Each of these functions are matched to a message command.  These are called
   from and array of function pointers so there should be one for each command defined. */
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
    dax_tag tag; /* We use a structure within the data[] area of the message */
    handle_t handle;
    
    xlog(10,"Tag Add Message from %d", msg->pid);
    memcpy(tag.name, msg->data, sizeof(dax_tag) - sizeof(handle_t));
    handle = tag_add(tag.name, tag.type, tag.count);
    /* TODO: Gotta handle the error condition here or the module locks up waiting on the message */
    
    if(handle >= 0) {
        _message_send(msg->pid, MSG_TAG_ADD, &handle, sizeof(handle_t));
    } else {
        _message_send(msg->pid, MSG_TAG_ADD, &handle, 0);
        return -1; /* do I need this????? */
    }
    return 0;
}

int msg_tag_del(dax_message *msg) {
    xlog(10,"Tag Delete Message from %d",msg->pid);
    return 0;
}

/* This function retrieves a tag from the database.  If the size is
   a single int then it returns the tag at that index if the size is
   the DAX_TAGNAME_SIZE then it uses that as the name and returns that
   tag. */

/* TODO: Need to handle cases where the name isn't found and where the 
   index requested isn't within bounds */
int msg_tag_get(dax_message *msg) {
    int result, index;
    dax_tag *tag;
    
    if(msg->size == sizeof(int)) { /* Is it a string or index */
        index = *((int *)msg->data); /* cast void * -> int * then indirect */
        tag = tag_get_index(index); /* get the index */
        if(tag == NULL) result = ERR_ARG;
        xlog(10, "Tag Get Message from %d for index %d",msg->pid,index);
    } else {
        ((char *)msg->data)[DAX_TAGNAME_SIZE] = 0x00; /* Just to avoid trouble */
        tag = tag_get_name((char *)msg->data);
        if(tag == NULL) result = ERR_NOTFOUND;
        xlog(10, "Tag Get Message from %d for name %s", msg->pid, (char *)msg->data);
    }
    if(tag) {
        _message_send(msg->pid, MSG_TAG_GET, tag, sizeof(dax_tag));
        xlog(10,"Returned tag to calling module %d",msg->pid);
    } else {
        _message_send(msg->pid, MSG_TAG_GET, &result, 0);
        xlog(10,"Bad tag query for MSG_TAG_GET");
    }
    return 0;
}

int msg_tag_list(dax_message *msg) {
    xlog(10,"Tag List Message from %d",msg->pid);
    return 0;
}

/* The first part of the payload of the message is the handle
   of the tag that we want to read and the next part is the size
   of the buffer that we want to read */
int msg_tag_read(dax_message *msg) {
    char data[MSG_DATA_SIZE];
    handle_t handle;
    size_t size;
    
	/* These crazy cast move data things are nuts.  They work but their nuts. */
    size = *(size_t *)((dax_tag_message *)msg->data)->data;
    handle = ((dax_tag_message *)msg->data)->handle;
    
	xlog(10,"Tag Read Message from %d, handle 0x%X, size %d",msg->pid,handle,size);
    
    if(tag_read_bytes(handle, data, size) == size) { /* Good read from tagbase */
        _message_send(msg->pid, MSG_TAG_READ, &data, size);
    } else { /* Send Error */
        _message_send(msg->pid, MSG_TAG_READ, &data, 0);
    }
    return 0;
}

/* Generic write message */
int msg_tag_write(dax_message *msg) {
    handle_t handle;
    void *data;
    size_t size;
    
    size = msg->size - sizeof(handle_t);
    handle = ((dax_tag_message *)msg->data)->handle;
    data = ((dax_tag_message *)msg->data)->data;

	xlog(10,"Tag Write Message from module %d, handle 0x%X, size %d", msg->pid, handle, size);

    /* TODO: Need error check here */
    if(tag_write_bytes(handle,data,size) != size) {
        xerror("Unable to write tag 0x%X with size %d",handle, size);
    }
    return 0;
}

/* Generic write with bit mask */
int msg_tag_mask_write(dax_message *msg) {
    handle_t handle;
    void *data;
	void *mask;
    size_t size;
    
    size = (msg->size - sizeof(handle_t)) / 2;
    handle = ((dax_tag_message *)msg->data)->handle;
    data = ((dax_tag_message *)msg->data)->data;
	mask = (char *)data + size;
	
	xlog(10,"Tag Mask Write Message from module %d, handle 0x%X, size %d", msg->pid, handle, size);
	
	/* TODO: Need error check here */
    if(tag_mask_write(handle, data, mask, size) != size) {
        xerror("Unable to write tag 0x%X with size %d",handle, size);
    }
    return 0;
}

int msg_mod_get(dax_message *msg) {
    xlog(10,"Get Module Handle Message from %d",msg->pid);
    return 0;
}

