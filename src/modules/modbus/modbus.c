/* modbus.c - Modbus (tm) Communications Library
 * Copyright (C) 2006 Phil Birkelbach
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Source code file that contains all of the modbus protocol stuff.
 */


#include <sys/select.h>
#include <pthread.h>
#include "modbus.h"
#include "database.h"

extern dax_state *ds;
extern pthread_barrier_t port_barrier;

int master_loop(mb_port *);
int client_loop(mb_port *);

/* Calculates the difference between the two times */
unsigned long long
timediff(struct timeval oldtime,struct timeval newtime)
{
    return (newtime.tv_sec - oldtime.tv_sec) * 1000 +
           (newtime.tv_usec / 1000)-(oldtime.tv_usec / 1000);
};


/* Opens the port passed in m_port and starts the loop that
 * will handle the port */
int
mb_run_port(struct mb_port *m_port)
{
    int result;

    while(1) {
        /* If the port is not already open */
        if(!m_port->fd) {
            result = mb_open_port(m_port);
        }
        if(result) {
            dax_log(DAX_LOG_ERROR, "Failed to open port %s - %s", m_port->name, strerror(errno));
            pthread_barrier_wait(&port_barrier); /* For clients and masters we just need to get past this */
        } else {
            /* If the port is still not open then we inhibit the port and
             * let the _loop() functions deal with it */
            if(m_port->fd == 0 && m_port->devtype != MB_NETWORK) {
                m_port->inhibit = 1;
            }

            if(m_port->type == MB_MASTER) {
                pthread_barrier_wait(&port_barrier); /* For clients and masters we just need to get past this */
                if(m_port->protocol == MB_TCP) {
                    dax_log(DAX_LOG_MAJOR, "Starting client loop for %s", m_port->name);
                    return client_loop(m_port);
                } else {
                    dax_log(DAX_LOG_MAJOR, "Starting master loop for %s", m_port->name);
                    return master_loop(m_port);
                }
            } else if(m_port->type == MB_SLAVE) {
                if(m_port->protocol == MB_TCP) {
                    dax_log(DAX_LOG_MAJOR, "Start Server Loop for port %s", m_port->name);
                    result = server_loop(m_port);
                    if(result) dax_log(DAX_LOG_ERROR, "Server loop exited with error, %d port %s", result, m_port->name);
                } else {
                    pthread_barrier_wait(&port_barrier); /* Doesn't matter since the port is already open */
                    dax_log(DAX_LOG_MAJOR, "Start Slave Loop for port %s", m_port->name);
                    result = slave_loop(m_port);
                    if(result) dax_log(DAX_LOG_ERROR, "Slave loop exited with error, %d port %s", result, m_port->name);
                }
            } else {
                return MB_ERR_PORTTYPE;
            }
        }
        usleep(2e6);
    }
    return 0;
}


/* This is the primary event loop for a Modbus TCP client.  It calls the functions
   to send the request and receive the responses.  It also takes care of the
   retries and the counters. */
int
client_loop(mb_port *mp)
{
    long time_spent;
    struct mb_cmd *mc;
    struct timeval start, end;

    mp->running = 1; /* Tells the world that we are going */
    mp->attempt = 0;
    mp->dienow = 0;

    while(1) {
        gettimeofday(&start, NULL);
        if(mp->enable) { /* If enable=0 then pause for the scanrate and try again. */
            mc = mp->commands;

            while(mc != NULL) {
                /* Only if the command is enabled and the interval counter is over */
                if(mc->enable && (mc->mode & MB_CONTINUOUS) && (++mc->icount >= mc->interval)) {
                    mc->icount = 0;
                    if(mp->maxattempts) {
                        mp->attempt++;
                    }
                    if( mb_send_command(mp, mc) > 0 ) {
                        mp->attempt = 0; /* Good response, reset counter */
                    }
//                    if((mp->maxattempts && mp->attempt >= mp->maxattempts) || mp->dienow) {
//                        mp->inhibit_temp = 0;
//                        mp->inhibit = 1;
//                    }
                }
                if(mp->delay > 0) usleep(mp->delay * 1000);
                mc = mc->next; /* get next command from the linked list */
            } /* End of while for sending commands */
        }
        /* This calculates the length of time that it took to send the messages on this port
           and then subtracts that time from the port's scanrate and calls usleep to hold
           for the right amount of time.  */
        gettimeofday(&end, NULL);
        time_spent = (end.tv_sec-start.tv_sec)*1000 + (end.tv_usec/1000 - start.tv_usec/1000);
        /* If it takes longer than the scanrate then just go again instead of sleeping */
        if(time_spent < mp->scanrate) {
            if(!mp->persist) {
                mb_close_port(mp);
            }
            mp->scanning = 0; /* We're going to assume this is atomic for now */
            usleep((mp->scanrate - time_spent) * 1000);
            mp->scanning = 1;
        }
    }
    /* Close the port */
    mb_close_port(mp);
    mp->dienow = 0;
    mp->running = 0;
    return MB_ERR_PORTFAIL;
}


