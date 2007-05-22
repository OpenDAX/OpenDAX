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
#include <opendcs.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <func.h>

static int __msqid;

/* Creates the main message queue for the program.  It's a fatal error if
   we cannot create this queue */
int msg_create_queue(void) {
    __msqid = msgget(DCS_IPC_KEY, (IPC_CREAT | IPC_EXCL | 0660));
    if(__msqid < 0) {
        __msqid=0;
        xfatal("Message Queue Cannot Be Created - %s",strerror(errno));
    }
    xlog(2,"Message Queue Created - id = %d",__msqid);
    return __msqid;
}

/* This destroys the queue if it was created */
void msg_destroy_queue(void) {
    int result=0;
    
    if(__msqid) {
        result=msgctl(__msqid,IPC_RMID,NULL);
    }
    if(result) {
        xerror("Unable to delete message queue");
    }
    xlog(2,"Message queue %d being destroyed",__msqid);
}

void msg_receive(void) {
    dcs_message dcsmsg;
    struct msgbuff *msg;
    int result;
    
    msg=xmalloc(sizeof(dcs_message));
    result=msgrcv(__msqid,msg,sizeof(dcs_message),1,0);
    if(result > 0) {
        memcpy(&dcsmsg,msg,sizeof(dcs_message));
        printf(dcsmsg.buff);
    } else {
        perror("msg_receive");
    }
}
