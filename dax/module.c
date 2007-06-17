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

static dcs_module *_current_mod=NULL;
static int _module_count=0;

static char **_arglist_tok(char *,char *);
static dcs_module *_get_module(mod_handle_t);
static dcs_module *_get_module_pid(pid_t);
static dcs_module *_get_module_name(char *);
static int _cleanup_module(pid_t,int);

/* This array is the dead module list.
   TODO: This should be improved to allow it to grow when needed.
*/
#ifndef DMQ_SIZE
  #define DMQ_SIZE 10
#endif
static dead_module _dmq[DMQ_SIZE];

/* The module list is implemented as a circular double linked list.
   There is no ordering of the list.  Handles are of type int and will
   simply be incremented for each add.  Handles will not be reused for
   modules that are deleted, the handle can overflow.  The current pointer
   will not be moved as this will make the list more efficient if 
   successive queries are made.
   
   Returns the new handle on success and 0 on failure.  (1 is reserved
   for the opendcs main process.)  If an existing module with the same
   name is found the handle of that module will be returned instead.
*/
mod_handle_t 
module_add(char *name, char *path, char *arglist, unsigned int flags) {
    /* The handle is incremented before first use.  The first added module
       will be handle 2 so that the core process can always be one */
    static mod_handle_t handle=1;

    dcs_module *new, *try;
    xlog(10,"Adding module %s",name);
    
    if((try=_get_module_name(name))) return try->handle; /* Name already used */
    /* TODO: Handle the errors for the following somehow */
    new=(dcs_module *)xmalloc(sizeof(dcs_module));
    if(new) {
        new->handle = ++handle;
        new->flags = flags;
        
        /* Add the module path to the struct */
        if(path) {
            new->path=strdup(path);
            
            /* tokenize and set arglist */
            new->arglist=_arglist_tok(path,arglist);
        }
        /* name the module */
        new->name=strdup(name);
        if(_current_mod==NULL) { /* List is empty */
            new->next=new;
            new->prev=new;
        } else {
            new->next = _current_mod->next;
            new->prev = _current_mod;
            _current_mod->next->prev = new;
            _current_mod->next = new;
        }
        _current_mod = new;
        _module_count++;
        return handle;
    } else {
        return 0;
    }
}

/* Convert a string like "-d -x -y" to a NULL terminated array of
   strings suitable as an arg list for the arg_list of an exec??()
   function. */
