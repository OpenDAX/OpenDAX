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
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <math.h>


static int _sfd;   /* Server's File Descriptor */
static unsigned int _reformat; /* Flags to show how to reformat the incoming data */

static int _message_send(int command, void *payload, size_t size) {
    int result;
    char buff[DAX_MSGMAX];
    
    /* We always send the size and command in network order */
    ((u_int32_t *)buff)[0] = htonl(size + MSG_HDR_SIZE);
    ((u_int32_t *)buff)[1] = htonl(command);
    memcpy(&buff[MSG_HDR_SIZE], payload, size);
    
    /* TODO: We need to set some kind of timeout here.  This could block
       forever if something goes wrong.  It may be a signal or something too. */
    result = write(_sfd, buff, size + MSG_HDR_SIZE);
    
    if(result < 0) {
    /* TODO: Should we handle the case when this returns due to a signal */
        dax_error("_message_send: %s", strerror(errno));
        return ERR_MSG_SEND;
    }
    return 0;
}

/* This function waits for a message with the given command to come in. If
   a message of another command comes in it will send that message out to
   an asynchronous command handler.  This is due to a race condition that could
   happen if the server puts a message in the queue after we send a request but
   before we retrieve the result. */
static int _message_recv(int command, void *payload, int *size, int response) {
    char buff[DAX_MSGMAX];
    int index, done, msg_size, result;
    done = index = msg_size = 0;
    
    while( index < msg_size || index < MSG_HDR_SIZE) {
        result = read(_sfd, &buff[index], DAX_MSGMAX);
        //printf("M-read returned %d\n", result);
        //for(done = 0; done < result; done ++) {
        //    printf("0x%02X[%c] " , (unsigned char)buff[done], (unsigned char)buff[done]);
        //} printf("\n");
        if(result < 0) {
            dax_debug(LOG_COMM, "_message_recv read failed: %s", strerror(errno));
            return ERR_MSG_RECV;
        } else if(result == 0) {
            printf("TODO: I don't know what to do with read returning 0\n");
            return -1;
        } else {
            index += result;
        }
        
        if(index >= MSG_HDR_SIZE) {
            msg_size = ntohl(*(u_int32_t *)buff);
            if(msg_size > DAX_MSGMAX) {
                dax_debug(LOG_COMM, "_message_recv message size is too big");
                return ERR_MSG_BAD;
            }
        }
    }
    /* This gets the command out of the buffer */
    result = ntohl(*((u_int32_t *)&buff[4]));
    
    /* Test if the error flag is set and then return the error code */
    if(result == (command | MSG_ERROR)) {
        return stom_dint((*(int32_t *)&buff[8]));
    } else if(result == (command | (response ? MSG_RESPONSE : 0))) {
        if(size) {
            if(msg_size > *size) {
                return ERR_2BIG;
            } else {
                memcpy(payload, &buff[MSG_HDR_SIZE], msg_size - MSG_HDR_SIZE);
                *size = msg_size - MSG_HDR_SIZE; /* size is value result */
            }
        }
    } else { /* This is the command we wanted */
        printf("TODO: Whoa we got the wrong command\n");
    }
    return 0;
}

/* Send regsitration message to the server.  This message sends
   the name that we want to be, it sends the test data so the 
   server can decide if we need to pack our data.  The returned
   message should show whether we need to pack our data and the
   name that the server decided to give us.  The server can change
   our name if it's a duplicate. */
int dax_mod_register(char *name) {
    int fd, len;
    char buff[DAX_MSGMAX];
    struct sockaddr_un addr;

    dax_debug(LOG_COMM, "Sending registration for name - %s", name);
    
    /* TODO: Probably move the connection stuff to another function */
    /* create a UNIX domain stream socket */
    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
        return(-1);
    /* TODO: Gotta set a timeout for this stuff. */
    /* fill socket address structure with our address */
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, opt_get_socketname(), sizeof(addr.sun_path));
    
    len = offsetof(struct sockaddr_un, sun_path) + strlen(addr.sun_path);
    if (connect(fd, (struct sockaddr *)&addr, len) < 0) {
        dax_error("Unable to connect to socket - %s", strerror(errno));
        return ERR_NO_SOCKET;
    } else {
        _sfd = fd;
        dax_debug(LOG_COMM, "Connected to Server fd = %d", fd);
    }
    

/* TODO: Boundary check that a name that is longer than data size will
   be handled correctly. */
/* This is how much room the packing test data takes up in the message.
   It should be adjusted anytime the 'name' must move down in the message */
