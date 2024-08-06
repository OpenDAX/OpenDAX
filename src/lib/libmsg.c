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

#define _XOPEN_SOURCE 600
#include <libdax.h>
#include <libcommon.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stddef.h>
#include <pthread.h>
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

    if(ds->sfd < 0) {
    	return ERR_DISCONNECTED;
    }
    /* We always send the size and command in network order */
    ((uint32_t *)buff)[0] = htonl(size + MSG_HDR_SIZE);
    ((uint32_t *)buff)[1] = htonl(command);
    memcpy(&buff[MSG_HDR_SIZE], payload, size);

    /* TODO: We need to set some kind of timeout here.  This could block
       forever if something goes wrong.  It may be a signal or something too. */
    result = write(ds->sfd, buff, size + MSG_HDR_SIZE);
    if(result < 0) {
    /* TODO: Should we handle the case when this returns due to a signal */
        dax_log(DAX_LOG_ERROR, "_message_send: %s", strerror(errno));
        return ERR_MSG_SEND;
    }
    return 0;
}

/* This function retrieves a single message from the given fd. */
static int
_message_get(int fd, dax_message *msg) {
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
            return ERR_DISCONNECTED;
        } else {
            index += result;
        }
    }
    /* At this point we should have the size and the message type */
    msg->size = ntohl(*(uint32_t *)buff);
    msg->msg_type = ntohl(*((uint32_t *)&buff[4]));
    /* Now we get the rest of the message */
    index = 0;
    while( index < msg->size) {
    	/* Read the rest of the message */
        result = read(fd, &buff[index], msg->size-index);
        if(result < 0) {
            if(errno == EWOULDBLOCK) {
                return ERR_TIMEOUT;
            } else {
                return ERR_MSG_RECV;
            }
        } else if(result == 0) {
            return ERR_DISCONNECTED;
        } else {
            index += result;
        }
    }
    /* TODO: Use msg->data in the read functions instead of this copy */
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
	int result;
    struct timespec timeout;

    pthread_mutex_lock(&ds->msg_lock);
    while(ds->last_msg == NULL) {
        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_sec += ds->msgtimeout/1000;
        timeout.tv_nsec += ds->msgtimeout%1000 *1e6;
        if(timeout.tv_nsec>1e9) {
            timeout.tv_sec++;
            timeout.tv_nsec-=1e9;
        }
        result = pthread_cond_timedwait(&ds->msg_cond, &ds->msg_lock, &timeout);
        if(result == ETIMEDOUT) {
            DF("message_recv() Timeout");
            pthread_mutex_unlock(&ds->msg_lock);
            return ERR_TIMEOUT;
        }
        assert(result == 0);
    }
    assert(ds->last_msg != NULL);
    if(ds->last_msg->msg_type == (command | MSG_ERROR)) {
        result = stom_dint((*(int32_t *)&ds->last_msg->data[0]));
        free(ds->last_msg);
        ds->last_msg = NULL;
        pthread_mutex_unlock(&ds->msg_lock);
        return result;
    }  else if(ds->last_msg->msg_type == (command | (response ? MSG_RESPONSE : 0))) {
        if(size) {
            memcpy(payload, ds->last_msg->data, ds->last_msg->size);
            *size = ds->last_msg->size;
        }
        free(ds->last_msg);
        ds->last_msg = NULL;
        pthread_mutex_unlock(&ds->msg_lock);
        return 0;
    } else {
        free(ds->last_msg);
        ds->last_msg = NULL;
        pthread_mutex_unlock(&ds->msg_lock);
        dax_log(DAX_LOG_ERROR, "Received a response of a different type than expected\n");
        return ERR_GENERIC;
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
        /* create a UNIX domain stream socket */
        if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
            dax_log(DAX_LOG_ERROR, "Unable to create local socket - %s", strerror(errno));
            return ERR_NO_SOCKET;
        }
        /* fill socket address structure with our address */
        memset(&addr_un, 0, sizeof(addr_un));
        addr_un.sun_family = AF_UNIX;
        strncpy(addr_un.sun_path, dax_get_attr(ds, "socketname"), sizeof(addr_un.sun_path));

        len = offsetof(struct sockaddr_un, sun_path) + strlen(addr_un.sun_path);
        if (connect(fd, (struct sockaddr *)&addr_un, len) < 0) {
            dax_log(DAX_LOG_ERROR, "Unable to connect to local socket - %s", strerror(errno));
            return ERR_NO_SOCKET;
        } else {
            dax_log(DAX_LOG_COMM, "Connected to Local Server fd = %d", fd);
        }
    } else { /* We are supposed to connect over the network */
        /* create an IPv4 TCP socket */
        if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            dax_log(DAX_LOG_ERROR, "Unable to create remote socket - %s", strerror(errno));
            return ERR_NO_SOCKET;
        }
        memset(&addr_in, 0, sizeof(addr_in));
        addr_in.sin_family = AF_INET;
        addr_in.sin_port = htons(strtol(dax_get_attr(ds, "serverport"), NULL, 0));
        inet_pton(AF_INET, dax_get_attr(ds, "serverip"), &addr_in.sin_addr);

        if (connect(fd, (struct sockaddr *)&addr_in, sizeof(addr_in)) < 0) {
            dax_log(DAX_LOG_ERROR, "Unable to connect to remote socket - %s", strerror(errno));
            return ERR_NO_SOCKET;
        } else {
            dax_log(DAX_LOG_COMM, "Connected to Network Server fd = %d", fd);
        }
    }
    tv.tv_sec = opt_get_msgtimeout(ds) / 1000;
    tv.tv_usec = (opt_get_msgtimeout(ds) % 1000) * 1000;

    len = setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    if(len) {
        dax_log(DAX_LOG_ERROR, "Problem setting socket timeout - s", strerror(errno));
    }
    return fd;

}

