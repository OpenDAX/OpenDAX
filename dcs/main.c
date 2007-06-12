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
 */

#include <common.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>
#include <syslog.h>
#include <module.h>
#include <options.h>
#include <func.h>
#include <message.h>
#include <tagbase.h>

void child_signal(int);
void quit_signal(int);

int main(int argc, const char *argv[]) {
    struct sigaction sa;
    //dcs_module *mod;
    //dcs_tag list[24];
    int temp,n; //,i,j;
    //long int handle;
    //char buff[256];
    
    
    /* Set up the signal handlers */
    memset (&sa,0,sizeof(struct sigaction));
    sa.sa_handler=&child_signal;
    sigaction (SIGCHLD,&sa,NULL);
    sa.sa_handler=&quit_signal;
    sigaction (SIGQUIT,&sa,NULL);
    sigaction (SIGINT,&sa,NULL);
    sigaction (SIGTERM,&sa,NULL);
    
    openlog("OpenDCS",LOG_NDELAY,LOG_DAEMON);
    if(daemonize("OpenDCS")) {
        xerror("Unable to go to the background");
    }

    setverbosity(10);
    
    temp=msg_setup_queue();    /* This creates and sets up the message queue */
    xlog(10,"msg_setup_queue() returned %d",temp);
    initialize_tagbase(); /* initiallize the tagname list and database */

    xlog(0,"OpenDCS started");
    
    temp=add_module("lsmod","/bin/ls","-l",MFLAG_OPENPIPES);
    temp=add_module("test","/home/phil/opendcs/modules/test/test",NULL,0);
    temp=add_module("testmod2","/home/phil/opendcs/test",NULL,0);
    temp=add_module("testmod3","/home/phil/opendcs/test",NULL,0);
    temp=add_module("testmod4","/home/phil/opendcs/test",NULL,0);
    temp=add_module("testmod5","/home/phil/opendcs/test",NULL,0);
    
    n=0;
    //~ strcpy(list[n].name,"Bool1"); list[n].type=DCS_BOOL; list[n++].count = 17;
    //~ strcpy(list[n].name,"Byte2"); list[n].type=DCS_BYTE; list[n++].count = 1;
    //~ strcpy(list[n].name,"Bool3"); list[n].type=DCS_BOOL; list[n++].count = 1;
    //~ strcpy(list[n].name,"Bool4"); list[n].type=DCS_BOOL; list[n++].count = 1;
    //~ strcpy(list[n].name,"Dword5"); list[n].type=DCS_DWORD; list[n++].count = 1;
    //~ strcpy(list[n].name,"Bool6"); list[n].type=DCS_BOOL; list[n++].count = 1;
    //~ strcpy(list[n].name,"Bool7"); list[n].type=DCS_BOOL; list[n++].count = 1;
    //~ strcpy(list[n].name,"Bool8"); list[n].type=DCS_BOOL; list[n++].count = 10;
    //~ strcpy(list[n].name,"Bool9"); list[n].type=DCS_BOOL; list[n++].count = 1;
    //~ strcpy(list[n].name,"Bool10"); list[n].type=DCS_BOOL; list[n++].count = 1;
    //~ strcpy(list[n].name,"Bool11"); list[n].type=DCS_BOOL; list[n++].count = 1;
    //~ strcpy(list[n].name,"Word12"); list[n].type=DCS_WORD; list[n++].count = 50;
    //~ strcpy(list[n].name,"LReal13"); list[n].type=DCS_LREAL; list[n++].count = 1;
    //~ strcpy(list[n].name,"Dword14"); list[n].type=DCS_REAL; list[n++].count = 1;
    //~ strcpy(list[n].name,"Dword15"); list[n].type=DCS_REAL; list[n++].count = 1;
    //~ strcpy(list[n].name,"Bool16"); list[n].type=DCS_BOOL; list[n++].count = 1;
    
    //~ for(n=0;n<16;n++) {
        //~ tag_add(list[n].name,list[n].type,list[n].count);
    //~ }
    //~ tags_list();
    
    //~ buff[0]=0x55;
    //~ tag_write_bytes(tag_get_handle("Byte2"),buff,1);
    //~ *((u_int16_t *)buff)=22222;
    //~ handle=tag_get_handle("Word12");
    //~ tag_write_bytes(handle,buff,2);
    //~ print_database();
    
    //~ tag_read_bytes(handle,&temp,2);
    //~ printf("Retrieved %d from handle %ld\n",temp,handle);
    
    /* This is just a dumb routine to show the database */
    //~ printf("         0               1\n");
    //~ printf("         0123456789ABCDEF0123456789ABCDEF\n");
    //~ for(i=0;i<120;i++) {
        //~ printf("0x%4x : ",i*32);
        //~ for(j=0;j<32;j++) {
            //~ if((n=tag_get_type(i*32 + j))>=0) {
                //~ printf("%d", n%10);
            //~ } else {
                //~ printf("_");
            //~ }
        //~ }
        //~ printf("\n");
    //~ }
    
    //start_module(2);
    start_module(3);
    //start_module(4);
    //start_module(5);
    
    while(1) {
        msg_receive();
        scan_modules();
        sleep(1);
    }
}

/* This handles the shutting down of a module */
void child_signal(int sig) {
    int status;
    pid_t pid;

    pid=wait(&status);
    xlog(1,"Caught Child Dying %d\n",pid);
    dead_module_add(pid,status);
}

/* this handles shutting down of the core */
void quit_signal(int sig) {
    xlog(0,"Quitting due to signal %d",sig);
    /* TODO: Should stop all running modules */
    kill(0,SIGTERM); /* ...this'll do for now */
    msg_destroy_queue(); /* Destroy the message queue */
    exit(-1);
}