#define REG_HDR_SIZE 8
    /* Whatever is left of the message size can be used for the module name */
    len = strlen(name) + 1;
    if(len > (MSG_DATA_SIZE - REG_HDR_SIZE)) {
        len = MSG_DATA_SIZE - REG_HDR_SIZE;
        name[len] = '\0';
    }
    
    *((u_int32_t *)&buff[0]) = htonl(getpid());       /* 32 bits for the PID */
    strcpy(&buff[REG_HDR_SIZE], name);                /* The rest is the name */
    
    /* For registration we pack the data no matter what */
    _message_send(MSG_MOD_REG, buff, REG_HDR_SIZE + len);
    len = DAX_MSGMAX;
    _message_recv(MSG_MOD_REG, buff, &len, 1);
    /* Here we check to see if the data that we got in the registration message is in the same
       format as we use here on the client module. This should be offloaded to a separate
       function that can determine what needs to be done to the incoming and outgoing data to
       get it to match with the server */
    if( (*((u_int16_t *)&buff[0]) != REG_TEST_INT) ||     
        (*((u_int32_t *)&buff[2]) != REG_TEST_DINT) || 
        (*((u_int64_t *)&buff[6]) != REG_TEST_LINT)) {   
        /* TODO: right now this is just to show error.  We need to determine if we can
           get the right data from the server by some means. */
        _reformat = REF_INT_SWAP;
    } else {
        _reformat = 0;
    }
    /* There has got to be a better way to compare that we are getting good floating point numbers */
    if( fabs(*((float *)&buff[14]) - REG_TEST_REAL) / REG_TEST_REAL   > 0.0000001 || 
        fabs(*((double *)&buff[18]) - REG_TEST_LREAL) / REG_TEST_REAL > 0.0000001) {   
        _reformat |= REF_FLT_SWAP;
    }
    /* TODO: Need to store my name somewhere ??? */
    return _reformat;
}

/* TODO: Write this function */
int dax_mod_unregister(void) {
    int len;
    _message_send(MSG_MOD_REG, NULL, 0);
    len = 0;
    _message_recv(MSG_MOD_REG, NULL, &len, 1);
    return 0;
}

/* Sends a message to dax to add a tag.  The payload is basically the
   tagname without the handle. */
handle_t dax_tag_add(char *name, unsigned int type, unsigned int count) {
    //dax_tag tag;
    int size;
    int result;
    char buff[DAX_TAGNAME_SIZE + 8 + 1];
    
    if(count == 0) return ERR_ARG;
    if(name) {
        if((size = strlen(name)) > DAX_TAGNAME_SIZE) {
            return ERR_2BIG;
        }
        /* Add the 8 bytes for type and count to one byte for NULL */
        size += 9;
        /* TODO Need to do some more error checking here */
        *((u_int32_t *)&buff[0]) = mtos_udint(type);
        *((u_int32_t *)&buff[4]) = mtos_udint(count);
        
        strcpy(&buff[8], name);
    } else {
        return ERR_TAG_BAD;
    }
    
    result = _message_send( MSG_TAG_ADD, buff, size);
    if(result) { 
        return ERR_MSG_SEND;
    }
    
    result= _message_recv(MSG_TAG_ADD, buff, &size, 1);
    return result;
}


/* Get the tag by name. */
/* TODO: Resolve array and bit references in the tagname */
int dax_get_tag(char *name, dax_tag *tag) {
    int result;
    int size;
    
    /* TODO: Check the tag cache here */
    /* TODO: Some of these may need to be debug messages so they won't print */
    /* It seems like we should bounds check *name but _message-send will clip it! */
    result = _message_send( MSG_TAG_GET, name, DAX_TAGNAME_SIZE);
    if(result) {
        dax_error("Can't send MSG_TAG_GET message");
        return result;
    }
    result = _message_recv(MSG_TAG_GET, (void *)tag, &size, 1);
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
    int size;
    result = _message_send( MSG_TAG_GET, &index, sizeof(int));
    if(result) {
        dax_error("Can't send MSG_TAG_GET message");
        return result;
    }
    result = _message_recv(MSG_TAG_GET, (void *)tag, &size, 1);
    if(result == ERR_ARG) return ERR_ARG;
    return 0;
}

/* The following three functions are the core of the data handling
   system in Dax.  They are the raw reading and writing functions.
   Each takes a handle, a buffer pointer, and a size in bytes.  */

/* TODO: This function should return some kind of error */
int dax_tag_read(handle_t handle, void *data, size_t size) {
    int n, count, m_size, sendsize, tmp;
    int result = 0;
    struct Payload { /* TODO: This is a bad idea - compilers can format these different */
        handle_t handle;
        size_t size;
    } payload;
    /* This calculates the amount of data that we can send with a single message
        It subtracts a handle_t from the data size for use as the tag handle.*/
    m_size = MSG_DATA_SIZE;
    count=((size-1) / m_size) +1;
    for(n=0; n < count; n++) {
        if(n == (count - 1)) { /* Last Packet */
            sendsize = size % m_size; /* What's left over */
        } else {
            sendsize = m_size;
        }
        payload.handle = handle + (m_size * 8 * n);
        payload.size = sendsize;
        result = _message_send(MSG_TAG_READ, (void *)&payload, sizeof(struct Payload));
        result = _message_recv(MSG_TAG_READ, &((u_int8_t *)data)[m_size * n], &tmp, 1);
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
        
        _message_send(MSG_TAG_WRITE,(void *)&msg,sendsize+sizeof(handle_t));
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
        _message_send(MSG_TAG_MWRITE,(void *)&msg,(sendsize * 2)+sizeof(handle_t));
    }
}

/* Adds an event for the given tag.  The count is how many of those
   tags the event should effect. */
/* TODO: The count is not actually implemented yet. */
int dax_event_add(char *tagname, int count) {
    dax_tag tag;
    dax_event_message msg;
    int result, test;
    int size;
    
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
    
    if(_message_send( MSG_EVNT_ADD, &msg, sizeof(dax_event_message))) {
        return ERR_MSG_SEND;
    } else {
        test = _message_recv(MSG_EVNT_ADD, &result, &size, 1);
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
