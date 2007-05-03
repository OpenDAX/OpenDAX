/*  opendcs - An open source distributed control system 
 *  Copyright (c) 1997 Phil Birkelbach
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
#include "module.h"
#include "options.h"
#include "func.h"

void child_signal(int);

int main(int argc, const char * argv[]) {
    struct sigaction sa;
    dcs_module *mod;
    int temp,n;
    long int rnum;
    char buff[256];
    
    memset (&sa,0,sizeof(struct sigaction));
    sa.sa_handler=&child_signal;
    sigaction (SIGCHLD,&sa,NULL);
    xlog(0,"OpenDCS started");
    
    temp=add_module("/bin/ls","-l",MFLAG_OPENPIPES);
    
    for(n=0;n<20;n++) {
        temp=add_module("/home/phil/opendcs/test",NULL,0);
        printf("Added Module Handle %d\n",temp);
    }
    setverbosity(10);
    start_module(1);
    start_module(2);
    start_module(3);
    start_module(4);
    while(1) {
        mod=get_module(1);
        if((temp=read(mod->pipe_out,buff,200)) > 0) {
            buff[temp]=0;
            printf("DUDE: %s\n",buff);
        }
        else {
            printf("Sleeping \n");
            sleep(1);
        }
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
  
  switch(sig) {
    case SIGCHLD:
      pid=wait(&status);
      /* No IO in a signal handler!!!! */
      printf("Module %d has died\n",pid);
      
      /* Write the status into the module stuct for the pid */
      break;
  }
}
