/*  OpenDAX - An open source data acquisition and control system
 *  Copyright (c) 2007 Phil Birkelbach
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *

 * This file contains the libdax functions that send messages to the server
 */

#include <libdax.h>
#include <libcommon.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <math.h>


/* These are the generic message functions.  They simply send the message of
 * the type given by command, attach the payload.  The payloads size should be
 * given in bytes */
static int
_message_send(dax_state *ds, int command, void *payload, size_t size)
{
    int result;
    char buff[DAX_MSGMAX];

    /* We always send the size and command in network order */
    ((u_int32_t *)buff)[0] = htonl(size + MSG_HDR_SIZE);
    ((u_int32_t *)buff)[1] = htonl(command);
    memcpy(&buff[MSG_HDR_SIZE], payload, size);

    /* TODO: We need to set some kind of timeout here.  This could block
       forever if something goes wrong.  It may be a signal or something too. */
    result = write(ds->sfd, buff, size + MSG_HDR_SIZE);

    if(result < 0) {
    /* TODO: Should we handle the case when this returns due to a signal */
        dax_error(ds, "_message_send: %s", strerror(errno));
        return ERR_MSG_SEND;
    }
    return 0;
}

/* This function retrieves a single message from the given fd. */
int
message_get(int fd, dax_message *msg) {
    unsigned char buff[DAX_MSGMAX];
    int index, result;
    index = 0;

    while( index < MSG_HDR_SIZE) {
    	/* Start by reading the header */
        result = read(fd, &buff[index], MSG_HDR_SIZE);
        if(result < 0) {
            if(errno == EWOULDBLOCK) {
                return ERR_TIMEOUT;
            } else {
                return ERR_MSG_RECV;
            }
        } else if(result == 0) {
            printf("TODO: I don't know what to do with read returning 0\n");
            return ERR_GENERIC;
        } else {
            index += result;
        }
    }
    /* At this point we should have the size and the message type */
    msg->size = ntohl(*(u_int32_t *)buff);
    msg->msg_type = ntohl(*((u_int32_t *)&buff[4]));
    /* Now we get the rest of the message */
    index = 0;
    while( index < msg->size) {
    	/* Start by reading the header */
        result = read(fd, &buff[index], msg->size-index);
        if(result < 0) {
            if(errno == EWOULDBLOCK) {
                //dax_debug(ds, LOG_COMM, "_message_recv Timed out");
                return ERR_TIMEOUT;
            } else {
                //dax_debug(ds, LOG_COMM, "_message_recv failed: %s", strerror(errno));
                return ERR_MSG_RECV;
            }
        } else if(result == 0) {
            printf("TODO: I don't know what to do with read returning 0\n");
            return ERR_GENERIC;
        } else {
            index += result;
        }
    }
	memcpy(msg->data, buff, msg->size);
    return 0;
}

/* This function waits for a message with the given command to come in. If
   a message of another command comes in it will send that message out to
   an asynchronous command handler.  This is due to a race condition that could
   happen if the server puts a message in the queue after we send a request but
   before we retrieve the result. */
/* TODO: Do a better job of explaining the functionality of this function */
static int
_message_recv(dax_state *ds, int command, void *payload, size_t *size, int response)
{
	dax_message msg;
	int result;
	while(1) {
		result = message_get(ds->sfd, &msg);
		if(result) return result;

        /* Test if the error flag is set and then return the error code */
		if(msg.msg_type == (command | MSG_ERROR)) {
			return stom_dint((*(int32_t *)&msg.data[0]));
		} else if(msg.msg_type == (command | (response ? MSG_RESPONSE : 0))) {
			if(size) {
				memcpy(payload, msg.data, msg.size);
				*size = msg.size;
			}
			return 0;
		/* This is an event message that needs to be stored the queue */
		} else if(msg.msg_type & MSG_EVENT) {
			push_event(ds, &msg);
		} else { /* This is not the command we wanted */
			printf("TODO: Whoa we got the wrong command\n");
		}
    }
    return 0;
}

/* Connect to the server.  If the "server" attribute is local we
 * connect via LOCAL domain socket called out in "socketname" else
 * we connect to the server at IP address "serverip" on port "serverport".
 * returns a positive file descriptor on sucess and a negative error
 * code on failure. */
