/*  OpenDAX - An open source data acquisition and control system 
 *  Copyright (c) 2012 Phil Birkelbach
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

/* Source file for the process handling functions for the master process
 * handling program. */

#include <process.h>
#include <sys/time.h>
#include <logger.h>
#include <sys/select.h>

/* Global Variables */
static pthread_t proc_thread;
static pthread_mutex_t proc_mutex;
/* These are used for coordination between the process monitoring thread
 * and the startup routines.*/
static pthread_mutex_t pipe_mutex;
static pthread_cond_t  pipe_cond;

/* This array is the dead process list.
   TODO: This should be improved to allow it to grow when needed.
*/
#ifndef DPQ_SIZE
  #define DPQ_SIZE 10
#endif
static dead_process _dpq[DPQ_SIZE];

static dax_process *_process_list = NULL;


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
        arr = malloc((count + 1) * sizeof(char*));
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

dax_process *
process_add(char *name, char *path, char *arglist, unsigned int flags)
{
    dax_process *new, *this;
    xlog(LOG_MAJOR,"Adding process %s",name);
    
    new = malloc(sizeof(dax_process));
    if(new) {
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
        new->next = NULL;
        pthread_mutex_lock(&proc_mutex);
        if(_process_list == NULL) { /* List is empty */
            _process_list = new;
        } else {
            this = _process_list;
            while(this->next != NULL);
            this->next = new;
        }
        pthread_mutex_unlock(&proc_mutex);
        return new;
    } else {
        return NULL;
    }
}

static void
_free_process(dax_process *proc)
{
    char **node;

    /* free allocated memory */
    if(proc->path) free(proc->path);
    if(proc->name) free(proc->name);
    if(proc->arglist) {
        node=proc->arglist;
        while(*node) {
            free(*node);
            node++;
        }
        free(proc->arglist);
    }
    free(proc);
}

/* Deletes the module from the list and frees the memory */
int
process_del(dax_process *proc)
{
    dax_process *last, *this;

    if(proc) {
        pthread_mutex_lock(&proc_mutex);
        if(proc == _process_list) { /* If this is the first process */
            _process_list = _process_list->next;
        } else {
            last = proc;
            while(this != NULL) {
                this = proc->next;
                if(this == proc) {
                    _free_process(proc);
                    pthread_mutex_unlock(&proc_mutex);
                    return 0;
                }
                last = this;
                this = this->next;
            }
        }
        pthread_mutex_unlock(&proc_mutex);
    }
    return ERR_ARG;
}

#define PIPE_BUFF_MAX 1024

/* This function watches the file descriptors for each child process
 * and sends the output to the system logger.  It will also signal the
 * process starting routines when the startup string has been detected. */
static void
_process_monitor_thread(void)
{
    dax_process *this;
    fd_set fds;
    int max_fd, result;
    struct timeval tv;
    char rbuff[PIPE_BUFF_MAX];

    while(1) {
        //fprintf(stderr,"Thread Running\n");
        FD_ZERO(&fds);
        max_fd = 0;
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        pthread_mutex_lock(&proc_mutex);
        this = _process_list;
        while(this != NULL) {
            if(this->state &= PSTATE_STARTED) {
                if(this->pipe_out > 0) FD_SET(this->pipe_out, &fds);
                if(this->pipe_out > max_fd) max_fd = this->pipe_out;
                if(this->pipe_err > 0) FD_SET(this->pipe_err, &fds);
                if(this->pipe_err > max_fd) max_fd = this->pipe_err;
            }
            this = this->next;
        }
        pthread_mutex_unlock(&proc_mutex);
        if(max_fd > 0) {
            result = select(max_fd+1, &fds, NULL, NULL, &tv);
            printf("select() returned %d\n", result);
            if(result > 0) {
                printf("lock wait\n");
                pthread_mutex_lock(&proc_mutex);
                printf("passed the lock\n");
                this = _process_list;
                while(this != NULL) {
                    if(FD_ISSET(this->pipe_out, &fds)) {
                        printf("read stdout for module %s\n", this->name);
                        result = read(this->pipe_out, rbuff, PIPE_BUFF_MAX);
                        if(result > 0) {
                            rbuff[result] = '\0';
                            xlog(LOG_ALL, "%s: %s", this->name, rbuff);
                        } else if(result < 0) {
                            xerror("master: error reading stdout for %s", this->name);
                        } else { /* result == 0 */
                            xerror("master: don't know what to do with read() returning 0");
                        }
                    }
                    if(FD_ISSET(this->pipe_err, &fds)) {
                        printf("read stderr for module %s\n", this->name);
                        result = read(this->pipe_err, rbuff, PIPE_BUFF_MAX);
                        if(result > 0) {
                            rbuff[result] = '\0';
                            xerror("%s: %s", this->name, rbuff);
                        } else if(result < 0) {
                            xerror("master: error reading stderr for %s", this->name);
                        } else { /* result == 0 */
                            xerror("master: don't know what to do with read() returning 0");
                        }
                    }
                    this = this->next;
                }
                pthread_mutex_unlock(&proc_mutex);
            }
        }
        sleep(1);
    }

}