#define CON_HDR_SIZE 8

static int
_mod_register(dax_state *ds, char *name)
{
    int result;
    size_t len;
    char buff[DAX_MSGMAX];
    dax_message msg;

/* TODO: Boundary check that a name that is longer than data size will
   be handled correctly. */
    len = strlen(name) + 1;
    if(len > (MSG_DATA_SIZE - CON_HDR_SIZE)) {
        len = MSG_DATA_SIZE - CON_HDR_SIZE;
        name[len] = '\0';
    }

    /* For registration we send the data in network order no matter what */
    /* TODO: The timeout is not actually implemented */
    *((uint32_t *)&buff[0]) = htonl(1000);       /* Timeout  */
    *((uint32_t *)&buff[4]) = htonl(CONNECT_SYNC);  /* registration flags */
    strcpy(&buff[CON_HDR_SIZE], name);                /* The rest is the name */

    dax_log(DAX_LOG_COMM, "Sending registration for name - %s", ds->modulename);
    if((result = _message_send(ds, MSG_MOD_REG, buff, CON_HDR_SIZE + len)))
        return result;
    len = DAX_MSGMAX;

    result = _message_get(ds->sfd, &msg);
    if(result) {
    	return result;
    }

    /* Store the unique ID that the server has sent us. */
    ds->id =  *((uint32_t *)&msg.data[0]);
    /* Here we check to see if the data that we got in the registration message is in the same
       format as we use here on the client module. This should be offloaded to a separate
       function that can determine what needs to be done to the incoming and outgoing data to
       get it to match with the server */
    if( (*((uint16_t *)&msg.data[4]) != REG_TEST_INT) ||
        (*((uint32_t *)&msg.data[6]) != REG_TEST_DINT) ||
        (*((uint64_t *)&msg.data[10]) != REG_TEST_LINT)) {
        /* TODO: right now this is just to show error.  We need to determine if we can
           get the right data from the server by some means. */
        ds->reformat = REF_INT_SWAP;
    } else {
        ds->reformat = 0; /* this is redundant, already done in dax_init() */
    }
    /* There has got to be a better way to compare that we are getting good floating point numbers */
    if( fabs(*((float *)&msg.data[18]) - REG_TEST_REAL) / REG_TEST_REAL   > 0.0000001 ||
        fabs(*((double *)&msg.data[22]) - REG_TEST_LREAL) / REG_TEST_REAL > 0.0000001) {
        ds->reformat |= REF_FLT_SWAP;
    }
    /* TODO: returning _reformat is only good until we figure out how to reformat the
     * messages. Then we should return 0.  Right now since there isn't any reformating
     * of messages being done we consider it an error and return that so that the module
     * won't try to communicate */
    return ds->reformat;
}

/* This function retrieves one message using the _message_get() function and decides whether
 * to add the message to a FIFO of event messages or to store it on last_msg.  The event FIFO
 * and the last_msg pointer are both protected by a condition variable.  This function is
 * called from the connection thread and functions that expect to either receive a response
 * message or an event use these condition variables to wait on these mechanims. */
static int
_read_next_message(dax_state *ds)
{
    static unsigned int events_lost;
    dax_message *msg;
    int result, n;

    msg = malloc(sizeof(dax_message));
    if(msg == NULL) return ERR_ALLOC;

    result = _message_get(ds->sfd, msg);
    if(result) {
        if(result == ERR_DISCONNECTED) {
            dax_log(DAX_LOG_ERROR, "Server disconnected abruptly");
        } else if(result == ERR_TIMEOUT) {
            ; /* Do nothing for timeout */
        } else {
            dax_log(DAX_LOG_ERROR, "_message_get() returned error %d", result);
        }
        free(msg);
        return result;
    }
    if(msg->msg_type & MSG_EVENT) { /* Events we store in the FIFO */
        pthread_mutex_lock(&ds->event_lock);
        if(ds->emsg_queue_count == ds->emsg_queue_size) {/* FIFO is full */
            if(events_lost % 20 == 0) { /* We only log every 20 of these */
                events_lost++;
                dax_log(DAX_LOG_ERROR, "Event received from the server is lost.  Total = %u", events_lost);
            }
            free(ds->emsg_queue[0]); /* Free the top one */
            for(n = 0;n<ds->emsg_queue_size-1;n++) {
                ds->emsg_queue[n] = ds->emsg_queue[n+1];
            }
            ds->emsg_queue[ds->emsg_queue_size - 1] = msg;
        } else {
            ds->emsg_queue[ds->emsg_queue_count] = msg;
            ds->emsg_queue_count++;
        }
        pthread_mutex_unlock(&ds->event_lock);
        pthread_cond_signal(&ds->event_cond);
    } else { /* All other messages we put here */
        pthread_mutex_lock(&ds->msg_lock);
        ds->last_msg = msg;
        pthread_mutex_unlock(&ds->msg_lock);
        pthread_cond_signal(&ds->msg_cond);
    }
    return 0;
}

static void
_connection_cleanup(dax_state *ds) {
    ds->sfd = -1;
    if(ds->last_msg != NULL) free(ds->last_msg);
    ds->last_msg = NULL;
    free_tag_cache(ds);
    /* TODO: Free up the event FIFO too */
}