static int
_get_connection(dax_state *ds)
{
    int fd, len;
    struct sockaddr_un addr_un;
    struct sockaddr_in addr_in;
    struct timeval tv;
    char *server;

    server = dax_get_attr(ds, "server");
    /* TODO: This probably means that the configuration hasn't been set up
     *       So this should have it's own error code. */
    if(server == NULL) return ERR_GENERIC;

    if( ! strcasecmp("local",  server)) {
        //printf("...... Connecting to local socket - %s\n", dax_get_attr("socketname"));
        /* create a UNIX domain stream socket */
        if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
            dax_error(ds, "Unable to create local socket - %s", strerror(errno));
            return ERR_NO_SOCKET;
        }
        /* fill socket address structure with our address */
        memset(&addr_un, 0, sizeof(addr_un));
        addr_un.sun_family = AF_UNIX;
        strncpy(addr_un.sun_path, dax_get_attr(ds, "socketname"), sizeof(addr_un.sun_path));

        len = offsetof(struct sockaddr_un, sun_path) + strlen(addr_un.sun_path);
        if (connect(fd, (struct sockaddr *)&addr_un, len) < 0) {
            dax_error(ds, "Unable to connect to local socket - %s", strerror(errno));
            return ERR_NO_SOCKET;
        } else {
            dax_debug(ds, LOG_COMM, "Connected to Local Server fd = %d", fd);
        }
    } else { /* We are supposed to connect over the network */
        //printf("...... Connecting over the network\n");
        /* create an IPv4 TCP socket */
        if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            dax_error(ds, "Unable to create remote socket - %s", strerror(errno));
            return ERR_NO_SOCKET;
        }
        memset(&addr_in, 0, sizeof(addr_in));
        addr_in.sin_family = AF_INET;
        addr_in.sin_port = htons(strtol(dax_get_attr(ds, "serverport"), NULL, 0));
        inet_pton(AF_INET, dax_get_attr(ds, "serverip"), &addr_in.sin_addr);

        if (connect(fd, (struct sockaddr *)&addr_in, sizeof(addr_in)) < 0) {
            dax_error(ds, "Unable to connect to remote socket - %s", strerror(errno));
            return ERR_NO_SOCKET;
        } else {
            dax_debug(ds, LOG_COMM, "Connected to Network Server fd = %d", fd);
        }
    }
    tv.tv_sec = opt_get_msgtimeout(ds) / 1000;
    tv.tv_usec = (opt_get_msgtimeout(ds) % 1000) * 1000;

    len = setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    if(len) {
        dax_error(ds, "Problem setting socket timeout - s", strerror(errno));
    }
    return fd;

}

#define CON_HDR_SIZE 8

static int
_mod_connect(dax_state *ds, char *name)
{
    int result;
    size_t len;
    char buff[DAX_MSGMAX];

/* TODO: Boundary check that a name that is longer than data size will
   be handled correctly. */
    len = strlen(name) + 1;
    if(len > (MSG_DATA_SIZE - CON_HDR_SIZE)) {
        len = MSG_DATA_SIZE - CON_HDR_SIZE;
        name[len] = '\0';
    }

    /* For registration we send the data in network order no matter what */
    /* TODO: The timeout is not actually implemented */
    *((u_int32_t *)&buff[0]) = htonl(1000);       /* Timeout  */
    *((u_int32_t *)&buff[4]) = htonl(CONNECT_SYNC);  /* registration flags */
    strcpy(&buff[CON_HDR_SIZE], name);                /* The rest is the name */

    if((result = _message_send(ds, MSG_MOD_REG, buff, CON_HDR_SIZE + len)))
        return result;
    len = DAX_MSGMAX;
    if((result = _message_recv(ds, MSG_MOD_REG, buff, &len, 1)))
        return result;

    /* Store the unique ID that the server has sent us. */
    ds->id =  *((u_int32_t *)&buff[0]);
    /* Here we check to see if the data that we got in the registration message is in the same
       format as we use here on the client module. This should be offloaded to a separate
       function that can determine what needs to be done to the incoming and outgoing data to
       get it to match with the server */
    if( (*((u_int16_t *)&buff[4]) != REG_TEST_INT) ||
        (*((u_int32_t *)&buff[6]) != REG_TEST_DINT) ||
        (*((u_int64_t *)&buff[10]) != REG_TEST_LINT)) {
        /* TODO: right now this is just to show error.  We need to determine if we can
           get the right data from the server by some means. */
        ds->reformat = REF_INT_SWAP;
    } else {
        ds->reformat = 0; /* this is redundant, already done in dax_init() */
    }
    /* There has got to be a better way to compare that we are getting good floating point numbers */
    if( fabs(*((float *)&buff[18]) - REG_TEST_REAL) / REG_TEST_REAL   > 0.0000001 ||
        fabs(*((double *)&buff[22]) - REG_TEST_LREAL) / REG_TEST_REAL > 0.0000001) {
        ds->reformat |= REF_FLT_SWAP;
    }
    /* TODO: returning _reformat is only good until we figure out how to reformat the
     * messages. Then we should return 0.  Right now since there isn't any reformating
     * of messages being done we consider it an error and return that so that the module
     * won't try to communicate */
    return ds->reformat;
}


