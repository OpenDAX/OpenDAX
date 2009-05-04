/*  OpenDAX - An open source data acquisition and control system 
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
#include <module.h>
#include <options.h>
#include <func.h>

#include <time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>

/* TODO: Module Scanning routine
    Function to scan modules to determine which need restarting
    and restart them.
    
    Function to shut down all modules.  Trying a SIGTERM first and
    then SIGKILL if they stick around.
*/

static dax_module *_current_mod = NULL;
static int _module_count = 0;

/* This array is the dead module list.
   TODO: This should be improved to allow it to grow when needed.
*/
#ifndef DMQ_SIZE
  #define DMQ_SIZE 10
#endif

static dead_module _dmq[DMQ_SIZE];

/* The module list is implemented as a circular double linked list.
 * There is no ordering of the list. The current pointer will stay
 * pointing to the last find.  This will make the list more efficient
 * if successive queries are made.
 */

/* Determine the host from the file descriptor of the socket.
 * host is used to return the parameter back to the caller and
 * the return value is there to indicate any error.  Returns
 * zero on success.  *host is set to zero if the socket originated
 * on the same host as the server, whether or not it was with a 
 * TCP socket or a Local Domain socket. */
static int
_get_host(int fd, in_addr_t *host)
{
    int result;
    socklen_t sock_len;
    struct sockaddr_storage addr;
    struct sockaddr_in *addr_in;
    
    sock_len = sizeof(addr);
    result = getpeername(fd, (struct sockaddr *)&addr, &sock_len);
    if(result < 0) {
        xerror("_get_host %s", strerror(errno));
    } else {
        if(addr.ss_family == AF_LOCAL) {
            *host = 0;
            return 0;
        } else if(addr.ss_family == AF_INET) {
            /* Get the modules IP address */
            addr_in = (struct sockaddr_in *)&addr;
            *host = addr_in->sin_addr.s_addr;
            /* Now see if it is the same as ours */
            result = getsockname(fd, (struct sockaddr *)&addr, &sock_len);
            if(result < 0) {
                xerror("_get_host %s", strerror(errno));
            } else {
                addr_in = (struct sockaddr_in *)&addr;
                if(addr_in->sin_addr.s_addr == *host) {
                    *host = 0;
                }
            }
            return 0;
        } else {
            xerror("Unable to identify socket type in module_register()\n");
        }
    }
    return ERR_NOTFOUND;
}

static dax_module *
_get_module_pid(pid_t pid)
{
    int n;
    
    /* In case we ain't got no list */
    if(_current_mod == NULL) return NULL;
    
    for(n = 0; n < _module_count; n++) {
        if(_current_mod->pid == pid) {
            return _current_mod;
        }
        _current_mod = _current_mod->next;
    }
    
    return NULL;
}

static dax_module *
_get_module_hostpid(in_addr_t host, pid_t pid)
{
    dax_module *last;
    
    if(_current_mod == NULL) return NULL;
    last = _current_mod;
    do {
        if(_current_mod->host == host && _current_mod->pid == pid) {
            return _current_mod;
        }
        _current_mod = _current_mod->next;
    } while(_current_mod != last);
    return NULL;
}

/* Return a pointer to the module with a matching file descriptor (fd)
 * returns NULL if not found */
static dax_module *
_get_module_fd(int fd)
{
    dax_module *last;
    
    if(_current_mod == NULL) return NULL;
    last = _current_mod;
    do {
        if(_current_mod->fd == fd) {
            return _current_mod;
        }
        _current_mod = _current_mod->next;
    } while(_current_mod != last);
    return NULL;
}

/* Return a pointer to the module with a matching event file
 * descriptor (efd).  Returns NULL if not found */
static dax_module *
_get_module_efd(int efd)
{
    dax_module *last;
    
    if(_current_mod == NULL) return NULL;
    last = _current_mod;
    do {
        if(_current_mod->efd == efd) {
            return _current_mod;
        }
        _current_mod = _current_mod->next;
    } while(_current_mod != last);
    return NULL;
}