static void *
_connection_thread(void *arg)
{
    int result;

    dax_state *ds;
    ds = (dax_state *)arg;

    /* This is the connection that we used for all the functional
     * request / response messages. */
    ds->sfd = _get_connection(ds);
    if(ds->sfd >= 0) {
        result = _mod_register(ds, ds->modulename);
        init_tag_cache(ds);
        /* This basically let's the dax_connect function return success */
        ds->error_code = 0;
        pthread_barrier_wait(&ds->connect_barrier);
        /* dax_disconnect() will set this to zero to get us out of this loop */
        while(ds->sfd >= 0) { /* Main connection loop */
            result = _read_next_message(ds);
            if(result == ERR_DISCONNECTED) {
                if(ds->disconnect_callback) {
                    ds->disconnect_callback(result);
                }
                break;
                //_connection_cleanup(ds);
            }
        }
        _connection_cleanup(ds);
        ds->error_code = result;
        return NULL;
    } else {
        result = ds->sfd;
        ds->error_code = result;
        pthread_barrier_wait(&ds->connect_barrier);
    }
    return NULL;
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
    /* We use this barrier to stop here until we connect to the server the
     * first time. */
    pthread_barrier_init(&ds->connect_barrier, NULL, 2);

    pthread_create(&ds->connection_thread, NULL, _connection_thread, ds);
    pthread_detach(ds->connection_thread);
    pthread_barrier_wait(&ds->connect_barrier);

    if(ds->error_code == 0) {
        opt_lua_init_func(ds);
    }

    return ds->error_code;
}

/*!
 * Unregister our client module with the server and disconnect
 * from the server
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

    pthread_mutex_lock(&ds->lock);
    /* Tells the connection thread to exit */
    if(ds->sfd) {
        result = _message_send(ds, MSG_MOD_REG, NULL, 0);
        if(! result ) {
            len = 0;
            result = _message_recv(ds, MSG_MOD_REG, NULL, &len, 1);
        }
        close(ds->sfd);
        ds->sfd = 0;
    }
    pthread_mutex_unlock(&ds->lock);
    return result;
}

/* Not yet implemented */


/*!
 * Adds a tag to the tag server.
 * @param ds The pointer to the dax state object
 * @param h  Pointer to a tag handle structure.  This structure will be filled
 *           in by this function if it is successful.  NULL may be passed if
 *           the caller is not interested in the tag handle
 * @param name Pointer to the name of the new name
 * @param type Data type.
 * @param attr A bit field set of attributes for the tag
 * @param count If count is greater than 1 then an array is created.
 *
 * @returns Zero on success and an error code otherwise
 */
int
dax_tag_add(dax_state *ds, tag_handle *h, char *name, tag_type type, int count, uint32_t attr)
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
        size += 13;
        /* TODO Need to do some more error checking here */
        *((uint32_t *)&buff[0]) = mtos_udint(type);
        *((uint32_t *)&buff[4]) = mtos_udint(count);
        *((uint32_t *)&buff[8]) = mtos_udint(attr);

        strcpy(&buff[12], name);
    } else {
        return ERR_TAG_BAD;
    }
    pthread_mutex_lock(&ds->lock);

    result = _message_send(ds, MSG_TAG_ADD, buff, size);
    if(result) {
        pthread_mutex_unlock(&ds->lock);
        return result;
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
        tag.attr = attr;
        /* Just in case this call modifies the tag */
        cache_tag_del(ds, tag.idx);
        cache_tag_add(ds, &tag);
    }
    pthread_mutex_unlock(&ds->lock);
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
    *((uint32_t *)&buff[0]) = mtos_udint(index);

    pthread_mutex_lock(&ds->lock);
    result = _message_send(ds, MSG_TAG_DEL, buff, sizeof(buff));
    if(result) {
        pthread_mutex_unlock(&ds->lock);
        return result;
    }

    size = 4; /* we just need the handle */
    result = _message_recv(ds, MSG_TAG_DEL, buff, &size, 1);
    pthread_mutex_unlock(&ds->lock);
    return result;
}

/*!
 * Retrieve a tag definition based on the tags name.
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
    if(size == 0) return ERR_NOTFOUND;

    pthread_mutex_lock(&ds->lock);
    if(check_cache_name(ds, name, tag)) {
        /* We make buff big enough for the outgoing message and the incoming
           response message which would have 4 additional int32s */
        buff = malloc(size + 18);
        if(buff == NULL) return ERR_ALLOC;
        buff[0] = TAG_GET_NAME;
        strcpy(&buff[1], name);
        /* Send the message to the server.  Add 2 to the size for the subcommand and the NULL */
        result = _message_send(ds, MSG_TAG_GET, buff, size + 2);
        if(result) {
            dax_log(DAX_LOG_ERROR, "Can't send MSG_TAG_GET message");
            free(buff);
            pthread_mutex_unlock(&ds->lock);
            return result;
        }
        size += 18; /* This makes room for the type, count and handle */

        result = _message_recv(ds, MSG_TAG_GET, buff, &size, 1);
        if(result) {
            free(buff);
            pthread_mutex_unlock(&ds->lock);
            return result;
        }
        tag->idx = stom_dint( *((int *)&buff[0]) );
        tag->type = stom_udint(*((uint32_t *)&buff[4]));
        tag->count = stom_udint(*((uint32_t *)&buff[8]));
        tag->attr = stom_udint(*((uint16_t *)&buff[12]));
        buff[size - 1] = '\0'; /* Just to make sure */
        strcpy(tag->name, &buff[14]);
        cache_tag_add(ds, tag);
        free(buff);
    }
    pthread_mutex_unlock(&ds->lock);
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
    char buff[DAX_TAGNAME_SIZE + 17];

    pthread_mutex_lock(&ds->lock);
    if(check_cache_index(ds, idx, tag)) {
        buff[0] = TAG_GET_INDEX;
        *((tag_index *)&buff[1]) = idx;
        result = _message_send(ds, MSG_TAG_GET, buff, sizeof(tag_index) + 1);
        if(result) {
            dax_log(DAX_LOG_ERROR, "Can't send MSG_TAG_GET message");
            pthread_mutex_unlock(&ds->lock);
            return result;
        }
        /* Maximum size of buffer, the 17 is the NULL plus four integers */
        size = DAX_TAGNAME_SIZE + 17;
        result = _message_recv(ds, MSG_TAG_GET, buff, &size, 1);
        if(result) {
            pthread_mutex_unlock(&ds->lock);
            return result;
        }
        tag->idx = stom_dint(*((int32_t *)&buff[0]));
        tag->type = stom_dint(*((int32_t *)&buff[4]));
        tag->count = stom_dint(*((int32_t *)&buff[8]));
        tag->attr = stom_dint(*((int16_t *)&buff[12]));
        buff[DAX_TAGNAME_SIZE + 14] = '\0'; /* Just to be safe */
        strcpy(tag->name, &buff[14]);
        /* Add the tag to the tag cache */
        cache_tag_add(ds, tag);
    }
    pthread_mutex_unlock(&ds->lock);
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
 * @param size The number of bytes to read.
 *
 * @returns Zero upon success or an error code otherwise
 */