/*!
 * Setup the module data structures and send registration message
 * to the server.
 * 
 * @param ds Pointer to the dax state object.
 *
 * @returns Zero on success or an error code otherwise
 */
int
dax_connect(dax_state *ds)
{
    int fd, result;

    libdax_lock(ds->lock);
    dax_debug(ds, LOG_COMM, "Sending registration for name - %s", ds->modulename);

    /* This is the connection that we used for all the functional
     * request / response messages. */
    fd = _get_connection(ds);
    if(fd > 0) {
        ds->sfd = fd;
    } else {
        libdax_unlock(ds->lock);
        return fd;
    }

    result = _mod_connect(ds, ds->modulename);
    if(result) {
        libdax_unlock(ds->lock);
        return result;
    }

    init_tag_cache(ds);

    libdax_unlock(ds->lock);
    return 0;
}

/*!
 * Unregister our client module with the server and dax_disconnect
 * the sockets of the connection
 * 
 * @param ds The pointer to the dax state object
 * 
 * @returns Zero on success or an error code otherwise
 */
int
dax_disconnect(dax_state *ds)
{
    int result = -1;
    size_t len;
    libdax_lock(ds->lock);

    if(ds->sfd) {
        result = _message_send(ds, MSG_MOD_REG, NULL, 0);
        if(! result ) {
            len = 0;
            result = _message_recv(ds, MSG_MOD_REG, NULL, &len, 1);
        }
        close(ds->sfd);
        ds->sfd = 0;
    }
    libdax_unlock(ds->lock);
    return result;
}

/* Not yet implemented */
int
dax_mod_get(dax_state *ds, char *modname)
{
    return 0;
}

/*!
 * This is a generic function for setting module parameters.
 */
int
dax_mod_set(dax_state *ds, u_int8_t cmd, void *param)
{
    int result;
    size_t size;
    char buff[1];  /* So far this is as big as we need */

    libdax_lock(ds->lock);
    buff[0] = cmd;
    if(cmd == MOD_CMD_RUNNING) {
        size = 1;
    } else {
        return ERR_ARG;
    }
        /* Send the message to the server.  Add 2 to the size for the subcommand and the NULL */
    result = _message_send(ds, MSG_MOD_SET, buff, size);
    if(result) {
        dax_error(ds, "Can't send MSG_MOD_SET message");
        libdax_unlock(ds->lock);
        return result;
    }
    size = 0;
    result = _message_recv(ds, MSG_MOD_SET, buff, &size, 1);
    if(result) {
        dax_error(ds, "Problem receiving message MSG_MOD_SET : result = %d", result);
        libdax_unlock(ds->lock);
        return result;
    }
    libdax_unlock(ds->lock);
    return 0;
}

/*! 
 * Adds a tag to the tag server.
 * @param ds The pointer to the dax state object
 * @param h  Pointer to a tag handle structure.  This structure will be filled
 *           in by this function if it is successful.  NULL may be passed if
 *           the caller is not interested in the tag handle
 * @param name Pointer to the name of the new name
 * @param type Data type.
 * @param count If count is greater than 1 then an array is created.
 * 
 * @returns Zero on success and an error code otherwise
 */
int
dax_tag_add(dax_state *ds, tag_handle *h, char *name, tag_type type, int count)
{
    int result;
    size_t size;
    dax_tag tag;
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
    libdax_lock(ds->lock);

    result = _message_send(ds, MSG_TAG_ADD, buff, size);
    if(result) {
        libdax_unlock(ds->lock);
        return ERR_MSG_SEND;
    }

    size = 4; /* we just need the handle */
    result = _message_recv(ds, MSG_TAG_ADD, buff, &size, 1);

    if(result == 0) {
        if(h != NULL) {
            h->index = *(tag_index *)buff;
            h->byte = 0;
            h->bit = 0;
            h->type = type;
            h->count = count;
            if(type == DAX_BOOL) {
                h->size = (count - 1)/8 +1;
            } else {
                h->size = count * dax_get_typesize(ds, type);
            }
        }
        strcpy(tag.name, name);
        tag.idx = *(tag_index *)buff;
        tag.type = type;
        tag.count = count;
        /* Just in case this call modifies the tag */
        cache_tag_del(ds, tag.idx);
        cache_tag_add(ds, &tag);
    }
    libdax_unlock(ds->lock);
    return result;
}