/* This is the primary event loop for a Modbus master.  It calls the functions
   to send the request and receive the responses.  It also takes care of the
   retries and the counters. */
int
master_loop(mb_port *mp)
{
    long time_spent;
    int result;
    struct mb_cmd *mc;
    struct timeval start, end;
    unsigned char bail = 0;

    mp->running = 1; /* Tells the world that we are going */
    mp->attempt = 0;
    mp->dienow = 0;

    while(1) {
        gettimeofday(&start, NULL);
        if(mp->enable && !mp->inhibit) { /* If enable=0 then pause for the scanrate and try again. */
            mc = mp->commands;
            while(mc != NULL && !bail) {
                /* Only if the command is enabled and the interval counter is over */
                if(mc->enable && (mc->mode & MB_CONTINUOUS) && (++mc->icount >= mc->interval)) {
                    mc->icount = 0;
                    if(mp->maxattempts) {
                        mp->attempt++;
                    }
                    if( mb_send_command(mp, mc) > 0 ) {
                        mp->attempt = 0; /* Good response, reset counter */
                    }
                    if((mp->maxattempts && mp->attempt >= mp->maxattempts) || mp->dienow) {
                        bail = 1;
                        mp->inhibit_temp = 0;
                        mp->inhibit = 1;
                    }
                }
                if(mp->delay > 0) usleep(mp->delay * 1000);
                mc = mc->next; /* get next command from the linked list */
            } /* End of while for sending commands */
        }
        if(mp->inhibit) {
            bail = 0;
            mb_close_port(mp);
            if(mp->inhibit_time) {
                sleep(mp->inhibit_time);
                result = mb_open_port(mp);
                if(result == 0) mp->inhibit = 0;
            } else {
                return MB_ERR_PORTFAIL;
            }
        }
        /* This calculates the length of time that it took to send the messages on this port
           and then subtracts that time from the port's scanrate and calls usleep to hold
           for the right amount of time.  */
        gettimeofday(&end, NULL);
        time_spent = (end.tv_sec-start.tv_sec)*1000 + (end.tv_usec/1000 - start.tv_usec/1000);
        /* If it takes longer than the scanrate then just go again instead of sleeping */
        if(time_spent < mp->scanrate) {
            if(!mp->persist) {
                mb_close_port(mp);
            }
            usleep((mp->scanrate - time_spent) * 1000);
        }
    }
    /* Close the port */
    mb_close_port(mp);
    mp->dienow = 0;
    mp->running = 0;
    return MB_ERR_PORTFAIL;
}

