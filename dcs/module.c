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
#include <time.h>
#include <string.h>
#include <opendcs.h>
#include "module.h"
#include "func.h"

/* TODO:
    Function to scan modules to determine which need restarting
    and restart them.
    
    Function to shut down all modules.  Trying a SIGTERM first and
    then SIGKILL if they stick around.
*/

static dcs_module *module_current=NULL;
static int module_count=0;

static char **arglist_tok(char *,char *);
dcs_module *get_module(unsigned int);


/* The module list is implemented as a circular double linked list.
   There is no ordering of the list.  Handles are of type int and will
   simply be incremented for each add.  Handles will not be reused for
   modules that are deleted, the handle can overflow.  The current pointer
   will not be moved as this will make the list more efficient if 
   successive queries are made.
   
   Returns the new handle on success and 0 on failure.  (zero is reserved
   for the opendcs main process.)
*/
unsigned int add_module(char *path, char *arglist, unsigned int flags) {
    static unsigned int handle=0;
    dcs_module *new;

    if(!path) return 0; /* No path / No Module */
    new=(dcs_module *)xmalloc(sizeof(dcs_module));
    if(new) {
        new->handle = ++handle;
        new->flags = flags;
        
        /* Add the module path to the struct */
        new->path=strdup(path);
        
        /* tokenize and set arglist */
        new->arglist=arglist_tok(path,arglist);
        
        if(module_current==NULL) { /* List is empty */
            new->next=new;
            new->prev=new;
        } else {
            new->next = module_current->next;
            new->prev = module_current;
            module_current->next->prev = new;
            module_current->next = new;
        }
        module_current = new;
        module_count++;
        return handle;
    } else {
        return 0;
    }
}

/* Convert a string like "-d -x -y" to a NULL terminated array of
   strings suitable as an arg list for the arg_list of an exec??()
   function. */
static char **arglist_tok(char *path,char *str) {
    char *temp,*token,*save;
    char **arr;
    int count=1;
    int index=1;
    save=temp=NULL;
    
    if(str!=NULL) {
        /* We need a non const char string for strtok */
        temp=strdup(str);
    }
    /* First we count the number of tokens */
    if(temp) {
        token=strtok_r(temp," ",&save);
        while(token) {
            count++;
            token=strtok_r(NULL," ",&save);
        }
    }
    /* Allocate the array, 1 extra for the NULL */
    if(path || temp) {
        arr=(char**)xmalloc((count+1)*sizeof(char*));
        if(arr==NULL) return NULL; /* OOOPS No Memory left */
    } else { /* No path supplied either */
        return NULL;
    }
    
    arr[0]=strdup(path); /* Put the path into the first argument */
    
    /* Now we re-parse the string to add the tokens to the array
       First we have to copy str to temp again.  No sense in using
       strdup() this time since the string is the same and the memory
       has already been allocated */
    if(temp) {
        strcpy(temp,str);
        token=strtok_r(temp," ",&save);
        while(token) {
            /* allocate and get a copy of the token and save it into the array */
            arr[index++]=strdup(token); /* TODO: ERROR CHECK THIS */
            token=strtok_r(NULL," ",&save);
        }
        arr[index]=NULL;
        /* dont forget this */
        free(temp);
    }
    return arr;
}


/* uses get_module to locate the module identified as 'handle' and removes
   it from the list. */
int del_module(unsigned int handle) {
    dcs_module *mod;
    char **node;

    mod=get_module(handle);
    if(mod) {
        if(mod->next==mod) { /* Last module */
            module_current=NULL;
        } else {
            /* disconnect module */
            (mod->next)->prev = mod->prev;
            (mod->prev)->next = mod->next;
            /* don't want current to be pointing to the one we are deleting */
            if(module_current==mod) {
                module_current=mod->next;
            }
        }
        module_count--;
        /* free allocated memory */
        if(mod->path) free(mod->path);
        if(mod->arglist) {
            node=mod->arglist;
            while(*node) {
                free(*node);
                node++;
            }
            free(mod->arglist);
        }
        free(mod);
        return 0;
    }
    return -1;
}

