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
#include <options.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>

#define RESPONSE 1
#define ASYNC 0

#ifndef FD_COPY
# define FD_COPY(x,y) memcpy((y),(x),sizeof(fd_set))
#endif

/* These are the listening sockets for the local UNIX domain
   socket and the remote TCP socekt. */
static int _localfd;
static int _remotefd;
static fd_set _fdset;
static int _maxfd;

/* This array holds the functions for each message command */
#define NUM_COMMANDS 12
int (*cmd_arr[NUM_COMMANDS])(dax_message *) = {NULL};

/* Macro to check whether or not the command 'x' is valid */
#define CHECK_COMMAND(x) (((x) < 0 || (x) > 11) ? 1 : 0)

int msg_mod_register(dax_message *msg);
int msg_tag_add(dax_message *msg);
int msg_tag_del(dax_message *msg);
int msg_tag_get(dax_message *msg);
int msg_tag_list(dax_message *msg);
int msg_tag_read(dax_message *msg);
int msg_tag_write(dax_message *msg);
int msg_tag_mask_write(dax_message *msg);
int msg_mod_get(dax_message *msg);
int msg_evnt_add(dax_message *msg);
int msg_evnt_del(dax_message *msg);
int msg_evnt_get(dax_message *msg);


/* Generic message sending function.  If size is zero then it is assumed that an error
   is being sent to the module.  In that case payload should point to a single int that
   indicates the error */
static int _message_send(int fd, int command, void *payload, size_t size, int response) {
    int result;
    static u_int32_t count = 0;
    size_t newsize;
    char buff[DAX_MSGMAX];
    
    ((u_int32_t *)buff)[0] = htonl(size + MSG_HDR_SIZE);
    if(response) ((u_int32_t *)buff)[1] = htonl(command);
    else         ((u_int32_t *)buff)[1] = htonl(command | MSG_RESPONSE);
    /* TODO: Bounds check this */
    memcpy(&buff[MSG_HDR_SIZE], payload, size);
    
    /* TODO: We need to set some kind of timeout here.  This could block
     forever if something goes wrong.  It may be a signal or something too. */
    result = write(fd, buff, size + MSG_HDR_SIZE);
    if(result < 0) {
        /* TODO: Should we handle the case when this returns due to a signal */
        xerror("_message_send: %s", strerror(errno));
        return ERR_MSG_SEND;
    }
    return 0;    
}

/* Sets up the local UNIX domain socket for listening.
   _localfd is the listening socket. */
int msg_setup_local_socket(void) {
    struct sockaddr_un addr;
    
    _localfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if(_localfd < 0) {
        xfatal("Unable to create local socket - %s", strerror(errno));
    }
    
    bzero(&addr, sizeof(addr));
    strncpy(addr.sun_path, opt_socketname(), sizeof(addr.sun_path));
    addr.sun_family = AF_LOCAL;
    unlink(addr.sun_path); /* Delete the socket if it exists on the filesystem */
    if(bind(_localfd, (const struct sockaddr *)&addr, sizeof(addr))) {
        xfatal("Unable to bind to local socket: %s", addr.sun_path);
    }
    
    if(listen(_localfd, 5) < 0) {
        xfatal("Unable to listen for some reason");
    }
    msg_add_fd(_localfd);
    
    printf("Created local socket - %d\n", _localfd);
    /* TODO: Need to store the name of the sun_path so we can delete the file later */
    return 0;
}

/* Sets up the remote TCP socket for listening.
   _remotefd is the listening socket. */
/* TODO: write this function */
int msg_setup_remote_socket(void) {
    return 0;
}

/* Creates and sets up the main message queue for the program.
   It's a fatal error if we cannot create this queue */
int msg_setup(void) {
    
    /* TODO: See if we are already running, by trying to send ourselves a message */
    _maxfd = 0;
    FD_ZERO(&_fdset);
    
    msg_setup_local_socket();
    msg_setup_remote_socket();
    
    buff_initialize(); /* This initializes the communications buffers */
    
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
    cmd_arr[MSG_EVNT_ADD]   = &msg_evnt_add;
    cmd_arr[MSG_EVNT_DEL]   = &msg_evnt_del;
    cmd_arr[MSG_EVNT_GET]   = &msg_evnt_get;
    
    return 0;
}

/* This destroys the queue if it was created */
void msg_destroy(void) {
    
    unlink(opt_socketname());
    xlog(LOG_MAJOR, "Resources being destroyed");
    
}

/* These two functions are wrappers to deal with adding and deleting
   file descriptors to the global _fdset and dealing with _maxfd */
void msg_add_fd(int fd) {
    printf("Adding fd - %d : ",fd);
    FD_SET(fd, &_fdset);
    if(fd > _maxfd) _maxfd = fd;
    printf("_maxfd = %d\n", _maxfd);
}