int
dax_read(dax_state *ds, tag_index idx, uint32_t offset, void *data, size_t size)
{
    int result = 0;
    uint8_t buff[14];

    /* If we try to read more data that can be held in a single message we return an error */
    if(size > MSG_DATA_SIZE) {
        return ERR_2BIG;
    }

    *((tag_index *)&buff[0]) = mtos_dint(idx);
    *((uint32_t *)&buff[4]) = mtos_dint(offset);
    *((uint32_t *)&buff[8]) = mtos_dint(size);

    pthread_mutex_lock(&ds->lock);
    result = _message_send(ds, MSG_TAG_READ, (void *)buff, sizeof(buff));
    if(result) {
        pthread_mutex_unlock(&ds->lock);
        return result;
    }
    result = _message_recv(ds, MSG_TAG_READ, (char *)data, &size, 1);

    if(result) {
        pthread_mutex_unlock(&ds->lock);
        if(result == ERR_DELETED) {
            cache_tag_del(ds, idx);
        }
        return result;
    }
    pthread_mutex_unlock(&ds->lock);
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
dax_write(dax_state *ds, tag_index idx, uint32_t offset, void *data, size_t size)
{
    size_t sendsize;
    int result;
    char buff[MSG_DATA_SIZE];

    /* This calculates the amount of data that we can send with a single message
       It subtracts a handle_t from the data size for use as the tag handle and
       an int, because we'll send the handle and the offset.*/
    sendsize = size + sizeof(tag_index) + sizeof(uint32_t);
    /* It is assumed that the flags that we want to set are the first 4 bytes are in *data */
    /* If we try to read more data that can be held in a single message we return an error */
    if(sendsize > MSG_DATA_SIZE) {
        return ERR_2BIG;
    }

    /* Write the data to the message buffer */
    *((tag_index *)&buff[0]) = mtos_dint(idx);
    *((uint32_t *)&buff[4]) = mtos_dint(offset);

    memcpy(&buff[8], data, size);

    pthread_mutex_lock(&ds->lock);
    result = _message_send(ds, MSG_TAG_WRITE, buff, sendsize);
    if(result) {
        pthread_mutex_unlock(&ds->lock);
        return result;
    }
    result = _message_recv(ds, MSG_TAG_WRITE, buff, 0, 1);
    if(result) {
        pthread_mutex_unlock(&ds->lock);
        if(result == ERR_DELETED) {
            cache_tag_del(ds, idx);
        }
        return result;
    }
    pthread_mutex_unlock(&ds->lock);
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
dax_mask(dax_state *ds, tag_index idx, uint32_t offset, void *data, void *mask, size_t size)
{
    size_t sendsize;
    uint8_t buff[MSG_DATA_SIZE];
    int result;

    /* This calculates the amount of data that we can send with a single message
       It subtracts a handle_t from the data size for use as the tag handle.*/
    sendsize = size*2 + sizeof(tag_index) + sizeof(uint32_t);
    /* It is assumed that the flags that we want to set are the first 4 bytes are in *data */
    /* If we try to read more data that can be held in a single message we return an error */
    if(sendsize > MSG_DATA_SIZE) {
        return ERR_2BIG;
    }
    /* Write the data to the message buffer */
    *((tag_index *)&buff[0]) = mtos_dint(idx);
    *((uint32_t *)&buff[4]) = mtos_dint(offset);
    memcpy(&buff[8], data, size);
    memcpy(&buff[8 + size], mask, size);

    pthread_mutex_lock(&ds->lock);
    result = _message_send(ds, MSG_TAG_MWRITE, buff, sendsize);
    if(result) {
        pthread_mutex_unlock(&ds->lock);
        return result;
    }
    result = _message_recv(ds, MSG_TAG_MWRITE, buff, 0, 1);
    if(result) {
        pthread_mutex_unlock(&ds->lock);
        if(result == ERR_DELETED) {
            cache_tag_del(ds, idx);
        }
        return result;
    }
    pthread_mutex_unlock(&ds->lock);
    return 0;
}

/*!
 * Used to add an override to the given tag
 * @param ds Pointer to the dax state object.
 * @param handle handle of the tag data to override
 * @param data Pointer to the data that we are using as the override
 *
 * @returns Zero upon success or an error code otherwise
*/
int
dax_tag_add_override(dax_state *ds, tag_handle handle, void *data) {
    int i, n, result = 0, size, sendsize;
    uint8_t *mask, *newdata = NULL;
    uint8_t buff[MSG_DATA_SIZE];

    size = handle.size;
    *((tag_index *)&buff[0]) = mtos_dint(handle.index);
    *((uint32_t *)&buff[4]) = mtos_dint(handle.byte);

    if(handle.type == DAX_BOOL && (handle.bit > 0 || handle.count % 8 )) {
        /* This takes care of the case where we have an even 8 bits but
         * we are offset from zero so we need an extra byte */
        if(handle.bit && !(handle.count % 8)) {
            size++;
        }
        mask = malloc(size);
        if(mask == NULL) return ERR_ALLOC;
        newdata = malloc(size);
        if(newdata == NULL) {
            free(mask);
            return ERR_ALLOC;
        }
        bzero(mask, size);
        bzero(newdata, size);

        i = handle.bit % 8;
        for(n = 0; n < handle.count; n++) {
            if( (0x01 << (n % 8)) & ((uint8_t *)data)[n / 8] ) {
                ((uint8_t *)newdata)[i / 8] |= (1 << (i % 8));
            }
            mask[i / 8] |= (1 << (i % 8));
            i++;
        }
        memcpy(&buff[12], newdata, size);
    } else {
        mask = malloc(size);
        if(mask == NULL) return ERR_ALLOC;
        for(n = 0; n < size; n++) {
            mask[n] = 0xFF;
        }
        memcpy(&buff[12], data, size);
    }

    memcpy(&buff[12 + size], mask, size);
    sendsize = size * 2 + 12;
    if(sendsize > MSG_DATA_SIZE) {
        if(newdata != NULL) free(newdata);
        free(mask);
    }

    pthread_mutex_lock(&ds->lock);
    result = _message_send(ds, MSG_ADD_OVRD, buff, sendsize);
    if(newdata != NULL) free(newdata);
    free(mask);
    if(result) {
        pthread_mutex_unlock(&ds->lock);
        return result;
    }
    result = _message_recv(ds, MSG_ADD_OVRD, buff, 0, 1);
    if(result) {
        pthread_mutex_unlock(&ds->lock);
        return result;
    }
    pthread_mutex_unlock(&ds->lock);
    return 0;
}

/*!
 * Removed an override on the given tag
 * @param ds Pointer to the dax state object.
 * @param handle handle of the tag data to override
 *
 * @returns Zero upon success or an error code otherwise
*/
int
dax_tag_del_override(dax_state *ds, tag_handle handle) {
    int i, n, result = 0, size, sendsize;
    uint8_t *mask = NULL;
    uint8_t buff[MSG_DATA_SIZE];
    size = handle.size;

    *((tag_index *)&buff[0]) = mtos_dint(handle.index);
    *((uint32_t *)&buff[4]) = mtos_dint(handle.byte);

    if(handle.type == DAX_BOOL && (handle.bit > 0 || handle.count % 8 )) {
        /* This takes care of the case where we have an even 8 bits but
         * we are offset from zero so we need an extra byte */
        if(handle.bit && !(handle.count % 8)) {
            size++;
        }
        mask = malloc(size);
        if(mask == NULL) return ERR_ALLOC;
        bzero(mask, size);

        i = handle.bit % 8;
        for(n = 0; n < handle.count; n++) {
            mask[i / 8] |= (1 << (i % 8));
            i++;
        }
    } else {
        mask = malloc(size);
        if(mask == NULL) return ERR_ALLOC;
        for(n = 0; n < size; n++) {
            mask[n] = 0xFF;
        }
    }

    memcpy(&buff[8], mask, size);
    sendsize = size + 8;
    if(sendsize > MSG_DATA_SIZE) {
        free(mask);
    }

    pthread_mutex_lock(&ds->lock);
    result = _message_send(ds, MSG_DEL_OVRD, buff, sendsize);
    free(mask);
    if(result) {
        pthread_mutex_unlock(&ds->lock);
        return result;
    }
    result = _message_recv(ds, MSG_DEL_OVRD, buff, 0, 1);
    if(result) {
        pthread_mutex_unlock(&ds->lock);
        return result;
    }
    pthread_mutex_unlock(&ds->lock);

    return 0;
}

/*!
 * Retrieve the override mask as well as the actual data of the tag.  When the override
 * is set this is the only way to read the 'actual' value as the normal tag reading functions
 * will return the overridden value.
 * @param ds Pointer to the dax state object.
 * @param handle handle of the tag data to override
 * @param data Pointer to the data buffer
 * @param mask Pointer to the mask buffer
 *
 * @returns Zero upon success or an error code otherwise
*/
int
dax_tag_get_override(dax_state *ds, tag_handle handle, void *data, void *mask) {
    int result = 0, sendsize;
    size_t size;
    uint8_t buff[MSG_DATA_SIZE];

    *((tag_index *)&buff[0]) = mtos_dint(handle.index);
    *((uint32_t *)&buff[4]) = mtos_dint(handle.byte);
    *((uint32_t *)&buff[8]) = mtos_dint(handle.size);

    sendsize = 12;
    pthread_mutex_lock(&ds->lock);
    result = _message_send(ds, MSG_GET_OVRD, buff, sendsize);
    if(result) {
        pthread_mutex_unlock(&ds->lock);
        return result;
    }
    size = handle.size * 2;
    result = _message_recv(ds, MSG_GET_OVRD, buff, &size, 1);
    if(result) {
        pthread_mutex_unlock(&ds->lock);
        return result;
    }
    pthread_mutex_unlock(&ds->lock);
    memcpy(data, buff, handle.size);
    memcpy(mask, &buff[handle.size], handle.size);
    return result;
}

/*!
 * Causes the override to become active.  After this command is sent to
 * the tag server the tag server will start sending the value that was
 * given in the dax_tag_add_override() instead of the actual value.  Other
 * modules can still write to the tag but the read functions will still
 * return the values in the override.
 *
 * @param ds Pointer to the dax state object.
 * @param handle handle of the tag data to override
 *
 * @returns Zero upon success or an error code otherwise
*/
int
dax_tag_set_override(dax_state *ds, tag_handle handle) {
    int result = 0, sendsize;
    uint8_t buff[MSG_DATA_SIZE];

    *((tag_index *)&buff[0]) = mtos_dint(handle.index);
    *((uint32_t *)&buff[4]) = 0xFF;

    sendsize = 5;
    pthread_mutex_lock(&ds->lock);
    result = _message_send(ds, MSG_SET_OVRD, buff, sendsize);
    if(result) {
        pthread_mutex_unlock(&ds->lock);
        return result;
    }
    result = _message_recv(ds, MSG_SET_OVRD, buff, 0, 1);
    if(result) {
        pthread_mutex_unlock(&ds->lock);
        return result;
    }
    pthread_mutex_unlock(&ds->lock);
    return 0;
}

/*!
 * Causes the override to become inactive.  After this command is sent to
 * the tag server the tag server will again start sending the actual data
 * of the tag instead of the override value.  Essentially the tag will
 * behave normally.  The override still exists and can be made active again
 * with the dax_tag_set_override()
 *
 * @param ds Pointer to the dax state object.
 * @param handle handle of the tag data to override
 *
 * @returns Zero upon success or an error code otherwise
*/
int
dax_tag_clr_override(dax_state *ds, tag_handle handle) {
    int result = 0, sendsize;
    uint8_t buff[MSG_DATA_SIZE];

    *((tag_index *)&buff[0]) = mtos_dint(handle.index);
    *((uint32_t *)&buff[4]) = 0x00;

    sendsize = 5;

    pthread_mutex_lock(&ds->lock);
    result = _message_send(ds, MSG_SET_OVRD, buff, sendsize);
    if(result) {
        pthread_mutex_unlock(&ds->lock);
        return result;
    }
    result = _message_recv(ds, MSG_SET_OVRD, buff, 0, 1);
    if(result) {
        pthread_mutex_unlock(&ds->lock);
        return result;
    }
    pthread_mutex_unlock(&ds->lock);

    return 0;
}


/*!
 * Atomic operations are operations that take place on the server without the
 * possibility of another client module modifying the data in during the operation.
 * This eliminates the race condition that exists with normal read/modify/write
 * operations.
 *
 * @param ds Pointer to the dax state object
 * @param h  Pointer to a handle that represents the tag or the part of the
 *           tag that we wish to apply the operation to.
 * @param data Any data that may be required by this operation.
 *
 * @param operation Number representing the operation that we wish to perform.
 *
 * @returns Zero on success or an error code otherwise.
 */
int
dax_atomic_op(dax_state *ds, tag_handle h, void *data, uint16_t operation) {
    size_t sendsize;
    int result;
    uint8_t buff[MSG_DATA_SIZE];

    /* This calculates the amount of data that we can send with a single message */
    sendsize = h.size + 21;
    if(sendsize > MSG_DATA_SIZE) {
        return ERR_2BIG;
    }
    if(IS_CUSTOM(h.type)) {
        return ERR_ILLEGAL;
    }
    *(dax_dint *)buff = mtos_dint(h.index);       /* Index */
    *(dax_dint *)&buff[4] = mtos_dint(h.byte);    /* Byte offset */
    *(dax_dint *)&buff[8] = mtos_dint(h.count);   /* Tag Count */
    *(dax_dint *)&buff[12] = mtos_dint(h.type);   /* Data Type */
    buff[16]=h.bit;                               /* Bit offset */
    *(dax_dint *)&buff[17] = mtos_uint(operation);/* Operation */

    if(data != NULL) {
        memcpy(&buff[21], data, h.size);
    }

    pthread_mutex_lock(&ds->lock);
    result = _message_send(ds, MSG_ATOMIC_OP, buff, sendsize);
    if(result) {
        pthread_mutex_unlock(&ds->lock);
        return result;
    }
    result = _message_recv(ds, MSG_ATOMIC_OP, buff, 0, 1);
    if(result) {
        pthread_mutex_unlock(&ds->lock);
        if(result == ERR_DELETED) {
            cache_tag_del(ds, h.index);
        }
        return result;
    }
    pthread_mutex_unlock(&ds->lock);
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
 * @returns Zero on success or an error code otherwise.
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

    pthread_mutex_lock(&ds->lock);
    result = _message_send(ds, MSG_EVNT_ADD, buff, size);
    if(result) {
        pthread_mutex_unlock(&ds->lock);
        return result;
    } else {
        test = _message_recv(ds, MSG_EVNT_ADD, &result, &size, 1);
        if(test) {
            pthread_mutex_unlock(&ds->lock);
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
                pthread_mutex_unlock(&ds->lock);
                return result;
            }
        }
    }
    pthread_mutex_unlock(&ds->lock);
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

    pthread_mutex_lock(&ds->lock);
    result = _message_send(ds, MSG_EVNT_DEL, buff, size);
    if(result) {
        pthread_mutex_unlock(&ds->lock);
        return result;
    } else {
        test = _message_recv(ds, MSG_EVNT_DEL, &result, &size, 1);
        if(test) {
            pthread_mutex_unlock(&ds->lock);
            return test;
        } else {
            del_event(ds, id);
        }
    }
    pthread_mutex_unlock(&ds->lock);
    return result;
}

/*!
 * Retrieve the details of the given event.
 * This function is not yet implemented
 */
int
dax_event_get(dax_state *ds, dax_id id)
{
    return ERR_NOTIMPLEMENTED;
    //return 0;
}

/*!
 * Set options flags on the given event.
 *
 * @param ds Pointer to the dax state object.
 * @param id The identifier of the event.
 * @param options Options bits
 *
 * @returns Zero on success or an error code otherwise
 */
int
dax_event_options(dax_state *ds, dax_id id, uint32_t options)
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

    pthread_mutex_lock(&ds->lock);
    result = _message_send(ds, MSG_EVNT_OPT, buff, size);
    if(result) {
        pthread_mutex_unlock(&ds->lock);
        return result;
    } else {
        test = _message_recv(ds, MSG_EVNT_OPT, &result, &size, 1);
        pthread_mutex_unlock(&ds->lock);
        return test;
    }
    pthread_mutex_unlock(&ds->lock);
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

    pthread_mutex_lock(&ds->lock);
    result = _message_send(ds, MSG_CDT_CREATE, buff, size);

    if(result) {
        pthread_mutex_unlock(&ds->lock);
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
    pthread_mutex_unlock(&ds->lock);
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
 *                 to retrieve this type
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
        *((uint32_t *)&buff[1]) = mtos_udint(cdt_type); /* type in the next four */
        size = 5;
    }

    pthread_mutex_lock(&ds->lock);
    result = _message_send(ds, MSG_CDT_GET, buff, size);

    if(result) {
        pthread_mutex_unlock(&ds->lock);
        return result;
    }

    size = MSG_DATA_SIZE;
    result = _message_recv(ds, MSG_CDT_GET, buff, &size, 1);
    pthread_mutex_unlock(&ds->lock);
    if(result == 0) {
        type = stom_udint(*((tag_type *)buff));
        result = add_cdt_to_cache(ds, type, &(buff[4]));
    }
    return result;
}