/* Static function definitions */
unsigned int get_module_count() {
    return module_count;
}

/* These are only used for the star_module() to handle the pipes.
   it was getting a little messy */
inline static int getpipes(int *);
inline static int childpipes(int *);

/* This function is used to start a module */
pid_t start_module(unsigned int handle) {
    pid_t child_pid;
    dcs_module *mod;
    int result=0;
    int pipes[6];
    mod=get_module(handle);
    if(mod) {
        /* Check the Open Pipes flag of the module */
        if(mod->flags & MFLAG_OPENPIPES) {
            /* from here on out if result is TRUE then deal with the pipes */
            result=getpipes(pipes);
        }

        child_pid = fork();
        if(child_pid>0) { /* This is the parent */
            xlog(1,"Starting Module - %s",mod->path);
            mod->starttime=time(NULL);
            mod->pid = child_pid;

            if(result) { /* do we have any pipes set */
                close(pipes[0]); /* close the write pipe on childs stdin */
                close(pipes[3]); /* close the read pipe on childs stdout */
                close(pipes[5]); /* close the read pipe on childs stderr */
            
                /* record the pipes for the module */
                mod->pipe_in  = pipes[1];  /* fd to write childs stdin */
                mod->pipe_out = pipes[2];  /* fd to read childs stdout */
                mod->pipe_err = pipes[4];  /* fd to read childs stderr */
            }
            return child_pid;
        } else if(child_pid==0) { /* Child */
            if(result) childpipes(pipes);
            /* TODO: Environment???? */
            /* TODO: Change the UID of the process */
            execvp(mod->path,mod->arglist);
            xerror("start_module exec failed - %s - %s",mod->path,strerror(errno));
        } else { /* Error on the fork */
            xerror("start_module fork failed - %s - %s",mod->path,strerror(errno));
        }
    } else {
        return 0;
    }
    return 0;
}

/* Gets the three sets of pipes for stdin stdout and stderr 
   pipes[0]=stdin  read fd
   pipes[1]=stdin  write fd
   pipes[2]=stdout read fd
   pipes[3]=stdout write fd
   pipes[4]=stderr read fd
   pipes[5]=stderr write fd
*/
static int getpipes(int *pipes) {

    if(pipe(&pipes[0])) {
        xerror("Unable to create pipe - %s",strerror(errno));
        return 0;
    }
    if(pipe(&pipes[2])) {
        xerror("Unable to create pipe - %s",strerror(errno));
        close(pipes[0]);
        close(pipes[1]);
        return 0;
    }
    if(pipe(&pipes[4])) {
        xerror("Unable to create pipe - %s",strerror(errno));
        close(pipes[0]);
        close(pipes[1]);
        close(pipes[2]);
        close(pipes[3]);
        return 0;
    }
    return 1;
}

/* This function handles the dup()ing and closing of
   the stdin/stdout/stderr file descriptors in the child */
/* TODO: check for errors and return appropriately. */
inline static int childpipes(int *pipes) {
    close(pipes[1]);
    close(pipes[2]);
    close(pipes[4]);
    dup2(pipes[0],0);
    dup2(pipes[3],1);
    dup2(pipes[5],2);
    close(pipes[0]);
    close(pipes[3]);
    close(pipes[5]);
    return 1;
}

/* TODO: If this function is public it should return a handle */
dcs_module *get_next_module() {
    if(module_current) {
        module_current=module_current->next;
    }
    return module_current;
}

/* static function to get a module pointer from a handle */
dcs_module *get_module(unsigned int handle) {
    dcs_module *last;
    /* In case we ain't go no list */
    if(module_current==NULL) return NULL;
    /* Figure out where we need to stop */
    last=module_current->prev;
    if(last->handle==handle) return last;

    while(module_current->handle != handle) {
        module_current = module_current->next;
        if(module_current==last) {
            return NULL;
        }
    }
    return module_current;
}