void msg_del_fd(int fd) {
    int n;
    
    xlog(LOG_COMM, "Connection Closed for fd %d", fd);
    FD_CLR(fd, &_fdset);
    
    /* If it's the largest one then we need to refigure _maxfd */
    if(fd == _maxfd) {
        for(n = 0; n <= _maxfd; n++) {
            if(FD_ISSET(n, &_fdset)) {
                _maxfd = n;
            }
        }
    }
    close(fd); /* Just to make sure */
    buff_free(fd);
}

/* This function blocks waiting for a message to be received.  Once a message
   is retrieved from the system the proper handling function is called */
int msg_receive(void) {
    fd_set tmpset;
    struct timeval tm;
    struct sockaddr_un addr;
    int result, fd, n;
    socklen_t len;
    
    FD_ZERO(&tmpset);
    FD_COPY(&_fdset, &tmpset);
    tm.tv_sec = 1; /* TODO: this should be configuration */
    tm.tv_usec = 0;
    
    result = select(_maxfd + 1, &tmpset, NULL, NULL, &tm);
    
    if(result < 0) {
        /* TODO: Deal with these errors */
        xerror("msg_receive select error: %s", strerror(errno));
        return ERR_MSG_RECV;
    } else if(result == 0) {
        buff_freeall(); /* this erases all of the _buffer nodes */
        return 0;
    } else {
        for(n = 0; n <= _maxfd; n++) {
            if(FD_ISSET(n, &tmpset)) {
                if(n == _localfd) { /* This is the local listening socket */
                    xlog(LOG_COMM, "Accepted socket on fd %d", n);
                    fd = accept(_localfd, (struct sockaddr *)&addr, &len);
                    if(fd < 0) {
                        /* TODO: Need to handle these errors */
                        xerror("Error Accepting socket: %s", strerror(errno));
                    } else {
                        msg_add_fd(fd);
                    }
                } else {
                    //--printf( "Reading buffer for fd - %d\n", n);
                    result = buff_read(n);
                    if(result == ERR_NO_SOCKET) { /* This is the end of file */
                        /* TODO: Deal with the module if I need too */
                        msg_del_fd(n);
                    } else if(result < 0) {
                        return result; /* Pass the error up */
                    }
                }
            }
        }
    }
    return 0;
}

/* This handles each message.  It extracts out the dax_message structure
   and then calls the proper message handling function.  This message will
   unmarshal the header but it is up to the individual wrapper function to
   unmarshal the data portion of the message if need be. */
int msg_dispatcher(int fd, unsigned char *buff) {
    dax_message message;
    //--dax_module *module;
    
    /* The first four bytes are the size and the size is always
       sent in network order */
    message.size = ntohl(*(u_int32_t *)buff);
    /* The next four bytes are the DAX command also sent in network
       byte order. */
    message.command = ntohl(*(u_int32_t *)&buff[4]);
    
    if(CHECK_COMMAND(message.command)) return ERR_MSG_BAD;
    
    message.fd = fd;    
    memcpy(message.data, &buff[8], message.size - MSG_HDR_SIZE);
    /* Now call the function to deal with it */
    return (*cmd_arr[message.command])(&message);
}


/* Each of these functions are matched to a message command.  These are called
   from and array of function pointers so there should be one for each command
   defined. */
int msg_mod_register(dax_message *msg) {
    pid_t pid; /* Now how do I figure this out??? */
    char buff[30];
    
    if(msg->size > MSG_HDR_SIZE) {
        xlog(4, "Registering Module %s fd = %d", &msg->data[8], msg->fd);
        /*
        printf("First Test Number = 0x%X\n", *((u_int16_t *)&msg->data[0]));
        
        printf("2nd   Test Number = 0x%X\n", *((u_int32_t *)&msg->data[2]));
        for(n=2; n<6; n++) printf("0x%X ", (unsigned char)msg->data[n]);
        printf("\n");
        
        printf("3rd   Test Number = 0x%llX\n", *((u_int64_t *)&msg->data[6]));
        for(n=6; n<14; n++) printf("0x%X ", (unsigned char)msg->data[n]);
        printf("\n");
        
        printf("4th   Test Number = %f\n", *((float *)&msg->data[14]));
        for(n=14; n<18; n++) printf("0x%X ", (unsigned char)msg->data[n]);
        printf("\n");
        
        printf("5th   Test Number = %f == %f\n", *((double *)&msg->data[18]), REG_TEST_LREAL);
        for(n=18; n<26; n++) printf("0x%X ",(unsigned char)msg->data[n]);
        printf("\n");
        */
        pid = ntohl(*((u_int32_t *)&msg->data[0]));
        printf("PID = %d\n", pid);
        printf("Name = %s\n", &msg->data[8]);
        module_register(&msg->data[8], pid, msg->fd);
        
        /* This puts the test data into the buffer for sending. */
        *((u_int16_t *)&buff[0]) = REG_TEST_INT;    /* 16 bit test data */
        *((u_int32_t *)&buff[2]) = REG_TEST_DINT;   /* 32 bit integer test data */
        *((u_int64_t *)&buff[6]) = REG_TEST_LINT;   /* 64 bit integer test data */
        *((float *)&buff[14])    = REG_TEST_REAL;   /* 32 bit float test data */
        *((double *)&buff[18])   = REG_TEST_LREAL;  /* 64 bit float test data */
        _message_send(msg->fd, MSG_MOD_REG, "Registration", 13, 1);
        
    } else {
        xlog(4, "Unregistering Module fd = %d", msg->fd);
        module_unregister(msg->fd);
    }
    return 0;
}