/*!
 * This function sends a message to add a data mapping to the server.
 * It takes two Handles as arguments.  The first is for the source data
 * point and the second is for the destination.  The id pointer will have
 * the unique id of the mapping if the function returns successfully.
 *
 * @param ds Pointer to the dax state object
 * @param src pointer to a tag handle for the data source
 * @param dest Pointer to a tag handle for the destination of the map
 * @param id Pointer to a structure that will be filled in with the
 *           id of the map
 * @returns Zero on success or an error code otherwise
 * */
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

    pthread_mutex_lock(&ds->lock);
    result = _message_send(ds, MSG_MAP_ADD, buff, size);

    if(result) {
        pthread_mutex_unlock(&ds->lock);
        return result;
    }

    size = MSG_DATA_SIZE;
    result = _message_recv(ds, MSG_MAP_ADD, buff, &size, 1);
    if(result == 0) {
        if(id != NULL) {
            id->id = *(tag_index *)buff;
            id->index = src->index;
        }
    }
    pthread_mutex_unlock(&ds->lock);
    return result;
}

/*!
 * This function deletes a tag mapping
 *
 * @param ds Pointer to the dax state object
 * @param src Pointer to the source handle that will be returned if found
 * @param dest Pointer to the destination handle that will be returned if found
 * @param id The id of the mapping that will be deleted.  This would have
 *           been assigned in the dax_map_add() function
 * @returns Zero on success or an error code otherwise
 * */
