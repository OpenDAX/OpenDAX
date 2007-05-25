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
#include <module.h>
#include <options.h>
#include <func.h>
#include <message.h>
#include <tagbase.h>

void child_signal(int);
void quit_signal(int);

int main(int argc, const char *argv[]) {
    struct sigaction sa;
    dcs_module *mod;
    int temp,n;
    long int handle;
    char buff[256];
    
    
    /* Set up the signal handlers */
    memset (&sa,0,sizeof(struct sigaction));
    sa.sa_handler=&child_signal;
    sigaction (SIGCHLD,&sa,NULL);
    sa.sa_handler=&quit_signal;
    sigaction (SIGQUIT,&sa,NULL);
    sigaction (SIGINT,&sa,NULL);
    
    setverbosity(10);
    
    msg_create_queue(); /* This creates the message queue */
    initialize_tagbase(); /* initiallize the tagname list and database */

    xlog(0,"OpenDCS started");
    
    temp=add_module("lsmod","/bin/ls","-l",MFLAG_OPENPIPES);
    temp=add_module("test","/home/phil/trunk/modules/test/test",NULL,0);
    temp=add_module("testmod2","/home/phil/opendcs/test",NULL,0);
    temp=add_module("testmod3","/home/phil/opendcs/test",NULL,0);
    temp=add_module("testmod4","/home/phil/opendcs/test",NULL,0);
    temp=add_module("testmod5","/home/phil/opendcs/test",NULL,0);
    
    handle=tag_add("Tag1",DCS_BOOL,1);
    printf("Tag1 = %ld\n",handle);
    handle=tag_add("Tag2",DCS_BOOL,4);
    printf("Tag2 = %ld\n",handle);
    handle=tag_add("Tag3",DCS_BOOL,2);
    printf("Tag3 = %ld\n",handle);
    
    
    //start_module(2);
    start_module(3);
    //start_module(4);
    //start_module(5);
    
    while(1) {
        //mod=get_module(2); /* thou shalt not follow the NULL pointer */
        //if((temp=read(mod->pipe_out,buff,200)) > 0) {
        //    buff[temp]=0;
        //    printf("DUDE: %s\n",buff);
        //}
        //else {
            printf("Scaning Modules \n");
            msg_receive();
            scan_modules();
            sleep(1);
        //}
    }
    
#ifdef ASDFLKJHALSKDJHFLAKJSHDFLKJHA
    while(1) {
        //usleep(10000);
        mod=get_next_module();
        
        rnum=random();
        
        if(get_module_count()==2000) {
            n=add_module("/sup/diddly",NULL,0);
            printf("DELETING ALL MODULES - %u M\n",n/1000000);
            while(mod=get_next_module()) {
                //printf("Deleting Module %d - %d\n",mod->handle,get_module_count());
                del_module(mod->handle);
            }
            sleep(1);
        }
        rnum=random();
        if(!(rnum%3)) {
            n=add_module("/home/phil/opendcs/test","a b d what --hello",0);
            //printf("Added Module Handle %d\n",n);
        }
        rnum=random();
        if(!(rnum%3)) {
            mod=get_next_module();
            if(mod)
                n=del_module(mod->handle);
        }
        //DO SOME CRAP.......
        //Fix modules
    }
    
#endif

}

/* This handles the shutting down of a module */
void child_signal(int sig) {
    int status;
    pid_t pid;

    pid=wait(&status);
    xlog(1,"Caught Child Dying %d\n",pid);
    dead_module_add(pid,status);
}

void quit_signal(int sig) {
    xlog(0,"Quitting due to signal %d",sig);
    /* TODO: Should stop all running modules */
    msg_destroy_queue(); /* Destroy the message queue */
    exit(-1);
}