/*!
 * Delete a tag from the tagserver.
 * 
 * @param ds Pointer to the dax state object
 * @param index Database index of the tag to be deleted
 * 
 * @returns Zero on success or an error code otherwise
 */
int
dax_tag_del(dax_state* ds, tag_index index)
{
    int result;
    size_t size;
    char buff[sizeof(tag_index)];
    *((u_int32_t *)&buff[0]) = mtos_udint(index);
    
    libdax_lock(ds->lock);
    result = _message_send(ds, MSG_TAG_DEL, buff, sizeof(buff));
    if(result) {
        libdax_unlock(ds->lock);
        return ERR_MSG_SEND;
    }

    size = 4; /* we just need the handle */
    result = _message_recv(ds, MSG_TAG_DEL, buff, &size, 1);
    libdax_unlock(ds->lock);
    return result;        
}

/*!
 * Retrive a tag definition based on the tags name.
 * 
 * @param ds Pointer to the dax state object
 * @param tag Pointer to the structure that this function will
 *            fill with the tags information
 * @param name The name of the tag that we are requesting
 * 
 * @returns Zero on success or an error code otherwise
 */
int
dax_tag_byname(dax_state *ds, dax_tag *tag, char *name)
{
    int result;
    size_t size;
    char *buff;

    if(name == NULL) return ERR_ARG;

    if((size = strlen(name)) > DAX_TAGNAME_SIZE) return ERR_2BIG;

    libdax_lock(ds->lock);
    if(check_cache_name(ds, name, tag)) {
        /* We make buff big enough for the outgoing message and the incoming
           response message which would have 3 additional int32s */
        buff = malloc(size + 14);
        if(buff == NULL) return ERR_ALLOC;
        buff[0] = TAG_GET_NAME;
        strcpy(&buff[1], name);
        /* Send the message to the server.  Add 2 to the size for the subcommand and the NULL */
        result = _message_send(ds, MSG_TAG_GET, buff, size + 2);
        if(result) {
            dax_error(ds, "Can't send MSG_TAG_GET message");
            free(buff);
            libdax_unlock(ds->lock);
            return result;
        }
        size += 14; /* This makes room for the type, count and handle */

        result = _message_recv(ds, MSG_TAG_GET, buff, &size, 1);
        if(result) {
            free(buff);
            libdax_unlock(ds->lock);
            return result;
        }
        tag->idx = stom_dint( *((int *)&buff[0]) );
        tag->type = stom_udint(*((u_int32_t *)&buff[4]));
        tag->count = stom_udint(*((u_int32_t *)&buff[8]));
        buff[size - 1] = '\0'; /* Just to make sure */
        strcpy(tag->name, &buff[12]);
        cache_tag_add(ds, tag);
        free(buff);
    }
    libdax_unlock(ds->lock);
    return 0;
}

/*!
 * Retrieve the tag by it's index.
 * 
 * @param ds Pointer to the dax state object
 * @param tag Pointer to the structure that this function will
 *            fill with the tags information
 * @param idx The index of the tag that we are requesting
 * 
 * @returns Zero on success or an error code otherwise
 */
 int
dax_tag_byindex(dax_state *ds, dax_tag *tag, tag_index idx)
{
    int result;
    size_t size;
    char buff[DAX_TAGNAME_SIZE + 13];

    libdax_lock(ds->lock);
    if(check_cache_index(ds, idx, tag)) {
        buff[0] = TAG_GET_INDEX;
        *((tag_index *)&buff[1]) = idx;
        result = _message_send(ds, MSG_TAG_GET, buff, sizeof(tag_index) + 1);
        if(result) {
            dax_error(ds, "Can't send MSG_TAG_GET message");
            libdax_unlock(ds->lock);
            return result;
        }
        /* Maximum size of buffer, the 13 is the NULL plus three integers */
        size = DAX_TAGNAME_SIZE + 13;
        result = _message_recv(ds, MSG_TAG_GET, buff, &size, 1);
        if(result) {
            //dax_error("Unable to retrieve tag for index %d", idx);
            libdax_unlock(ds->lock);
            return result;
        }
        tag->idx = stom_dint(*((int32_t *)&buff[0]));
        tag->type = stom_dint(*((int32_t *)&buff[4]));
        tag->count = stom_dint(*((int32_t *)&buff[8]));
        buff[DAX_TAGNAME_SIZE + 12] = '\0'; /* Just to be safe */
        strcpy(tag->name, &buff[12]);
        /* Add the tag to the tag cache */
        cache_tag_add(ds, tag);
    }
    libdax_unlock(ds->lock);
    return 0;
}