int
dax_map_get(dax_state *ds, tag_handle *src, tag_handle *dest, dax_id id) {
    int result;
    dax_dint temp;
    size_t size;

    char buff[MSG_DATA_SIZE];

    temp = mtos_dint(id.index);    /* Index */
    memcpy(buff, &temp, 4);
    temp = mtos_dint(id.id);       /* map id */
    memcpy(&buff[4], &temp, 4);

    pthread_mutex_lock(&ds->lock);
    result = _message_send(ds, MSG_MAP_GET, buff, 8);
    if(result) {
        pthread_mutex_unlock(&ds->lock);
        return result;
    }

    size = MSG_DATA_SIZE;
    result = _message_recv(ds, MSG_MAP_GET, buff, &size, 1);
    pthread_mutex_unlock(&ds->lock);

    src->index = *(dax_dint *)buff;
    src->byte = *(dax_dint *)&buff[4];
    src->bit = buff[8];
    src->count = *(dax_dint *)&buff[9];
    src->size = *(dax_udint *)&buff[13];
    src->type = *(dax_dint *)&buff[17];

    dest->index = *(dax_dint *)&buff[21];
    dest->byte = *(dax_dint *)&buff[25];
    dest->bit = buff[29];
    dest->count = *(dax_dint *)&buff[30];
    dest->size = *(dax_udint *)&buff[34];
    dest->type = *(dax_dint *)&buff[38];

    return result;

}