/* This function formulates and sends the Modbus RTU master request */
static int
sendRTUrequest(mb_port *mp, mb_cmd *cmd)
{
    uint8_t buff[MB_FRAME_LEN], length;
    uint16_t crc, temp;

    /* build the request message */
    buff[0]=cmd->node;
    buff[1]=cmd->function;

    switch (cmd->function) {
        case 1:
        case 2:
        case 3:
        case 4:
            COPYWORD(&buff[2], &cmd->m_register);
            COPYWORD(&buff[4], &cmd->length);
            length = 6;
            break;
        case 5:
            temp = *cmd->data;
            if(cmd->enable == MB_CONTINUOUS || (temp != cmd->lastcrc) || !cmd->firstrun ) {
                COPYWORD(&buff[2], &cmd->m_register);
                if(temp) buff[4] = 0xff;
                else     buff[4] = 0x00;
                buff[5] = 0x00;
                cmd->firstrun = 1;
                cmd->lastcrc = temp;
                length = 6;
                break;
            } else {
                return 0;
            }
            break;
        case 6:
            temp = *cmd->data;
            /* If the command is contiunous go, if conditional then
             check the last checksum against the current datatable[] */
            if(cmd->enable == MB_CONTINUOUS || (temp != cmd->lastcrc)) {
                COPYWORD(&buff[2], &cmd->m_register);
                COPYWORD(&buff[4], &temp);
                cmd->lastcrc = temp; /* Since it's a single just store the word */
                length = 6;
                break;
            } else {
                return 0;
            }
        case 15: /* Write multiple output coils */
            if(cmd->enable != MB_CONTINUOUS) {
                /* If we have more than one word then do a crc otherwise just copy the data */
                if(cmd->length > 16) {
                    crc = crc16(cmd->data, cmd->datasize);
                } else if(cmd->length > 8) {
                    crc = *(uint16_t *)cmd->data;
                } else {
                    crc = *cmd->data;
                }
            }
            if(cmd->enable == MB_CONTINUOUS || (crc != cmd->lastcrc)) {
                COPYWORD(&buff[2], &cmd->m_register);
                COPYWORD(&buff[4], &cmd->length);
                buff[6] = (cmd->length-1)/8 + 1;
                printf("Bytes = %d\n", buff[7]);
                for(int n = 0; n < buff[7]; n++) {
                    buff[7+n] = cmd->data[n];
                }
                cmd->lastcrc = crc; /* Store for next time */
                length = 7 + buff[7];
                break;
            } else {
                return 0;
            }
        case 16: /* Write multiple holding registers */
            if(cmd->enable != MB_CONTINUOUS) {
                crc = crc16(cmd->data, cmd->datasize);
            }
            if(cmd->enable == MB_CONTINUOUS || (crc != cmd->lastcrc)) {
                COPYWORD(&buff[2], &cmd->m_register);
                COPYWORD(&buff[4], &cmd->length);
                buff[6] = cmd->length*2;
                for(int n = 0; n < cmd->length; n++) {
                    COPYWORD(&buff[7+n*2], &cmd->data[n*2]);
                }
                cmd->lastcrc = crc; /* Store for next time */
                length = 7 + cmd->length*2;
                break;
            } else {
                return 0;
            }
        /* TODO: Add the rest of the function codes */
        default:
            break;
    }
    crc = crc16(buff, length);
    COPYWORD(&buff[length], &crc);
    /* Send Request */
    cmd->requests++; /* Increment the request counter */
    tcflush(mp->fd, TCIOFLUSH);
    /* Send the buffer to the callback routine. */
    if(mp->out_callback) {
        mp->out_callback(mp, buff, length + 2);
    }

    return write(mp->fd, buff, length + 2);
}

/*
 * This function waits for one interbyte timeout (wait) period and then
 * checks to see if there is data on the port.  If there is no data and
 * we still have not received any data then it compares the current time
 * against the time the loop was started to see about the timeout.  If there
 * is data then it is written into the buffer.  If there is no data on the
 * read() and we have received some data already then the full message should
 * have been received and the function exits.

 * Returns 0 on timeout
 * Returns -1 on CRC fail
 * Returns the length of the message on success
 */
static int
getRTUresponse(uint8_t *buff, mb_port *mp)
{
    unsigned int buffptr = 0;
    struct timeval oldtime, thistime;
    int result;

    gettimeofday(&oldtime, NULL);

    while(1) {
        usleep(mp->frame * 1000);
        result = read(mp->fd, &buff[buffptr], MB_FRAME_LEN);
        if(result > 0) { /* Get some data */
            buffptr += result; // TODO: WE NEED A BOUNDS CHECK HERE.  Seg Fault Commin'
        } else { /* Message is finished, good or bad */
            if(buffptr > 0) { /* Is there any data in buffer? */

                if(mp->in_callback) {
                    mp->in_callback(mp, buff, buffptr);
                }
                /* Check the checksum here. */
                result = crc16check(buff, buffptr);

                if(!result) return -1;
                else return buffptr;

            } else { /* No data in the buffer */
                gettimeofday(&thistime,NULL);
                if(timediff(oldtime, thistime) > mp->timeout) {
                    return 0;
                }
            }
        }
    }
    return 0; /* Should never get here */
}

/* We haven't implemented ASCII yet */
static int
sendASCIIrequest(mb_port *mp, mb_cmd *cmd)
{
    return 0;
}

static int
getASCIIresponse(uint8_t *buff, mb_port *mp)
{
    return 0;
}

static uint16_t _req_tid;

