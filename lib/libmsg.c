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

 * This file contains libdax functions that need to send messages to the queue
 */
 
#include <libdax.h>
#include <dax/libcommon.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>


static int __msqid;

static int _message_send(long int module, int command, void *payload, size_t size) {
    dax_message outmsg;
    int result;
    outmsg.module = module;
    outmsg.command = command;
    outmsg.pid = getpid();
    outmsg.size = size;
    memcpy(outmsg.data, payload, size);
    result = msgsnd(__msqid, (struct msgbuff *)(&outmsg), MSG_HDR_SIZE + size, 0);
    /* TODO: need to handle the case where the system call returns because of a signal.
        This msgsnd will block if the queue is full and a signal will bail us out.
        This may be good this may not but it'll need to be handled here somehow.
        also need to handle errors returned here*/
    if(result) {
        dax_error("msgsnd() returned %d", result);
        return ERR_MSG_SEND;
    }
    return 0;
}

/* This function waits for a message with the given command to come in. If
   a message of another command comes in it will send that message out to
   an asynchronous command handler.  This is due to a race condition that could
   happen if someone puts a message in the queue after we send a request but
   before we retrieve the result. */
/* TODO: I suppose that it's possible for a module to hang here.  Perhaps opendax
   can send a wake up message every now and then so this function can timeout */