/*!
 * This function deletes a tag mapping
 *
 * @param ds Pointer to the dax state object
 * @param id The id of the mapping that will be deleted.  This would have
 *           been assigned in the dax_map_add() function
 * @returns Zero on success or an error code otherwise
 * */
int
dax_map_del(dax_state *ds, dax_id id) {
    int result;
    dax_dint temp;
    size_t size;

    char buff[MSG_DATA_SIZE];

    temp = mtos_dint(id.index);    /* Index */
    memcpy(buff, &temp, 4);
    temp = mtos_dint(id.id);       /* map id */
    memcpy(&buff[4], &temp, 4);

    pthread_mutex_lock(&ds->lock);
    result = _message_send(ds, MSG_MAP_DEL, buff, 8);

    if(result) {
        pthread_mutex_unlock(&ds->lock);
        return result;
    }

    size = MSG_DATA_SIZE;
    result = _message_recv(ds, MSG_MAP_DEL, buff, &size, 1);

    pthread_mutex_unlock(&ds->lock);
    return result;
}

/*!
 * Add a data group to the tag server .  Data groups
 * are a way to aggregate tags or parts of tags into a single
 * group that can be read or written all at once.
 *
 * @param ds      Pointer to the dax state object
 * @param result  Pointer to the result 0 = success
 * @param h       Pointer to an array of tag_handles that define the group
 * @param count   Number of handles in the array
 * @param options Options Flags - Not Implemented set to zero
 * @returns       A pointer to a tag group object.  This will be filled in
 *                with all the information necessary to access this group.
 *                This will be used in all the functions that access the group.
 *                This function allocates memory so it will have be deleted with
 *                the dax_group_del() function to free everything.
 */