/* This function formulates and sends the Modbus TCP client request */
static int
sendTCPrequest(mb_port *mp, mb_cmd *cmd)
{
    uint8_t buff[MB_FRAME_LEN];
    uint16_t crc, temp, length;

    /* build the request message */
    /* MBAP Header minus the length.  We'll set it later */
    buff[0] = _req_tid>>8;  /* Transaction ID */
    buff[1] = _req_tid++;   /* Transaction ID */
    buff[2] = 0x00;  /* Protocol ID */
    buff[3] = 0x00;  /* Protocol ID */
    /* Modbus RTU PDU */
    buff[6] = cmd->node;     /* Unit ID */
    buff[7] = cmd->function; /* Function Code */

    switch (cmd->function) {
        case 1: /* Read Coils */
        case 2: /* Read Input Contacts */
        case 3: /* Read Holding Registers */
        case 4: /* Read Analog Inputs */
            COPYWORD(&buff[8], &cmd->m_register);
            COPYWORD(&buff[10], &cmd->length);
            length = 6;
            break;
        case 5: /* Write single coil */
            temp = *cmd->data;
            if(cmd->enable == MB_CONTINUOUS || (temp != cmd->lastcrc) || !cmd->firstrun ) {
                COPYWORD(&buff[8], &cmd->m_register);
                if(temp) buff[10] = 0xff;
                else     buff[10] = 0x00;
                buff[5] = 0x00;
                cmd->firstrun = 1;
                cmd->lastcrc = temp;
                length = 6;
                break;
            } else {
                return 0;
            }
            break;
        case 6: /* Write single Holding Register */
            temp = *cmd->data;
            /* If the command is continuous go, if conditional then
             check the last checksum against the current datatable[] */
            if(cmd->enable == MB_CONTINUOUS || (temp != cmd->lastcrc)) {
                COPYWORD(&buff[8], &cmd->m_register);
                COPYWORD(&buff[10], cmd->data);
                cmd->lastcrc = temp; /* Since it's a single just store the word */
                length = 6;
                break;
            } else {
                return 0;
            }
        case 15: /* Write multiple output coils */
            if(cmd->enable != MB_CONTINUOUS) {
                /* If we have more than one word then do a crc otherwise just copy the data */
                if(cmd->length > 16) {
                    crc = crc16(cmd->data, cmd->datasize);
                } else if(cmd->length > 8) {
                    crc = *(uint16_t *)cmd->data;
                } else {
                    crc = *cmd->data;
                }
            }
            if(cmd->enable == MB_CONTINUOUS || (crc != cmd->lastcrc)) {
                COPYWORD(&buff[8], &cmd->m_register);
                COPYWORD(&buff[10], &cmd->length);
                buff[12] = (cmd->length-1)/8 + 1;
                printf("Bytes = %d\n", buff[12]);
                for(int n = 0; n < buff[12]; n++) {
                    buff[13+n] = cmd->data[n];
                }
                cmd->lastcrc = crc; /* Store for next time */
                length = 7 + buff[12];
                break;
            } else {
                return 0;
            }
        case 16: /* Write multiple holding registers */
            if(cmd->enable != MB_CONTINUOUS) {
                crc = crc16(cmd->data, cmd->datasize);
            }
            if(cmd->enable == MB_CONTINUOUS || (crc != cmd->lastcrc)) {
                COPYWORD(&buff[8], &cmd->m_register);
                COPYWORD(&buff[10], &cmd->length);
                buff[12] = cmd->length*2;
                for(int n = 0; n < cmd->length; n++) {
                    COPYWORD(&buff[13+n*2], &cmd->data[n*2]);
                }
                cmd->lastcrc = crc; /* Store for next time */
                length = 7 + cmd->length*2;
                break;
            } else {
                return 0;
            }
        /* TODO: Add the rest of the function codes */
        default:
            break;
    }
    /* Go back and put the length in the MBAP Header */
    COPYWORD(&buff[4], &length);
    /* Send Request */
    cmd->requests++; /* Increment the request counter */
    tcflush(mp->fd, TCIOFLUSH);
    /* Send the buffer to the callback routine. */
    if(mp->out_callback) {
        mp->out_callback(mp, buff, length + 6);
    }

    return write(mp->fd, buff, length + 6);
}

/*
 * This function immediately tries to read data from the file handle.

 * Returns 0 on timeout
 * Returns the length of the message on success
 */
static int
getTCPresponse(uint8_t *buff, mb_port *mp)
{
    uint8_t tempbuff[MB_FRAME_LEN];
    struct timeval timeout;
    int result;
    fd_set readfs, errfs;


    timeout.tv_usec = (mp->timeout % 1000) * 1000;
    timeout.tv_sec = mp->timeout / 1000;
    FD_ZERO(&readfs);
    FD_SET(mp->fd, &readfs);
    FD_ZERO(&errfs);
    FD_SET(mp->fd, &errfs);
    result = select(mp->fd+1, &readfs, NULL, &errfs, &timeout);
    if(FD_ISSET(mp->fd, &readfs)) {
        /* TODO Can we really assume that we'll get the whole thing in one read() */
        result = read(mp->fd, tempbuff, MB_BUFF_SIZE);
    }
    if(result > 0) { /* Is there any data in buffer? */
        if(mp->in_callback) {
            mp->in_callback(mp, tempbuff, result);
        }
        /* We have to convert the TCP message to it's RTU equivalent,
         * because that is what the calling function is expecting. */
        result -= 6;
        memcpy(buff, &tempbuff[6], result);
    }
    return result;
}