/* Initialize the global data and start the monitoring thread */
int
process_init(void)
{
    int result;

    pthread_mutex_init(&proc_mutex, NULL);
    pthread_mutex_init(&pipe_mutex, NULL);
    pthread_cond_init(&pipe_cond, NULL);

    result = pthread_create(&proc_thread, NULL, (void *)&_process_monitor_thread, NULL);
    return result;
}

/* Start all of the processes in the process list */
void
process_start_all(void)
{
    dax_process *this;
    struct timeval start, interval, end;
    int timeout;
    int result, done = 0;

    /* In case we ain't go no list */
    if(_process_list == NULL) return;

    pthread_mutex_lock(&proc_mutex);
    this = _process_list;
    while(this != NULL) {
        process_start(this);
        this = this->next;

//        gettimeofday(&start, NULL);
//        this->starttime = start.tv_sec;
//        if(this->waitstr != NULL) {
//            interval.tv_sec = this->timeout / 1000;
//            interval.tv_usec = (this->timeout % 1000)*1000;
//            timeradd(&start, &interval, &end);
//            timeout = this->timeout;
//            fds[0].fd = this->pipe_in;
//            fds[0].events = POLLRDNORM;
//            // Let's just deal with stdout for now
//            //fds[1].fd = this->pipe_err;
//            //fds[1].events = POLLRDNORM;
//            while(!done) {
//                result = poll(fds, 1, timeout);
//                if(result == 0) {
//                    done = 1; /* Timeout */
//                } else if(result < 0) {
//                    /* Any error besides interruption */
//                    if(errno != EINTR) {
//                        /* TODO: Deal with the rest of the errors */
//                        done = 1;
//                    }
//                } else { /* We've got some data here */
//
//                }
//
//            }
//        }
//        while(!done) {
//            result = read(proc->pipe_in,)
//            result = pthread_cond_timedwait(&_startup_cond, &_startup_mutex, &ts);
//            done = 1; /* Let's assume we are done */
//            if(result != ETIMEDOUT) { /* If we didn't timeout */
//                for(j = 0; j < _module_count; j++) { /* Loop through modules */
//                    /* If the module is in the current startup tier and the running flag is not set then... */
//                    if(_current_mod->startup == tier && !(_current_mod->flags & PSTATE_RUNNING)) {
//                        done = 0; /* ...Lets go again */
//                    }
//                    _current_mod = _current_mod->next;
//                }
//            }
//        }
        pthread_mutex_unlock(&proc_mutex);
    }
//        pthread_mutex_unlock(&_startup_mutex);
//    }
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
        return -1;
    }
    if(pipe(&pipes[2])) {
        xerror("Unable to create pipe - %s",strerror(errno));
        close(pipes[0]);
        close(pipes[1]);
        return -1;
    }
    if(pipe(&pipes[4])) {
        xerror("Unable to create pipe - %s",strerror(errno));
        close(pipes[0]);
        close(pipes[1]);
        close(pipes[2]);
        close(pipes[3]);
        return -1;
    }
    return 0;
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


/* This function is used to start a module */
pid_t
process_start(dax_process *proc)
{
    pid_t child_pid;
    int result = 0;
    int pipes[6];
 
    if(proc) { /* We are the parent */
        result = _getpipes(pipes);
        if(result) xerror("Unable to properly set up pipes for %s\n", proc->name);
        child_pid = fork();
        if(child_pid > 0) { /* This is the parent */
            proc->pid = child_pid;
            proc->exit_status = 0;

            xlog(LOG_VERBOSE, "Starting Process - %s - %d",proc->path,child_pid);
            proc->starttime = time(NULL);

            if(result) { /* do we have any pipes set */
                close(pipes[0]); /* close the write pipe on childs stdin */
                close(pipes[3]); /* close the read pipe on childs stdout */
                close(pipes[5]); /* close the read pipe on childs stderr */
            
                /* record the pipes for the process */
                proc->pipe_in  = pipes[1];  /* fd to write childs stdin */
                proc->pipe_out = pipes[2];  /* fd to read childs stdout */
                proc->pipe_err = pipes[4];  /* fd to read childs stderr */
            }
            proc->state = PSTATE_STARTED;
            return child_pid;
        } else if(child_pid == 0) { /* Child */
            if(result) _childpipes(pipes);
            /* TODO: Environment???? */
            /* TODO: Change the UID of the process */
            if(execvp(proc->path, proc->arglist)) {
                xerror("start_module exec failed - %s - %s",
                       proc->path, strerror(errno));

                exit(errno);
            }
        } else { /* Error on the fork */
            xerror("start_module fork failed - %s - %s", proc->path, strerror(errno));
        }
    } else {
        return 0;
    }
    return 0;
}