tag_group_id *
dax_group_add(dax_state *ds, int *result, tag_handle *h, int count, uint8_t options) {
    int offset, n;
    tag_group_id *id = NULL;
    size_t size, group_size;
    dax_dint temp;
    dax_udint u_temp;

    uint8_t buff[MSG_DATA_SIZE];
    *result = 0;
    /* Sanity check the sizes.  These checks are redundant because
     * the server also does them but this will keep from sending the message */
    if(count > TAG_GROUP_MAX_MEMBERS) {
        *result = ERR_ARG;
        return NULL;
    }
    group_size = 0;
    for(n=0; n<count; n++) {
        group_size += h[n].size;
        if(group_size > MSG_TAG_GROUP_DATA_SIZE) {
            *result = ERR_2BIG;
            return NULL;
        }
    }
    buff[0] = count;
    buff[1] = options; /* Not implemented yet */
    /* size of the handles array + the count and the options bytes */
    size = 21*count + 2;
    for(n=0; n<count; n++) {
        offset = 21*n + 2;
        temp = mtos_dint(h[n].index);
        memcpy(&buff[offset], &temp, 4);
        u_temp = mtos_udint(h[n].byte);
        memcpy(&buff[offset+4], &u_temp, 4);
        buff[offset+8] = h[n].bit;
        u_temp = mtos_udint(h[n].count);
        memcpy(&buff[offset+9], &u_temp, 4);
        u_temp = mtos_udint(h[n].size);
        memcpy(&buff[offset+13], &u_temp, 4);
        u_temp = mtos_udint(h[n].type);
        memcpy(&buff[offset+17], &u_temp, 4);
    }

    pthread_mutex_lock(&ds->lock);
    *result = _message_send(ds, MSG_GRP_ADD, buff, size);

    if(*result) {
        pthread_mutex_unlock(&ds->lock);
        return NULL;
    }
    size = MSG_DATA_SIZE;
    *result = _message_recv(ds, MSG_GRP_ADD, buff, &size, 1);
    if(*result == 0) {
        id = (tag_group_id *)malloc(sizeof(tag_group_id));
        if(id == NULL) {
            pthread_mutex_unlock(&ds->lock);
            *result = ERR_ALLOC;
            return NULL;
        }
        id->handles = (tag_handle *)malloc(sizeof(tag_handle)*count);
        if(id->handles == NULL) {
            pthread_mutex_unlock(&ds->lock);
            *result = ERR_ALLOC;
            return NULL;
        }
        memcpy(id->handles, h, sizeof(tag_handle)*count);
        id->index = *(uint32_t *)buff;
        id->count = count;
        id->options = options;
        id->size = group_size;
    }
    pthread_mutex_unlock(&ds->lock);
    return id;
}


/*!
 * Get the size of tag data group in bytes
 *
 * @param id      Pointer to the tag group id
 * @returns       The size of the group in bytes
 */
int
dax_group_get_size(tag_group_id *id) {
    return id->size;
}

/*!
 * Reads a tag data group from the server.  It reads the data from the server
 * and writes it into the data area pointed to by 'buff'.  buff must be large
 * enough to contain the entire tag data group.
 *
 * @param ds      Pointer to the dax state object
 * @param id      Pointer to the tag group id returned by dax_group_add()
 * @param data    Pointer to a buffer that will receive the data
 * @param size    Size of the above buffer
 * @returns       0 on success an error code otherwise
 */
int
dax_group_read(dax_state *ds, tag_group_id *id, void *data, size_t size) {
    int result;
    uint32_t u_temp;

    if(size < id->size) return ERR_ARG;
    u_temp = mtos_udint(id->index);
    memcpy(data, &u_temp, 4);

    pthread_mutex_lock(&ds->lock);
    result = _message_send(ds, MSG_GRP_READ, data, 4);
    if(result) {
        pthread_mutex_unlock(&ds->lock);
        return result;
    }

    size = MSG_DATA_SIZE;
    result = _message_recv(ds, MSG_GRP_READ, data, &size, 1);
    if(result) {
        pthread_mutex_unlock(&ds->lock);
        return result;
    }
    if(result == 0) {
        result = group_read_format(ds, id, data);
    }
    pthread_mutex_unlock(&ds->lock);
    return result;
}

/*!
 * Writes a tag data group to the server.
 *
 * @param ds      Pointer to the dax state object
 * @param id      Pointer to the tag group id returned by dax_group_add()
 * @param data    Pointer to the buffer that will be written
 *
 * @returns       0 on success an error code otherwise
 */
int
dax_group_write(dax_state *ds, tag_group_id *id, void *data) {
    int result;
    uint32_t u_temp;
    char buff[MSG_DATA_SIZE];

    u_temp = mtos_udint(id->index);
    memcpy(buff, &u_temp, 4);

    result = group_write_format(ds, id, data);
    if(result) return result;
    memcpy(&buff[4], data, id->size);
    pthread_mutex_lock(&ds->lock);
    result = _message_send(ds, MSG_GRP_WRITE, buff, id->size+4);
    if(result) {
        pthread_mutex_unlock(&ds->lock);
        return result;
    }

    result = _message_recv(ds, MSG_GRP_WRITE, NULL, 0, 1);
    pthread_mutex_unlock(&ds->lock);
    return result;
}

/*!
 * Deletes a tag data group from the server.
 *
 * @param ds      Pointer to the dax state object
 * @param id      Pointer to the tag group id returned by dax_group_add()
 *
 * @returns       0 on success an error code otherwise
 */
int
dax_group_del(dax_state *ds, tag_group_id *id) {
    int result;
     uint32_t u_temp;
     char buff[4];

     u_temp = mtos_udint(id->index);
     memcpy(buff, &u_temp, 4);

     pthread_mutex_lock(&ds->lock);
     result = _message_send(ds, MSG_GRP_DEL, buff, 4);
     if(result) {
         pthread_mutex_unlock(&ds->lock);
         return result;
     }

     result = _message_recv(ds, MSG_GRP_DEL, NULL, 0, 1);
     if(result == 0) {
         free(id->handles);
         free(id);
     }
     pthread_mutex_unlock(&ds->lock);
     return result;
}