static int _message_recv(int command, void *payload, size_t *size) {
    dax_message inmsg;
    int result,done;
    
    done = 0;
    while(!done) {
        result = msgrcv(__msqid,(struct msgbuff *)(&inmsg), DAX_MSGMAX, (long)getpid(), 0);
        if(inmsg.command == command) {
            done = 1;
            *size = (size_t)(result - MSG_HDR_SIZE);
            memcpy(payload, inmsg.data, *size);
            if( inmsg.size == 0 ) { /* If size is zero then it's an error response */
                /* return the error code found in the data portion */
                return *((int *)inmsg.data);
            }
        } else {
            dax_debug(2, "Asynchronous message received\n");
            /* TODO: Insert the async message handling logic here */
        }
        /* TODO: Also need to handle cases when signals or errors happen here */
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
    
    __msqid = msgget(DAX_IPC_KEY, 0660);
    if(__msqid < 0) {
        return ERR_NO_QUEUE;
    }
    if(name) {
        if(strlen(name) > 254) {
            return ERR_2BIG;
        }
        size = strlen(name) + 1;
    } else {
        size = 0;
    }
    /* TODO: this call will block if queue is full.  Should also check
             for exit status that would be caused by signals EINTR.  Now do
             we return on EINTR or do we wait in a loop here?  Probably should
             arrange to have an alarm signal sent to us after a few seconds so
             this will have some kind of timeout. */
    result = _message_send(1, MSG_MOD_REG, name, size);
    if(result) { 
        return -2;
    }
    return 0;
}

int dax_mod_register(char *name) {
    dax_debug(10,"Sending registration for name - %s",name);
    return mod_reg(name);
}

int dax_mod_unregister(void) {
    dax_debug(10,"Sending un-registration");
    return mod_reg(NULL);
}

/* Sends a message to dax to add a tag.  The payload is basically the
   tagname without the handle. */
handle_t dax_tag_add(char *name, unsigned int type, unsigned int count) {
    dax_tag tag;
    size_t size;
    int result;
    
    if(count == 0) return ERR_ARG;
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
    result = _message_send(1, MSG_TAG_ADD, &(tag.name),size);
    if(result) { 
        return -2;
    }
    /*  We're putting a lot of faith in the sending function here.  That
        might be okay in this instance */
    result= _message_recv(MSG_TAG_ADD, &(tag.handle), &size);
    return tag.handle;
}


/* Get the tag by name. */
/* TODO: Resolve array and bit references in the tagname */
int dax_get_tag(char *name, dax_tag *tag) {
    int result;
    size_t size;
    
    /* TODO: Check the tag cache here */
    /* TODO: Some of these may need to be debug messages so they won't print */
    /* It seems like we should bounds check *name but _message-send will clip it! */
    result = _message_send(1, MSG_TAG_GET, name, DAX_TAGNAME_SIZE);
    if(result) {
        dax_error("Can't send MSG_TAG_GET message");
        return result;
    }
    result = _message_recv(MSG_TAG_GET, (void *)tag, &size);
    if(result) {
        dax_error("Problem receiving message MSG_TAG_GET");
    }
    return result;
}

/* This function takes the name argument and figures out the text part and puts
   that in 'tagname' then it sees if there is an index in [] or if there is
   a '.' for a bit index.  It puts those in the pointers that are passed */
static inline int parsetag(char *name, char *tagname, int *index, int *bit) {
    int n = 0;
    int i = 0;
    int tagend = 0;
    char test[10];
    *index = -1;
    *bit = -1;

    while(name[n] != '\0') {
        if(name[n] == '[') {
            tagend = n++;
            /* figure the tagindex here */
            while(name[n] != ']') {
                if(name[n] == '\0') return -1; /* Gotta get to a ']' before the end */
                test[i++] = name[n++];
                if(i == 9) return -1; /* Number is too long */
            }
            test[i] = '\0';
            *index = (int)strtol(test, NULL, 10);
            n++;
        } else if(name[n] == '.') {
            if(*index < 0) {
                tagend = n;
            }
            n++;
            *bit = (int)strtol(&name[n], NULL, 10);
            break;
        } else {
            n++;
        }
    }
    if(tagend) { /********BUFFER OVERFLOW POTENTIAL***************/
        strncpy(tagname, name, tagend);
        tagname[tagend] = '\0';
    } else {
        strcpy(tagname, name);
    }
    return 0;
}

/* These tag name getting routines will have to be rewritten when we get
the custom data types going.  Returns zero on success. */
int dax_tag_byname(char *name, dax_tag *tag) {
    dax_tag tag_test;
    char tagname[DAX_TAGNAME_SIZE + 1];
    int index, bit, result, size;
    
    if(parsetag(name, tagname, &index, &bit)) {
        return ERR_TAG_BAD;
    }
    /*********** TESTING ONLY **************/
    //--dax_debug(1,"parsetag returned tagname=\'%s\' index=%d bit=%d", tagname, index, bit);
    
    result = dax_get_tag(tagname, &tag_test);
    if(result) {
        return result;
    }
    
    /* This will happen if there are no subscripts to the tag */
    if(index < 0 && bit < 0) {
        memcpy(tag, &tag_test, sizeof(dax_tag));
        return 0;
    }
    /* Check that the given subscripts still fall within the tags. */
    size = TYPESIZE(tag_test.type) * tag_test.count;
    
    if( (index >=0 && index >= tag_test.count) || 
       (bit >= size || (index * TYPESIZE(tag_test.type) + bit >= size)) ||
       (index >= 0 && bit >= TYPESIZE(tag_test.type))) {
           return ERR_TAG_BAD;
    }
    /* if we make it this far all is good and we can calculate the type,
       size and handle of the tag */
    tag->name[0] = '\0'; /* We don't return the name */
    if(index < 0 && bit < 0) {
        index = 0; /* Make these zeros so we can do math on them */
        bit = 0;
        tag->type = tag_test.type;
        tag->count = tag_test.count;
    } else if(index < 0 && bit >=0) {
        index = 0;
        tag->type = DAX_BOOL;
        tag->count = 1;
    } else if(index >=0 && bit < 0) {
        bit = 0;
        tag->type = tag_test.type;
        tag->count = 1;
    } else  {
        tag->type = DAX_BOOL;
        tag->count = 1;
    }
    tag->handle = tag_test.handle + (index * TYPESIZE(tag_test.type) + bit);
    return 0;
}

/* Retrieves the tag by index.  */
int dax_tag_byindex(int index, dax_tag *tag) {
    int result;
    size_t size;
    result = _message_send(1, MSG_TAG_GET, &index, sizeof(int));
    if(result) {
        dax_error("Can't send MSG_TAG_GET message");
        return result;
    }
    result = _message_recv(MSG_TAG_GET, (void *)tag, &size);
    if(result == ERR_ARG) return ERR_ARG;
    return 0;
}

/* The following three functions are the core of the data handling
   system in Dax.  They are the raw reading and writing functions.
   Each takes a handle, a buffer pointer, and a size in bytes.  */

/* TODO: This function should return some kind of error */
int dax_tag_read(handle_t handle, void *data, size_t size) {
    size_t n,count,m_size,sendsize,tmp;
    int result;
    struct Payload {
        handle_t handle;
        size_t size;
    } payload;
    /* This calculates the amount of data that we can send with a single message
        It subtracts a handle_t from the data size for use as the tag handle.*/
    m_size = MSG_DATA_SIZE;
    count=((size-1)/m_size)+1;
    for(n=0; n<count; n++) {
        if(n == (count-1)) { /* Last Packet */
            sendsize = size % m_size; /* What's left over */
        } else {
            sendsize = m_size;
        }
        payload.handle = handle + (m_size * 8 * n);
        payload.size = sendsize;
        result = _message_send(1,MSG_TAG_READ,(void *)&payload, sizeof(struct Payload));
        result = _message_recv(MSG_TAG_READ, &((u_int8_t *)data)[m_size * n], &tmp);
    }
    return result;
}

/* This is a type neutral way to just write bytes to the data table */
/* TODO: Do we return error on this one or not??? Maybe with some flags?? */
void dax_tag_write(handle_t handle, void *data, size_t size) {
    size_t n,count,m_size,sendsize;
    dax_tag_message msg;
    
    /* This calculates the amount of data that we can send with a single message
       It subtracts a handle_t from the data size for use as the tag handle.*/
    m_size = MSG_DATA_SIZE-sizeof(handle_t);
    count=((size-1)/m_size)+1;
    for(n=0;n<count;n++) {
        if(n == (count-1)) { /* Last Packet */
            sendsize = size % m_size; /* What's left over */
        } else {
            sendsize = m_size;
        }
        /* Write the data to the message structure */
        msg.handle = handle + (m_size * 8 * n);
        memcpy(msg.data,data+(m_size * n),sendsize);
        
        _message_send(1,MSG_TAG_WRITE,(void *)&msg,sendsize+sizeof(handle_t));
    }
}

/* TODO: is there an error condition that we need to deal with here?? */
void dax_tag_mask_write(handle_t handle, void *data, void *mask, size_t size) {
    size_t n,count,m_size,sendsize;
    dax_tag_message msg;
    
    /* This calculates the amount of data that we can send with a single message
       It subtracts a handle_t from the data size for use as the tag handle.*/
    /* TODO: Verify that all this works if m_size is odd */
	m_size = (MSG_DATA_SIZE-sizeof(handle_t)) / 2;
    count=((size-1)/m_size)+1;
    for(n=0;n<count;n++) {
        if(n == (count-1)) { /* Last Packet */
		    sendsize = (size % m_size); /* What's left over */
        } else {
            sendsize = m_size;
        }
		/* Write the data to the message structure */
        msg.handle = handle + (m_size * 8 * n);
        memcpy(msg.data, data+(m_size * n), sendsize);
        memcpy(msg.data + sendsize, mask+(m_size * n), sendsize);
        _message_send(1,MSG_TAG_MWRITE,(void *)&msg,(sendsize * 2)+sizeof(handle_t));
    }
}

/* Adds an event for the given tag.  The count is how many of those
   tags the event should effect. */
/* TODO: The count is not actually implemented yet. */
int dax_event_add(char *tagname, int count) {
    dax_tag tag;
    dax_event_message msg;
    int result, test;
    size_t size;
    
    result = dax_tag_byname(tagname, &tag);
    if(result) {
        return result;
    }
    
    msg.handle = tag.handle;
    if(tag.type == DAX_BOOL) {
        msg.size = tag.count;
    } else {
        msg.size = TYPESIZE(tag.type) / 8 * tag.count;
    }
    
    if(_message_send(1, MSG_EVNT_ADD, &msg, sizeof(dax_event_message))) {
        return ERR_MSG_SEND;
    } else {
        test = _message_recv(MSG_EVNT_ADD, &result, &size);
        if(test) return test;
    }       
    return result;
}

int dax_event_del(int id) {
    return 0;
}

int dax_event_get(int id) {
    return 0;
}