/*!
 * This function takes the message buffer and the current command and
 * determines what to do with the message.  It may write data to the
 * datatable or just return if the message is an acknowledge of a write.
 * This function is protocol indifferent so the checksums should be
 * checked before getting here.  This function assumes that *buff looks
 * like an RTU message so the ASCII & TCP response functions should translate
 * the their responses into RTUish messages
 */
/* TODO: There is all kinds of buffer overflow potential here.  It should all be checked */
static int
handleresponse(uint8_t *buff, mb_cmd *cmd)
{
    int n;

    cmd->responses++;
    if(buff[1] >= 0x80)
        return buff[2];
    if(buff[0] != cmd->node)
        return ME_WRONG_DEVICE;
    if(buff[1] != cmd->function)
        return ME_WRONG_FUNCTION;
    /* If we get this far the message should be good */
    switch (cmd->function) {
        case 1:
        case 2:
            memcpy(cmd->data, &buff[3], buff[2]);
            break;
        case 3:
        case 4:
            /* TODO: There may be times when we get more data than cmd->length but how to deal with that? */
            for(n = 0; n < (buff[2] / 2); n++) {
                COPYWORD(&((uint16_t *)cmd->data)[n], &buff[(n * 2) + 3]);
            }
            break;
        case 5:
        case 6:
        case 15:
        case 16:
            break;
        default:
            break;
    }
    return 0;
}

/* This function is called before a write command request is sent.  It's purpose
 * is to read the data from the tagserver and put it in the command buffer.  First
 * it checks to see if we have already retrieved our handle from the
 * tag server.  If not then we attempt to retrieve it.  Then we make sure that
 * the sizes of the tag and the command buffer are the same and adjust if necessary.
 * If the handles has been retrieved then we simply read the data from the
 * tagserver and put it in the command data buffer so that it can be sent. */
static int
_get_write_data(mb_cmd *mc) {
    int result;

    if(mc->data_h.index == 0) {
        result = dax_tag_handle(ds, &mc->data_h, mc->data_tag, mc->tagcount);
        if(result) return result;
        /* We need to check if the tag size and the data buffer size in the modbus command
         * are the same.  If not then if the tag is smaller it's no big deal but if the command
         * data size is smaller then we'll truncate the size in the tag handle. */
        if(mc->data_h.size != mc->datasize) {
            dax_log(DAX_LOG_ERROR, "Tag size and Modbus request size are different.  Data may be truncated");
            if(mc->datasize < mc->data_h.size) mc->data_h.size = mc->datasize;
        }
    }
    /* If we get here we assume that we now have a valid tag handle */
    return dax_read_tag(ds, mc->data_h, mc->data);
}

/* This function is called after a command response has been received.  It's purpose
 * is to put the data into the tagserver.  It first checks to see if we have already
 * retrieved our handle from the tag server.  If not then we attempt to retrieve it.
 * Then we make sure that the sizes of the tag and the command buffer are the same
 * and adjust if necessary. If the handles has been retrieved then we simply write
 * the data from the command data buffer to the tagserver. */
static int
_send_read_data(mb_cmd *mc) {
    int result;

    if(mc->data_h.index == 0) {
        result = dax_tag_handle(ds, &mc->data_h, mc->data_tag, mc->tagcount);
        if(result) return result;
        /* We need to check if the tag size and the data buffer size in the modbus command
         * are the same.  If not then if the tag is smaller it's no big deal but if the command
         * data size is smaller then we'll truncate the size in the tag handle. */
        if(mc->data_h.size != mc->datasize) {
            dax_log(DAX_LOG_ERROR, "Tag size and Modbus request size are different.  Data will be truncated");
            if(mc->datasize < mc->data_h.size) mc->data_h.size = mc->datasize;
        }
    }
    /* If we get here we assume that we now have a valid tag handle */
    return dax_write_tag(ds, mc->data_h, mc->data);
}


/*!
 * External function to send a Modbus commaond (mc) to port (mp).  The function
 * sets some function pointers to the functions that handle the port protocol and
 * then uses those functions generically.  The retry loop tries the command for the
 * configured number of times and if successful returns 0.  If not, an error code
 * is returned. mb_buff should be at least MB_FRAME_LEN in length
 */