int msg_tag_add(dax_message *msg) {
    dax_tag tag; /* We use a structure within the data[] area of the message */
    handle_t handle;
    
    xlog(10,"Tag Add Message from %d", msg->fd);
    memcpy(tag.name, msg->data, sizeof(dax_tag) - sizeof(handle_t));
    handle = tag_add(tag.name, tag.type, tag.count);
    /* TODO: Gotta handle the error condition here or the module locks up waiting on the message */
    
    if(handle >= 0) {
        _message_send(msg->fd, MSG_TAG_ADD, &handle, sizeof(handle_t), RESPONSE);
    } else {
        _message_send(msg->fd, MSG_TAG_ADD, &handle, 0, RESPONSE);
        return -1; /* do I need this????? */
    }
    return 0;
}

/* TODO: Make this function do something */
int msg_tag_del(dax_message *msg) {
    xlog(10,"Tag Delete Message from %d",msg->fd);
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
    _dax_tag *tag;
    
    if(msg->size == sizeof(int)) { /* Is it a string or index */
        index = *((int *)msg->data); /* cast void * -> int * then indirect */
        tag = tag_get_index(index); /* get the tag */
        if(tag == NULL) result = ERR_ARG;
        xlog(10, "Tag Get Message from %d for index %d",msg->fd,index);
    } else {
        ((char *)msg->data)[DAX_TAGNAME_SIZE] = 0x00; /* Just to avoid trouble */
        tag = tag_get_name((char *)msg->data);
        if(tag == NULL) result = ERR_NOTFOUND;
        xlog(10, "Tag Get Message from %d for name %s", msg->fd, (char *)msg->data);
    }
    if(tag) {
        _message_send(msg->fd, MSG_TAG_GET, tag, sizeof(dax_tag), RESPONSE);
        xlog(10,"Returned tag to calling module %d",msg->fd);
    } else {
        _message_send(msg->fd, MSG_TAG_GET, &result, 0, RESPONSE);
        xlog(10,"Bad tag query for MSG_TAG_GET");
    }
    return 0;
}

int msg_tag_list(dax_message *msg) {
    xlog(10,"Tag List Message from %d", msg->fd);
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
    
	xlog(10,"Tag Read Message from %d, handle 0x%X, size %d",msg->fd, handle, size);
    
    if(tag_read_bytes(handle, data, size) == size) { /* Good read from tagbase */
        _message_send(msg->fd, MSG_TAG_READ, &data, size, RESPONSE);
    } else { /* Send Error */
        _message_send(msg->fd, MSG_TAG_READ, &data, 0, RESPONSE);
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

	xlog(10,"Tag Write Message from module %d, handle 0x%X, size %d", msg->fd, handle, size);

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
	
	xlog(10,"Tag Mask Write Message from module %d, handle 0x%X, size %d", msg->fd, handle, size);
	
    if(tag_mask_write(handle, data, mask, size) != size) {
        xerror("Unable to write tag 0x%X with size %d", handle, size);
    }
    return 0;
}

int msg_mod_get(dax_message *msg) {
    xlog(10,"Get Module Handle Message from %d", msg->fd);
    return 0;
}

int msg_evnt_add(dax_message *msg) {
    dax_event_message event;
    dax_module *module;
    int event_id = -1;
    
    xlog(10, "Add Event Message from %d", msg->fd);
    memcpy(&event, msg->data, sizeof(dax_event_message));
    
    module = module_find_fd(msg->fd);
    if( module != NULL ) {
        event_id = event_add(event.handle, event.size, module);
    }
        
    if(event_id < 0) { /* Send Error */
        _message_send(msg->fd, MSG_EVNT_ADD, &event_id, 0, RESPONSE);    
    } else {
        _message_send(msg->fd, MSG_EVNT_ADD, &event_id, sizeof(int), RESPONSE);
    }
    return 0;
}

int msg_evnt_del(dax_message *msg) {
    return 0;
}

int msg_evnt_get(dax_message *msg) {
    return 0;
}