/*!
 * Raw low level database read.  The data will be retrieved exactly
 * like it appears in the server.  It is up to the module to convert
 * the data to the servers's number format.  There are other functions
 * in this library to help with the conversion.
 * 
 * @param ds The pointer to the dax state object
 * @param idx Tag index found in the tag_handle of the tag as 
 *            returned by the dax_tag_add() function.  
 * @param offset The byte offset within the data area of the tag
 * @param data Pointer to a data area where the data will be written
 * @size size The number of bytes to read.
 * 
 * @returns Zero upon success or an error code otherwise
 */
int
dax_read(dax_state *ds, tag_index idx, u_int32_t offset, void *data, size_t size)
{
    int result = 0;
    u_int8_t buff[14];

    
    /* If we try to read more data that can be held in a single message we return an error */
    if(size > MSG_DATA_SIZE) {
        return ERR_2BIG;
    }

    *((tag_index *)&buff[0]) = mtos_dint(idx);
    *((u_int32_t *)&buff[4]) = mtos_dint(offset);
    *((u_int32_t *)&buff[8]) = mtos_dint(size);
    
    libdax_lock(ds->lock);
    result = _message_send(ds, MSG_TAG_READ, (void *)buff, sizeof(buff));
    if(result) {
        libdax_unlock(ds->lock);
        return result;
    }
    result = _message_recv(ds, MSG_TAG_READ, (char *)data, &size, 1);

    if(result) {
        libdax_unlock(ds->lock);
        if(result == ERR_DELETED) {
            cache_tag_del(ds, idx);
        }
        return result;
    }
    libdax_unlock(ds->lock);
    return 0;
}

/*!
 * Raw low level database write.  Used to write raw data to the database.
 * This function makes no assumptions about the type of data and simply
 * writes the data to the database. It is assumed that the data is 
 * already in the servers number format.
 * @param ds Pointer to the dax state object.
 * @param idx The index of the tag that we are writing to
 * @param offset Byte offset within the tags data area where we are
 *               writing the data
 * @param data Pointer to the data that we wish to write
 * @param size Size of the data that we wish to write in bytes
 * 
 * @returns Zero upon success or an error code otherwise
 */
int
dax_write(dax_state *ds, tag_index idx, u_int32_t offset, void *data, size_t size)
{
    size_t sendsize;
    int result;
    char buff[MSG_DATA_SIZE];

    /* This calculates the amount of data that we can send with a single message
       It subtracts a handle_t from the data size for use as the tag handle and
       an int, because we'll send the handle and the offset.*/
    sendsize = size + sizeof(tag_index) + sizeof(u_int32_t);
    /* It is assumed that the flags that we want to set are the first 4 bytes are in *data */
    /* If we try to read more data that can be held in a single message we return an error */
    if(sendsize > MSG_DATA_SIZE) {
        return ERR_2BIG;
    }

    /* Write the data to the message buffer */
    *((tag_index *)&buff[0]) = mtos_dint(idx);
    *((u_int32_t *)&buff[4]) = mtos_dint(offset); 
    
    memcpy(&buff[8], data, size);

    libdax_lock(ds->lock);
    result = _message_send(ds, MSG_TAG_WRITE, buff, sendsize);
    if(result) {
        libdax_unlock(ds->lock);
        return result;
    }
    result = _message_recv(ds, MSG_TAG_WRITE, buff, 0, 1);
    if(result) {
        libdax_unlock(ds->lock);
        if(result == ERR_DELETED) {
            cache_tag_del(ds, idx);
        }
        return result;
    }
    libdax_unlock(ds->lock);
    return 0;
}

/*!
 * Same as the dax_write() function except that only bits that are in *mask
 * will be changed.
 * @param ds Pointer to the dax state object.
 * @param idx The index of the tag that we are writing to
 * @param offset Byte offset within the tags data area where we are
 *               writing the data
 * @param data Pointer to the data that we wish to write
 * @param mask Pointer to the buffer that we wish to use as the mask.
 *             This should be the same size as data and is used to mark
 *             the portion of the data buffer that will be written.  If
 *             any bit is clear (=0) in this mask the data in the tag server
 *             will remain unchanged.
 * @param size Size of the data that we wish to write in bytes
 * 
 * @returns Zero upon success or an error code otherwise
*/
int
dax_mask(dax_state *ds, tag_index idx, u_int32_t offset, void *data, void *mask, size_t size)
{
    size_t sendsize;
    u_int8_t buff[MSG_DATA_SIZE];
    int result;

    /* This calculates the amount of data that we can send with a single message
       It subtracts a handle_t from the data size for use as the tag handle.*/
    sendsize = size*2 + sizeof(tag_index) + sizeof(u_int32_t);
    /* It is assumed that the flags that we want to set are the first 4 bytes are in *data */
    /* If we try to read more data that can be held in a single message we return an error */
    if(sendsize > MSG_DATA_SIZE) {
        return ERR_2BIG;
    }
    /* Write the data to the message buffer */
    *((tag_index *)&buff[0]) = mtos_dint(idx);
    *((u_int32_t *)&buff[4]) = mtos_dint(offset);
    memcpy(&buff[8], data, size);
    memcpy(&buff[8 + size], mask, size);

    libdax_lock(ds->lock);
    result = _message_send(ds, MSG_TAG_MWRITE, buff, sendsize);
    if(result) {
        libdax_unlock(ds->lock);
        return result;
    }
    result = _message_recv(ds, MSG_TAG_MWRITE, buff, 0, 1);
    if(result) {
        libdax_unlock(ds->lock);
        if(result == ERR_DELETED) {
            cache_tag_del(ds, idx);
        }
        return result;
    }
    libdax_unlock(ds->lock);
    return 0;
}