int
mb_send_command(mb_port *mp, mb_cmd *mc)
{
    uint8_t buff[MB_FRAME_LEN]; /* Modbus Frame buffer */
    int try = 1;
    int result, msglen;
    static int (*sendrequest)(struct mb_port *, struct mb_cmd *) = NULL;
    static int (*getresponse)(uint8_t *,struct mb_port *) = NULL;

    if(!mc->enable) return 0; /* If we are not enabled we don't send */
  /* This sets up the function pointers so we don't have to constantly check
   *  which protocol we are using for communication.  From this point on the
   *  code is generic for RTU or ASCII */
    if(mp->protocol == MB_RTU) {
        sendrequest = sendRTUrequest;
        getresponse = getRTUresponse;
    } else if(mp->protocol == MB_ASCII) {
        sendrequest = sendASCIIrequest;
        getresponse = getASCIIresponse;
    } else if(mp->protocol == MB_TCP) {
        sendrequest = sendTCPrequest;
        getresponse = getTCPresponse;
    } else {
        return -1;
    }
    pthread_mutex_lock(&mp->send_lock);
    if(mp->protocol == MB_TCP) {
        /* We get our fd from the pool for TCP connections and put it in the
         * port fd.  This just keeps it consistent with the serial port functions
         * so that we don't have to pass the fd to the send/get functions */
        mp->fd = mb_get_connection(mp, mc->ip_address, mc->port);
        if(mp->fd < 0) {
            pthread_mutex_unlock(&mp->send_lock);
            return MB_ERR_OPEN;
        }
    }
    /* Retrieve the data from the tag server */
    if(mb_is_write_cmd(mc)) {
        result = _get_write_data(mc);
    }
    do { /* retry loop */
        result = sendrequest(mp, mc);
        if(result > 0) {
            msglen = getresponse(buff, mp);
        } else if(result == 0) {
            /* Should be 0 when a conditional command simply doesn't run */
            if(!mp->scanning && !mp->persist) close(mp->fd);
            pthread_mutex_unlock(&mp->send_lock);
            return result;
        } else {
            if(!mp->scanning && !mp->persist) close(mp->fd);
            pthread_mutex_unlock(&mp->send_lock);
            return -1;
        }

        if(msglen > 0) {
            result = handleresponse(buff, mc); /* Returns 0 on success + on failure */
            if(result > 0) {
                mc->exceptions++;
                mc->lasterror = result | ME_EXCEPTION;
            } else { /* Everything is good */
                mc->lasterror = 0;
                /* Send the data to the tag server */
                if(mb_is_read_cmd(mc)) {
                    result = _send_read_data(mc);
                }
            }
            if(!mp->scanning && !mp->persist) close(mp->fd);
            pthread_mutex_unlock(&mp->send_lock);
            return msglen; /* We got some kind of message so no sense in retrying */
        } else if(msglen == 0) {
            mc->timeouts++;
            mc->lasterror = ME_TIMEOUT;
        } else {
            /* Checksum failed in response */
            mc->crcerrors++;
            mc->lasterror = ME_CHECKSUM;
        }
    } while(try++ <= mp->retries);
    /* After all the retries get out with error */
    /* TODO: Should set error code?? */
    if(!mp->scanning && !mp->persist) close(mp->fd);
    pthread_mutex_unlock(&mp->send_lock);
    return 0 - mc->lasterror;
}

static int
_create_exception(unsigned char *buff, uint16_t exception)
{
    buff[1] |= ME_EXCEPTION;
    buff[2] = exception;
    return 3;
}

/* Handles responses for function codes 1 and 2 */
static int
_read_bits_response(mb_port *port, int nodeid, unsigned char *buff, int size, int mbreg)
{
    int n, bit, word, buffbit, buffbyte;
    unsigned int regsize;
    uint16_t index, count;
    uint16_t reg[150];

    COPYWORD(&index, (uint16_t *)&buff[2]); /* Starting Address */
    COPYWORD(&count, (uint16_t *)&buff[4]); /* Number of disc/coils requested */

    if(mbreg == MB_REG_COIL) {
        slave_read_database(port->nodes[nodeid]->coil_idx, mbreg, index, count, reg);
        regsize = port->nodes[nodeid]->coil_size;
    } else {
        slave_read_database(port->nodes[nodeid]->disc_idx, mbreg, index, count, reg);
        regsize = port->nodes[nodeid]->disc_size;
    }

    if(((count - 1)/8+1) > (size - 3)) { /* Make sure we have enough room */
        return MB_ERR_OVERFLOW;
    }
    if((index + count) > regsize) {
        return _create_exception(buff, ME_BAD_ADDRESS);
    }
    buff[2] = (count - 1)/8+1;

    bit = 0;
    word = 0;
    buffbit = 0;
    buffbyte = 3;
    for(n = 0; n < count; n++) {
        if(reg[word] & (0x01 << bit)) {
            buff[buffbyte] |= (0x01 << buffbit);
        } else {
            buff[buffbyte] &= ~(0x01 << buffbit);
        }
        buffbit++;
        if(buffbit == 8) {
            buffbit = 0; buffbyte++;
        }
        bit++;
        if(bit == 16) {
            bit = 0; word++;
        }
    }
    return (count - 1)/8+4;
}