static char **_arglist_tok(char *path,char *str) {
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
int module_del(mod_handle_t handle) {
    dcs_module *mod;
    char **node;

    mod=_get_module(handle);
    if(mod) {
        if(mod->next==mod) { /* Last module */
            _current_mod=NULL;
        } else {
            /* disconnect module */
            (mod->next)->prev = mod->prev;
            (mod->prev)->next = mod->next;
            /* don't want current to be pointing to the one we are deleting */
            if(_current_mod==mod) {
                _current_mod=mod->next;
            }
        }
        _module_count--;
        /* free allocated memory */
        if(mod->path) free(mod->path);
        if(mod->name) free(mod->name);
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

/* These are only used for the module_start() to handle the pipes.
   it was getting a little messy */
inline static int _getpipes(int *);
inline static int _childpipes(int *);

/* This function is used to start a module */
pid_t module_start(mod_handle_t handle) {
    pid_t child_pid;
    dcs_module *mod;
    int result=0;
    int pipes[6];
    mod=_get_module(handle);
    if(mod) {
        /* Check the Open Pipes flag of the module */
        if(mod->flags & MFLAG_OPENPIPES) {
            /* from here on out if result is TRUE then deal with the pipes */
            result=_getpipes(pipes);
        }

        child_pid = fork();
        if(child_pid>0) { /* This is the parent */
            mod->pid = child_pid;
            xlog(1,"Starting Module - %s - %d",mod->path,child_pid);
            mod->starttime=time(NULL);

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
            if(result) _childpipes(pipes);
            mod->state=MSTATE_RUNNING | MSTATE_CHILD;
            mod->exit_status=0;
            /* TODO: Environment???? */
            /* TODO: Change the UID of the process */
            if(execvp(mod->path,mod->arglist)) {
                xerror("start_module exec failed - %s - %s",
                       mod->path,strerror(errno));

                exit(errno);
            }
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
inline static int _getpipes(int *pipes) {

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
inline static int _childpipes(int *pipes) {
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


/* TODO: Program stop_module */
int module_stop(mod_handle_t handle) {
    return 0;
}
/* The dax core will not send messages to modules that are not register.  Also
   modules that are not started by the core need a way to announce themselves.
   name can be passed as NULL for modules that were started from DAX */
void module_register(char *name ,pid_t pid) {
    dcs_module *mod;
    mod_handle_t handle;
    
    /* First see if we already have a module of the given PID
       This should happen if DAX started the module */
    mod=_get_module_pid(pid);
    if(mod) {
        if(name) {
            if(strcmp(name,mod->name)) { /* New name given */
                xlog(6,"Changing the name of module %s to %s",mod->name,name);
                free(mod->name);
                mod->name=xstrdup(name);
            }
        }
    } else {
        /* Does the module exist in in the handle list */
        mod=_get_module_name(name);
        if(!mod) {
            handle=module_add(name,NULL,NULL,0);
            mod=_get_module(handle);
        }
    }
    if(mod) {
        mod->pid = pid;
        mod->state |= MSTATE_RUNNING;
        mod->state |= MSTATE_REGISTERED;
    }
}

void module_unregister(pid_t pid) {
    dcs_module *mod;
    mod=_get_module_pid(pid);
    if(mod) {
        //mod->pid=0;
        mod->state &= (~MSTATE_REGISTERED);
    }
}


mod_handle_t module_get_pid(pid_t pid) {
    dcs_module *mod;
    mod=_get_module_pid(pid);
    if(mod)
        return mod->handle;
}


/* This function scans the modules to see if there are any that need
   to be cleaned up or restarted.  This function should never be called
   from the start_module process or the child signal handler to avoid a
   race condition. */
void module_scan(void) {
    int n;
    /* Check the dead module queue for pid's that need cleaning */
    for(n=0;n<DMQ_SIZE;n++) {
        if(_dmq[n].pid!=0) {
            if(_cleanup_module(_dmq[n].pid,_dmq[n].status)) {
                _dmq[n].pid=0;
            }
        }
    }
    /* TODO: Restart modules if necessary */
}

/* This function is called from the scan_modules function and is used 
   to find and cleanup the module after it has died.  */
static int _cleanup_module(pid_t pid,int status) {
    dcs_module *mod;
    mod=_get_module_pid(pid);
    /* at this point _current_mod should be pointing to a module with
    the PID that we passed but we should check because there may not 
    be a module with our PID */
    if(mod) {
        xlog(2,"Cleaning up Module %d\n",pid);
        /* Close the stdio pipe fd's */
        close(mod->pipe_in);
        close(mod->pipe_out);
        close(mod->pipe_err);
        mod->pid=0;
        mod->exit_status=status;
        mod->state=MSTATE_WAITING;
        return 1;
    } else { 
        xerror("Module %d not found \n",pid);
        return 0;
    }
}


/* Adds the dead module to the first blank spot in the list.  If the list
   is overflowed then it'll just overwrite the last one.
   TODO: If more than DMQ_SIZE modules dies all at once this will cause
         problems.  Should be fixed. */
void module_dmq_add(pid_t pid,int status) {
    int n=0;
    while(_dmq[n].pid!=0 && n<DMQ_SIZE) {
        n++;
    }
    xlog(10,"Adding Dead Module pid=%d index=%d",pid,n);
    _dmq[n].pid=pid;
    _dmq[n].status=status;
}



/* static function to get a module pointer from a handle */
static dcs_module *_get_module(mod_handle_t handle) {
    dcs_module *last;
    /* In case we ain't go no list */
    if(_current_mod==NULL) return NULL;
    /* Figure out where we need to stop */
    last=_current_mod->prev;
    if(last->handle==handle) return last;

    while(_current_mod->handle != handle) {
        _current_mod = _current_mod->next;
        if(_current_mod==last) {
            return NULL;
        }
    }
    return _current_mod;
}

/* Lookup and return the pointer to the module with pid */
static dcs_module *_get_module_pid(pid_t pid) {
    dcs_module *last;
    /* In case we ain't go no list */
    if(_current_mod==NULL) return NULL;
    /* Figure out where we need to stop */
    last=_current_mod->prev;
    if(last->pid==pid) return last;

    while(_current_mod->pid != pid) {
        _current_mod = _current_mod->next;
        if(_current_mod==last) {
            return NULL;
        }
    }
    return _current_mod;
}

/* Retrieves a pointer to module given name */
static dcs_module *_get_module_name(char *name) {
    dcs_module *last;
    /* In case we ain't go no list */
    if(_current_mod==NULL) return NULL;
    /* Figure out where we need to stop */
    last=_current_mod->prev;
    if(!strcmp(_current_mod->name,name)) return last;

    while(strcmp(_current_mod->name,name)) {
        _current_mod = _current_mod->next;
        if(_current_mod==last) {
            return NULL;
        }
    }
    return _current_mod;
}