/*!
 * Add an event to the tag server.  An event is triggered when the conditions
 * given become true.
 * 
 * @param ds Pointer to the dax state ojbect
 * @param h  Pointer to a handle that repreents the tag or the part of the 
 *           tag that we wish to apply the event to.
 * @param event_type The type of event
 * @param data Any data that may be required by this event.  For example if
 *             the event is a Greater Than event then this data would represent
 *             the number that the tag given by the handle would be compared against.
 *             If not needed for the particular event type being created it should
 *             be set to NULL.
 * @param id Pointer that will be filled in with an identifier for this event.  This
 *           identifier will be needed to delete or modify this event.
 * @param callback This is the function that will be called by the system from either
 *                 the event_wait() or event_poll() functions.
 * @param udata This a pointer to any arbitrary data that the user needs to be sent
 *              when the event happens.  This pointer will be passed to the callback
 *              function.  This allows a single callback function to deal with
 *              different events.  It can store any data that the user wishes.
 *              OpenDAX makes no assumptions about the type of this data.
 * @param free_callback If given, this function will be called when the event
 *                      is deleted.  This gives the module a method to free
 *                      the udata if necessary.  Can be set to NULL if not needed.
 * 
 * @return Zero on success or an error code otherwise.
 */
int
dax_event_add(dax_state *ds, tag_handle *h, int event_type, void *data,
              dax_id *id, void (*callback)(dax_state *ds, void *udata),
              void *udata, void (*free_callback)(void *udata))
{
    int test;
    dax_dint result;
    dax_dint temp;
    dax_udint u_temp;
    size_t size;
    dax_id eid;
    char buff[MSG_DATA_SIZE];

    temp = mtos_dint(h->index);      /* Index */
    memcpy(buff, &temp, 4);
    temp = mtos_dint(h->byte);       /* Byte offset */
    memcpy(&buff[4], &temp, 4);
    temp = mtos_dint(h->count);      /* Tag Count */
    memcpy(&buff[8], &temp, 4);
    temp = mtos_dint(h->type);       /* Data type */
    memcpy(&buff[12], &temp, 4);
    temp = mtos_dint(event_type);    /* Event Type */
    memcpy(&buff[16], &temp, 4);
    u_temp = mtos_udint(h->size);    /* Size in Bytes */
    memcpy(&buff[20], &u_temp, 4);
    buff[24]=h->bit;                 /* Bit offset */

    if(data != NULL) {
        mtos_generic(h->type, &buff[25], data);
        size = 25 + TYPESIZE(h->type) / 8;
    } else {
        size = 25;
    }

    libdax_lock(ds->lock);
    if(_message_send(ds, MSG_EVNT_ADD, buff, size)) {
        libdax_unlock(ds->lock);
        return ERR_MSG_SEND;
    } else {
        test = _message_recv(ds, MSG_EVNT_ADD, &result, &size, 1);
        if(test) {
            libdax_unlock(ds->lock);
            return test;
        } else {
            if(id != NULL) {
                id->id = result;
                id->index = h->index;
            }
            eid.id = result;
            eid.index = h->index;
            result = add_event(ds, eid, udata, callback, free_callback);
            if(result) {
                libdax_unlock(ds->lock);
                return result;
            }
        }
    }
    libdax_unlock(ds->lock);
    return 0;
}

/*!
 * Delete the given event from the tag server
 * 
 * @param ds Pointer to the dax state object.
 * @param id The identifier of the event that we wish to remove.
 * 
 * @returns Zero on success or an error code otherwise
 */