/* Handles responses for function codes 3 and 4 */
static int
_read_words_response(mb_port *port, int nodeid, unsigned char *buff, int size, int mbreg)
{
    int n;
    unsigned int regsize;
    uint16_t index, count;
    uint16_t reg[150]; /* This is where we will store the data */

    COPYWORD(&index, (uint16_t *)&buff[2]); /* Starting Address */
    COPYWORD(&count, (uint16_t *)&buff[4]); /* Number of words/coils */

    if(mbreg == MB_REG_HOLDING) {
        slave_read_database(port->nodes[nodeid]->hold_idx, mbreg, index, count, reg);
        regsize = port->nodes[nodeid]->hold_size;
    } else {
        slave_read_database(port->nodes[nodeid]->input_idx, mbreg, index, count, reg);
        regsize = port->nodes[nodeid]->input_size;
    }

    if((count * 2) > (size - 3)) { /* Make sure we have enough room */
        return MB_ERR_OVERFLOW;
    }
    if((index + count) > regsize) {
        return _create_exception(buff, ME_BAD_ADDRESS);
    }
    buff[2] = count * 2;
    for(n = 0; n < count; n++) {
        COPYWORD(&buff[3+(n*2)], &reg[n]);
    }
    return (count * 2) + 3;
}

/* This function makes sure that we have a register defined for the given
 * function code.  This is used to determine if we need to send a wrong
 * function code exception.
 */
int
_check_function_code_exception(mb_port *port, int nodeid, uint8_t function) {
    switch(function) {
        case 1:
        case 5:
        case 15:/* Read Coils */
            if(port->nodes[nodeid]->coil_size) return 0;
            break;
        case 2: /* Read Discrete Inputs */
            if(port->nodes[nodeid]->disc_size) return 0;
            break;
        case 3: /* Read Holding Registers */
        case 6:
        case 16:
            if(port->nodes[nodeid]->hold_size) return 0;
            break;
        case 4: /* Read Input Registers */
            if(port->nodes[nodeid]->input_size) return 0;
            break;
        case 8:
            if(port->protocol != MB_TCP) return 0;
            break;
        default:
            break;
    }
    return ME_WRONG_FUNCTION;
}

/* Creates a generic slave response.  buff should point to a buffer that
 * looks like an RTU request without the checksum.  This function will generate
 * an RTU response message, and write that back into buff.  size is the total
 * size that we can write into buff.  Returns positive number of bytes written, zero
 * if no bytes written and negative error code if there is a problem. */