/* returns a module with the given name, NULL if not found */
static dax_module *
_get_module_name(char *name)
{
    int n;
    
    /* In case we ain't go no list */
    if(_current_mod == NULL) return NULL;
    
    for(n = 0; n < _module_count; n++) {
        if(!strcmp(_current_mod->name, name)) return _current_mod;
        _current_mod = _current_mod->next;
    }
    
    return NULL;
}

/* Convert a string like "-d -x -y" to a NULL terminated array of
 * strings suitable as an arg list for the arg_list of an exec??()
 * function. */
static char **
_arglist_tok(char *path, char *str)
{
    char *temp, *token, *save;
    char **arr;
    int count = 1;
    int index = 1;
    save=temp=NULL;
    
    if(str!=NULL) {
        /* We need a non const char string for strtok */
        temp = strdup(str);
    }
    /* First we count the number of tokens */
    if(temp) {
        token = strtok_r(temp," ",&save);
        while(token) {
            count++;
            token = strtok_r(NULL," ",&save);
        }
    }
    /* Allocate the array, 1 extra for the NULL */
    if(path || temp) {
        arr = xmalloc((count + 1) * sizeof(char*));
        if(arr == NULL) return NULL; /* OOOPS No Memory left */
    } else { /* No path supplied either */
        return NULL;
    }
    
    arr[0] = strdup(path); /* Put the path into the first argument */
    
    /* Now we re-parse the string to add the tokens to the array
       First we have to copy str to temp again.  No sense in using
       strdup() this time since the string is the same and the memory
       has already been allocated */
    if(temp) {
        strcpy(temp, str);
        token = strtok_r(temp, " ", &save);
        while(token) {
            /* allocate and get a copy of the token and save it into the array */
            arr[index++] = strdup(token); /* TODO: ERROR CHECK THIS */
            token = strtok_r(NULL, " ", &save);
        }
        arr[index] = NULL;
        
        free(temp);
    }
    return arr;
}

/* Lookup and return the pointer to the module with pid */
/* Retrieves a pointer to module given name */
//#ifdef DEBUG
static void
_print_modules(void)
{
    dax_module *last;
  
    /* In case we ain't got no list */
    if(_current_mod == NULL) return;
    /* Figure out where we need to stop */
    last = _current_mod;
    
    do {
        printf("Module - %s - %u\n", _current_mod->name, _current_mod->pid);
        _current_mod = _current_mod->next;
    } while (_current_mod != last);
}
//#endif


dax_module *
module_add(char *name, char *path, char *arglist, int startup, unsigned int flags)
{
    dax_module *new, *try;
    xlog(LOG_MAJOR,"Adding module %s",name);
    
    if((try = _get_module_name(name))) 
        return try;
    
    new = xmalloc(sizeof(dax_module));
    if(new) {
        new->flags = flags;
        if(startup > 0) new->startup = startup;
        else startup = 0;
        
        new->pipe_in = -1;
        new->pipe_out = -1;
        new->pipe_err = -1;
        new->fd = 0;
        new->efd = 0;
        new->pid = 0;
        
        /* Add the module path to the struct */
        if(path) {
            new->path = strdup(path);
            
            /* tokenize and set arglist */
            new->arglist = _arglist_tok(path, arglist);
        }
        /* name the module */
        new->name = strdup(name);
        if(_current_mod == NULL) { /* List is empty */
            new->next = new;
            new->prev = new;
        } else {
            new->next = _current_mod->next;
            new->prev = _current_mod;
            _current_mod->next->prev = new;
            _current_mod->next = new;
        }
        _current_mod = new;
        _module_count++;
        return new;
    } else {
        return NULL;
    }
}

