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
 *  Main source code file for the OpenDCS test module
 */

#include <common.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <opendcs.h>
#include <dcs/message.h>
#include <dcs/func.h>
#include <string.h>

int main(int argc,char *argv[]) {
    int msgid;
    int n=0;
    dcs_message outmsg;
    struct msgbuff *msg;
    msg=xmalloc(sizeof(dcs_message));
    
    msgid=msgget(DCS_IPC_KEY,0660);
    if(msgid < 0) {
        perror("msgget");
        exit(1);
    }
    
    while(1) {
        outmsg.handle=1;
        outmsg.function=0;
        outmsg.size=0;
        sprintf(outmsg.buff,"Hello DCS World %d\n",n++);
        memcpy(msg,&outmsg,sizeof(dcs_message));
        msgsnd(msgid,msg,sizeof(dcs_message),0);
    
        sleep(3);
        //printf("Waiting to die\n");
    }
}