static dax_process *
_get_process_pid(pid_t pid)
{
	dax_process *this;

	this = _process_list;
	while(this != NULL) {
		if(this->pid == pid) return this;
		this = this->next;
	}
	return NULL;
}

/* This function is called from the scan_modules function and is used 
 * to find and cleanup the module after it has died.  */
static int
_cleanup_process(pid_t pid, int status)
{
    dax_process *proc;
    
    proc = _get_process_pid(pid);

    /* at this point _current_mod should be pointing to a module with
     * the PID that we passed but we should check because there may not 
     * be a module with our PID */
    if(proc) {
        xlog(LOG_MINOR, "Cleaning up Process %d", pid);
        /* Close the stdio pipe fd's */
        /* TODO: really should fix these */
        close(proc->pipe_in);
        close(proc->pipe_out);
        close(proc->pipe_err);
        proc->pipe_in = 0;
        proc->pipe_out = 0;
        proc->pipe_err = 0;
        proc->pid = 0;
        proc->exit_status = status;
        proc->state = PSTATE_DEAD;
        return 0;
    } else {
        xerror("Process %d not found \n", pid);
        return ERR_NOTFOUND;
    }
	return 0;
}


/* This function scans the modules to see if there are any that need
   to be cleaned up or restarted.  This function should never be called
   from the start_module process or the child signal handler to avoid a
   race condition. */
void
process_scan(void)
{
    int n;
    dax_process *this;

    /* Check the dead module queue for pid's that need cleaning */
    for(n = 0; n < DPQ_SIZE; n++) {
        if(_dpq[n].pid != 0) {
            _cleanup_process(_dpq[n].pid, _dpq[n].status);
            _dpq[n].pid = 0;
        }
    }

    this = _process_list;
    while(this != NULL) {
        this = this->next;
    }
    /* TODO: Restart modules if necessary */
}


/* TODO: Program process_stop() */
int
process_stop(dax_process *proc)
{
    return 0;
}

/* Adds the dead module to the first blank spot in the list.  If the list
   is overflowed then it'll just overwrite the last one.
   TODO: If more than DMQ_SIZE modules dies all at once this will cause
         problems.  Should be fixed. */
void
process_dpq_add(pid_t pid, int status)
{
    int n = 0;
    while(_dpq[n].pid != 0 && n < DPQ_SIZE) {
        n++;
    }
    //xlog(10,"Adding Dead Module pid=%d index=%d",pid,n);
    _dpq[n].pid = pid;
    _dpq[n].status = status;
}

void
_print_process_list(void)
{
    dax_process *this;
    this = _process_list;
    char **args;
    int n = 0;

    pthread_mutex_lock(&proc_mutex);
    while(this != NULL) {
        printf("Process %s\n", this->name);
        printf("  path    = %s\n", this->path);
        args = this->arglist;
        while(args[n] != NULL) {
            printf("  arg[%d]    = %s\n", n, args[n]);
            n++;
        }
        printf("  user     = %s\n", this->user);
        printf("  uid      = %d\n", this->uid);
        printf("  group    = %s\n", this->group);
        printf("  gid      = %d\n", this->gid);
        printf("  env      = %s\n", this->env);
        printf("  waitstr  = %s\n", this->waitstr);
        printf("  timeout  = %d\n", this->timeout);
        printf("  cpu      = %f\n", this->cpu);
        printf("  mem      = %d kB\n", this->mem);
        printf("  pid      = %d\n", this->pid);
        printf("  pipe_in  = %d\n", this->pipe_in);
        printf("  pipe_err = %d\n", this->pipe_err);
        printf("  pipe_out = %d\n", this->pipe_out);

        this = this->next;
    }
    pthread_mutex_unlock(&proc_mutex);
}