/* Deletes the module from the list and frees the memory */
int
module_del(dax_module *mod)
{
    char **node;

    if(mod) {
        if(mod->next == mod) { /* Last module */
            _current_mod = NULL;
        } else {
            /* disconnect module */
            (mod->next)->prev = mod->prev;
            (mod->prev)->next = mod->next;
            /* don't want current to be pointing to the one we are deleting */
            if(_current_mod == mod) {
                _current_mod = mod->next;
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
    return ERR_ARG;
}

void
module_start_all(void)
{
    dax_module *last;
    int i, j, x;
    /* In case we ain't go no list */
    if(_current_mod == NULL) return;
    /* Figure out where we need to stop */
    last = _current_mod->prev;
    
    x = opt_maxstartup();
    
    for(i = 1; i <= x; i++) {
        for(j = 0; j < _module_count; j++) {
            if(_current_mod->startup == i) {
                module_start(_current_mod);
            }
            _current_mod = _current_mod->next;
        }
        /* TODO: Wait until all the modules of this level have registered. Need to use
         a global variable to say that we are in the initial startup routine.  Then use
         a condition variable to block (with timeout) until the module_register function 
         runs.  The module register function would check the global variable and if it is
         set then aquire our mutex, register the module then signal our condition variable
         when all of the modules of that level have been registered then we can move to the
         next level. Some modules are unregisterable, perhaps another flag is in order. */
    }
}

/* These are only used for the module_start() to handle the pipes.
   it was getting a little messy */
inline static int _getpipes(int *);
inline static int _childpipes(int *);

/* This function is used to start a module */
pid_t
module_start(dax_module *mod)
{
    pid_t child_pid;
    int result = 0;
    int pipes[6];
 
    if(mod) {
        /* Check the Open Pipes flag of the module */
        if(mod->flags & MFLAG_OPENPIPES) {
            /* from here on out if result is TRUE then deal with the pipes */
            result = _getpipes(pipes);
        }

        child_pid = fork();
        if(child_pid > 0) { /* This is the parent */
            mod->pid = child_pid;
            xlog(1, "Starting Module - %s - %d",mod->path,child_pid);
            mod->starttime = time(NULL);

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
        } else if(child_pid == 0) { /* Child */
            if(result) _childpipes(pipes);
            mod->state = MSTATE_RUNNING | MSTATE_CHILD;
            mod->exit_status = 0;
            /* TODO: Environment???? */
            /* TODO: Change the UID of the process */
            if(execvp(mod->path, mod->arglist)) {
                xerror("start_module exec failed - %s - %s",
                       mod->path, strerror(errno));

                exit(errno);
            }
        } else { /* Error on the fork */
            xerror("start_module fork failed - %s - %s", mod->path, strerror(errno));
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
inline static int 
_getpipes(int *pipes)
{

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
 * the stdin/stdout/stderr file descriptors in the child */
/* TODO: check for errors and return appropriately. */
inline static int
_childpipes(int *pipes)
{
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
int
module_stop(dax_module *mod)
{
    return 0;
}

/* TODO: GOTTA RETHINK MODULE REGISTRATION!! THIS DON'T WORK!!*/
/* The dax server will not send messages to modules that are not registered.
 * Also modules that are not started by the core need a way to announce
 * themselves. name can be NULL for modules that were started from DAX */
dax_module *
module_register(char *name, pid_t pid, int fd)
{
    dax_module *mod, *test;
    //char *newname;
    //size_t size;
    int result;
    in_addr_t host;
    
    /* A module with the given file descriptor already exists */
    if(_get_module_fd(fd)) {
        return NULL;
    }
    
    result = _get_host(fd, &host);
    if(result) return NULL;
    
    /* First see if we already have a module of the given PID & host
     * This should happen if DAX started the module */
    mod = _get_module_hostpid(host, pid);
    
    /* Check to see if this fd is in use */
    test = _get_module_fd(fd);
    if(test && test != mod) {
        printf("...WHOA THAT AIN'T RIGHT! WHAT HAPPENED TO %d?\n", fd);
        module_unregister(test->fd);
        test->fd = 0;
        test->efd = 0;
    }
    
    /* If the module doesn't already exist */
    if(mod == NULL) {
        /* Do we already have a module of this name */
        //--mod = _get_module_name(name);
        //--if(mod && mod->state & MSTATE_REGISTERED) {
            /* We already have one of these running */
            /* Get a new string big enough for adding the PID */
            //--size = strlen(name) + 7;
            //--newname = (char *)malloc(size);
            //--snprintf(newname, size, "%s%d", name, pid);
            mod = module_add(name, NULL, NULL, 0, 0);
        //} else  {
            /* Never seen this guy before so we'll add it */
        //    mod = module_add(name, NULL, NULL, 0, 0);
        //}
    }
    if(mod) {
        mod->pid = pid;
        mod->fd = fd;
        mod->state |= MSTATE_RUNNING;
        mod->state |= MSTATE_REGISTERED;
    } else {
        xerror("Major problem registering module - %s : %d", name, pid);
        return NULL;
    }
    _print_modules();
    return mod;
}

/* This finds the module given by pid and sets the modules
 * event notification socket file descriptor in the module
 * list. */
dax_module *
event_register(pid_t pid, int fd)
{
    dax_module *mod;
    int result;
    in_addr_t host;
    
    /* A module with the given file descriptor already exists */
    if(_get_module_efd(fd)) {
        return NULL;
    }
    
    result = _get_host(fd, &host);
    if(result) return NULL;
    
    mod = _get_module_hostpid(host, pid);
    
    /* TODO: We need to check that the fd doesn't already exist or we'll
       have some communication trouble */
    if(!mod) {
        return NULL;
    } else {
        mod->efd = fd;
    }
    _print_modules();
    return mod;
}


void
module_unregister(int fd)
{
    dax_module *mod;
    
    mod = module_find_fd(fd);
    if(mod) {
        mod->state &= (~MSTATE_REGISTERED);
        mod->fd = 0;
        mod->efd = 0;
        /* If we didn't start it then delete it */
        if( !(mod->state & MSTATE_CHILD) ) {
            module_del(mod);
        }
    }
    _print_modules();
}


dax_module *
module_find_fd(int fd)
{
    int n;
    
    /* In case we ain't go no list */
    if(_current_mod == NULL) return NULL;
    
    for(n = 0; n < _module_count; n++) {
        if(_current_mod->fd == fd) return _current_mod;
        _current_mod = _current_mod->next;
    }
    
    return NULL;
}

/* This function is called from the scan_modules function and is used 
 * to find and cleanup the module after it has died.  */
static int
_cleanup_module(pid_t pid, int status)
{
    dax_module *mod;
    
    /* Should only be called for local modules so we can assume the
     * upper 32 bits are zero and just use the pid for the mid */
    mod = _get_module_pid(pid);
    /* at this point _current_mod should be pointing to a module with
     * the PID that we passed but we should check because there may not 
     * be a module with our PID */
    if(mod) {
        xlog(LOG_MINOR, "Cleaning up Module %d", pid);
        /* Close the stdio pipe fd's */
        /* TODO: really should fix these */
        //close(mod->pipe_in);
        //close(mod->pipe_out);
        //close(mod->pipe_err);
        mod->pid = 0;
        mod->exit_status = status;
        mod->state = MSTATE_WAITING;
        return 0;
    } else {
        xerror("Module %d not found \n", pid);
        return ERR_NOTFOUND;
    }
}


/* This function scans the modules to see if there are any that need
   to be cleaned up or restarted.  This function should never be called
   from the start_module process or the child signal handler to avoid a
   race condition. */
void
module_scan(void)
{
    int n;
    /* Check the dead module queue for pid's that need cleaning */
    for(n = 0; n < DMQ_SIZE; n++) {
        if(_dmq[n].pid != 0) {
            _cleanup_module(_dmq[n].pid, _dmq[n].status);
            _dmq[n].pid = 0;
        }
    }
    /* TODO: do a waitpid(-1, NULL, WNOHANG); to clean up any zombies */
    /* TODO: Restart modules if necessary */
}

/* Adds the dead module to the first blank spot in the list.  If the list
   is overflowed then it'll just overwrite the last one.
   TODO: If more than DMQ_SIZE modules dies all at once this will cause
         problems.  Should be fixed. */
void
module_dmq_add(pid_t pid,int status)
{
    int n = 0;
    while(_dmq[n].pid != 0 && n < DMQ_SIZE) {
        n++;
    }
    //xlog(10,"Adding Dead Module pid=%d index=%d",pid,n);
    _dmq[n].pid = pid;
    _dmq[n].status = status;
}