int
dax_event_del(dax_state *ds, dax_id id)
{
    int test;
    size_t size;
    dax_dint result;
    dax_dint temp;
    char buff[MSG_DATA_SIZE];

    temp = mtos_dint(id.index);      /* Tag Index */
    memcpy(buff, &temp, 4);
    temp = mtos_dint(id.id);       /* Event ID */
    memcpy(&buff[4], &temp, 4);
    size = 8;

    libdax_lock(ds->lock);
    if(_message_send(ds, MSG_EVNT_DEL, buff, size)) {
        libdax_unlock(ds->lock);
        return ERR_MSG_SEND;
    } else {
        test = _message_recv(ds, MSG_EVNT_DEL, &result, &size, 1);
        if(test) {
            libdax_unlock(ds->lock);
            return test;
        } else {
            del_event(ds, id);
        }
    }
    libdax_unlock(ds->lock);
    return result;
}

/*!
 * Retrieve the details of the given event.
 * This function is not yet implemented
 */
int
dax_event_get(dax_state *ds, dax_id id)
{

    return 0;
}

/*!
 * Set options flags on the given event.
 *
 * @param ds Pointer to the dax state object.
 * @param id The identifier of the event that we wish to remove.
 * @param options Options bits
 *
 * @returns Zero on success or an error code otherwise
 */
int
dax_event_options(dax_state *ds, dax_id id, u_int32_t options)
{
	int test;
	size_t size;
	dax_dint result;
	dax_dint temp;
	char buff[MSG_DATA_SIZE];

	temp = mtos_dint(id.index);      /* Tag Index */
	memcpy(buff, &temp, 4);
	temp = mtos_dint(id.id);         /* Event ID */
	memcpy(&buff[4], &temp, 4);
	temp = mtos_dint(options);       /* Options */
	memcpy(&buff[8], &temp, 4);
	size = 12;

	libdax_lock(ds->lock);
	if(_message_send(ds, MSG_EVNT_OPT, buff, size)) {
		libdax_unlock(ds->lock);
		return ERR_MSG_SEND;
	} else {
		test = _message_recv(ds, MSG_EVNT_OPT, &result, &size, 1);
		libdax_unlock(ds->lock);
		return test;
	}
	libdax_unlock(ds->lock);
    return 0;
}

/*!
 * Write the compound datatype to the server and free the memory
 * associated with it.  This is the last function to be called during
 * the create of a compound data type.
 *
 * @param ds Pointer to the dax state object
 * @param cdt Pointer to the compound data type object that was created
 *            with dax_cdt_new().
 * @param type Pointer to a type.  This value will be filled in by this
 *             function with the data type identifier that can then be
 *             used to create tags.
 * @returns Zero on success or an error code otherwise
 */
/* TODO: There is an arbitrary limit to the size that a compound
 * datatype can be. If the desription string is larger than can
 * be sent in one message this will fail. */
int
dax_cdt_create(dax_state *ds, dax_cdt *cdt, tag_type *type)
{
    int result;
    size_t size = 0;
    cdt_member *this;
    char test[DAX_TAGNAME_SIZE + 1];
    char buff[MSG_DATA_SIZE], rbuff[10];

    if(cdt->name == NULL || cdt->members == NULL) return ERR_EMPTY;

    /* The first thing we do is figure out how big it
     * will all be. */
    size = strlen(cdt->name);
    this = cdt->members;

    while(this != NULL) {
        size += strlen(this->name);
        size += strlen(dax_type_to_string(ds, this->type));
        snprintf(test, DAX_TAGNAME_SIZE + 1, "%d", this->count);
        size += strlen(test);
        size += 3; /* This is for the ':' and the two commas */
        this = this->next;
    }
    size += 1; /* For Trailing NULL */

    if(size > MSG_DATA_SIZE) return ERR_2BIG;

    /* Now build the string */
    strncpy(buff, cdt->name, size - 1);
    this = cdt->members;

    while(this != NULL) {
        strncat(buff, ":", size - 1);
        strncat(buff, this->name, size - 1);
        strncat(buff, ",", size);
        strncat(buff, dax_type_to_string(ds, this->type), size - 1);
        strncat(buff, ",", size - 1);
        snprintf(test, DAX_TAGNAME_SIZE + 1, "%d", this->count);
        strncat(buff, test, size - 1);

        this = this->next;
    }

    libdax_lock(ds->lock);
    result = _message_send(ds, MSG_CDT_CREATE, buff, size);

    if(result) {
        libdax_unlock(ds->lock);
        return result;
    }

    size = 10;
    result = _message_recv(ds, MSG_CDT_CREATE, rbuff, &size, 1);

    if(result == 0) {
        if(type != NULL) {
            *type = stom_udint(*((tag_type *)rbuff));
        }
        result = add_cdt_to_cache(ds, stom_udint(*((tag_type *)rbuff)), buff);
        dax_cdt_free(cdt);
    }
    libdax_unlock(ds->lock);
    return result;
}