int
create_response(mb_port *port, unsigned char *buff, int size)
{
    uint8_t node, function;
    uint16_t index, value, count;
    uint16_t data[128]; /* should be the largest Modbus data size */
    int word, bit, n;
    int8_t result, retval;
    lua_State *L;

    node = buff[0]; /* Node Number */
    function = buff[1]; /* Modbus Function Code */

    if(port->nodes[node] == NULL) { /* No configuration for this node */
        if(port->nodes[0] != NULL) { /* We use node 0 as the default */
            node = 0;
        } else {
            return 0; /* Otherwise we ignore */
        }
    }

    if(port->nodes[node]->read_callback != LUA_REFNIL) {
        if(function == 1 || function == 2 || function == 3 || function == 4)  {
            COPYWORD(&index, (uint16_t *)&buff[2]); /* Starting Address */
            COPYWORD(&count, (uint16_t *)&buff[4]); /* Count */
            L = dax_get_luastate(ds);

            lua_settop(L, 0); /* Delete the stack */
            /* Get the filter function from the registry and push it onto the stack */
            lua_rawgeti(L, LUA_REGISTRYINDEX, port->nodes[node]->read_callback);
            lua_pushinteger(L, node);     /* First argument is the node number */
            lua_pushinteger(L, function); /* Second argument is the function code */
            lua_pushinteger(L, index); /* Third argument is the requested register (0 based)  */
            lua_pushinteger(L, count); /* fourth argument is the count */
            if(lua_pcall(L, 4, 1, 0) != LUA_OK) {
                dax_log(DAX_LOG_ERROR, "callback funtion for %s - %d: %s", port->name, node, lua_tostring(L, -1));
            } else { /* Success */
                result = lua_tointeger(L, -1);
            }
        }
    }
    /* If the Lua callback function returns a non-zero integer then create an
       exception message with that integer */
    if(result) {
        return _create_exception(buff, result);
    }
    if(_check_function_code_exception(port, node, function)) {
        return _create_exception(buff, ME_WRONG_FUNCTION);
    }

    switch(function) {
        case 1: /* Read Coils */
            return _read_bits_response(port, node, buff, size, MB_REG_COIL);
        case 2: /* Read Discrete Inputs */
            return _read_bits_response(port, node, buff, size, MB_REG_DISC);
        case 3: /* Read Holding Registers */
            return _read_words_response(port, node, buff, size, MB_REG_HOLDING);
        case 4: /* Read Input Registers */
            return _read_words_response(port, node, buff, size, MB_REG_INPUT);
        case 5: /* Write Single Coil */
            COPYWORD(&index, (uint16_t *)&buff[2]); /* Starting Address */
            COPYWORD(&value, (uint16_t *)&buff[4]); /* Value */
            if(index >= port->nodes[node]->coil_size) {
                return _create_exception(buff, ME_BAD_ADDRESS);
            }
            if(value) {
                data[0] = 0x01;
            } else {
                data[0] = 0x00;
            }
            slave_write_database(port->nodes[node]->coil_idx, MB_REG_COIL, index, 1, data);
            retval = 6;
            break;
        case 6: /* Write Single Register */
            COPYWORD(&index, (uint16_t *)&buff[2]); /* Starting Address */
            COPYWORD(&value, (uint16_t *)&buff[4]); /* Value */
            if(index >= port->nodes[node]->hold_size) {
                return _create_exception(buff, ME_BAD_ADDRESS);
            }
            data[0] = value;
            slave_write_database(port->nodes[node]->hold_idx, MB_REG_HOLDING, index, 1, data);
            retval = 6;
            break;
        case 8:
            return size / 2;
        case 15: /* Write Multiple Coils */
            COPYWORD(&index, (uint16_t *)&buff[2]); /* Starting Address */
            COPYWORD(&count, (uint16_t *)&buff[4]); /* Count */
            if((index + count) > port->nodes[node]->coil_size) {
                return _create_exception(buff, ME_BAD_ADDRESS);
            }
            word = 0;
            bit = 0;
            for(n = 0; n < count; n++) {
                if(buff[7 + n/8] & (0x01 << (n%8))) {
                    data[word] |= (0x01 << bit);
                } else {
                    data[word] &= ~(0x01 << bit);
                }
                bit++;
                if(bit == 16) { bit = 0; word++; }
            }
            slave_write_database(port->nodes[node]->coil_idx, MB_REG_COIL, index, count, data);
            retval = 6;
            break;
        case 16: /* Write Multiple Registers */
            COPYWORD(&index, (uint16_t *)&buff[2]); /* Starting Address */
            COPYWORD(&count, (uint16_t *)&buff[4]); /* Count */
            if((index + count) > port->nodes[node]->hold_size) {
                return _create_exception(buff, ME_BAD_ADDRESS);
            }
            for(n = 0; n < count; n++) {
                COPYWORD(&data[n], &buff[7 + (n*2)]);
            }
            slave_write_database(port->nodes[node]->hold_idx, MB_REG_HOLDING, index, count, data);
            retval = 6;
            break;
        default:
            break;
    }

    /* We should only get this far if we have had a successful write response created*/
    if(port->nodes[node]->write_callback != LUA_REFNIL) {
        if(function == 5 || function == 6 || function == 15 || function == 16)  {
            L = dax_get_luastate(ds);

            lua_settop(L, 0); /* Delete the stack */
            /* Get the filter function from the registry and push it onto the stack */
            lua_rawgeti(L, LUA_REGISTRYINDEX, port->nodes[node]->write_callback);
            lua_pushinteger(L, node);     /* First argument is the node number */
            lua_pushinteger(L, function); /* Second argument is the function code */
            lua_pushinteger(L, index); /* Third argument is the requested register (0 based)  */
            if(function == 5 || function == 6) {
                lua_pushinteger(L, 1); /* fourth argument is the count for fc 5&6 it's going to be 1*/
            } else {
                lua_pushinteger(L, count); /* fourth argument is the count */
            }
            if(lua_pcall(L, 4, 1, 0) != LUA_OK) {
                dax_log(DAX_LOG_ERROR, "callback funtion for %s - %d: %s", port->name, node, lua_tostring(L, -1));
            } else { /* Success */
                result = lua_tointeger(L, -1);
            }
        }
    }
    /* If the write Lua callback returns non-zero then we create an exception for that
       result.  Otherwise just continue on */
    if(result) {
        return _create_exception(buff, result);
    }
    return retval;
}