/*!
 * This function retrieves the serialized string definition
 * from the server and puts the definition into list of
 * datatypes so that the rest of the library can access them.
 * Either type or name should be passed.  If name is passed
 * we will retrieve the cdt from the server by name and by
 * the type if NULL is passed for name.
 *
 * If this function returns success then it can be assumed that
 * the type is in the cache and can then be retrieved there.
 * 
 * @param ds Pointer to the dax state object
 * @param cdt_type Identifier to the type that we are requesting
 *                 This can be set to zero of name is being used
 *                 to retrive this type
 * @param name Name of the type that we wish to retrieve.  If type
 *             is being used to retrieve the type then NULL can be
 *             passed here.
 * @returns Zero on success or an error code otherwise
 */
int
dax_cdt_get(dax_state *ds, tag_type cdt_type, char *name)
{
    int result;
    size_t size;
    char buff[MSG_DATA_SIZE];
    tag_type type;

    size = sizeof(tag_type);
    if(name != NULL) {
        buff[0] = CDT_GET_NAME;
        size = strlen(name) + 1;
        strncpy(&(buff[1]), name, size); /* Pass the cdt name in this subcommand */
        size++; /* Add one for the sub command */
    } else {
        buff[0] = CDT_GET_TYPE;  /* Put the subcommand in the first byte */
        *((u_int32_t *)&buff[1]) = mtos_udint(cdt_type); /* type in the next four */
        size = 5;
    }

    libdax_lock(ds->lock);
    result = _message_send(ds, MSG_CDT_GET, buff, size);

    if(result) {
        libdax_unlock(ds->lock);
        return ERR_MSG_SEND;
    }

    size = MSG_DATA_SIZE;
    result = _message_recv(ds, MSG_CDT_GET, buff, &size, 1);
    if(result == 0) {
        type = stom_udint(*((tag_type *)buff));
        result = add_cdt_to_cache(ds, type, &(buff[4]));
    }
    libdax_unlock(ds->lock);
    return result;
}

/*!
 * This function sends a message to add a data mapping to the server.
 * It takes two Handles as arguments.  The first is for the source data
 * point and the second is for the destination.  The id pointer will have
 * the unique id of the mapping if the function returns successfully.
 */
int
dax_map_add(dax_state *ds, tag_handle *src, tag_handle *dest, dax_id *id)
{
    int result;
    size_t size;
    dax_dint temp;
    dax_udint u_temp;

    char buff[MSG_DATA_SIZE];

    temp = mtos_dint(src->index);      /* Index */
    memcpy(buff, &temp, 4);
    temp = mtos_dint(src->byte);       /* Byte offset */
    memcpy(&buff[4], &temp, 4);
    buff[8] = src->bit;                /* Bit offset */
    temp = mtos_dint(src->count);      /* Tag Count */
    memcpy(&buff[9], &temp, 4);
    u_temp = mtos_udint(src->size);    /* Size in Bytes */
    memcpy(&buff[13], &u_temp, 4);
    temp = mtos_dint(src->type);       /* Data type */
    memcpy(&buff[17], &temp, 4);

    temp = mtos_dint(dest->index);      /* Index */
    memcpy(&buff[21], &temp, 4);
    temp = mtos_dint(dest->byte);       /* Byte offset */
    memcpy(&buff[25], &temp, 4);
    buff[29] = dest->bit;               /* Bit offset */
    temp = mtos_dint(dest->count);      /* Tag Count */
    memcpy(&buff[30], &temp, 4);
    u_temp = mtos_udint(dest->size);    /* Size in Bytes */
    memcpy(&buff[34], &u_temp, 4);
    temp = mtos_dint(dest->type);       /* Data type */
    memcpy(&buff[38], &temp, 4);

    size = sizeof(tag_handle) * 2;

    libdax_lock(ds->lock);
    result = _message_send(ds, MSG_MAP_ADD, buff, size);

    if(result) {
        libdax_unlock(ds->lock);
        return ERR_MSG_SEND;
    }

    size = MSG_DATA_SIZE;
    result = _message_recv(ds, MSG_MAP_ADD, buff, &size, 1);
    if(result == 0) {
        if(id != NULL) {
            id->id = *(tag_index *)buff;
            id->index = src->index;
        }
    }
    libdax_unlock(ds->lock);
    return result;
}
