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
#include <unistd.h>
#include <signal.h>

/* This array is the dead process list.
   TODO: This should be improved to allow it to grow when needed.
*/
#ifndef DPQ_SIZE
  #define DPQ_SIZE 10
#endif
static dead_process _dpq[DPQ_SIZE];
static dax_process *_process_list = NULL;

#ifdef HAVE_PROCDIR
static long pagesize;

void
process_init(void) {
    pagesize = sysconf(_SC_PAGESIZE);
}
#else
void
process_init(void) {
    return;
}
#endif

/* Return the difference between the two times in mSec */
static long
_difftimeval(struct timeval *start, struct timeval *end)
{
    long result;
    result = (end->tv_sec-start->tv_sec) * 1000;
    result += (end->tv_usec-start->tv_usec) / 1000;
    return result;
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
    xlog(LOG_CONFIG,"Adding process %s to configuration",name);
    
    new = malloc(sizeof(dax_process));
    if(new) {
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
        new->starttime.tv_sec = 0;
        new->starttime.tv_usec = 0;
        new->deadtime.tv_sec = 0;
        new->deadtime.tv_usec = 0;
        new->restartdelay = 0;
        new->restartcount = 0;
        new->pcpu = 0.0;
        new->rss = 0;
        new->last_ptime = 0;
        new->last_etime = 0;

        new->next = NULL;

        if(_process_list == NULL) { /* List is empty */
            _process_list = new;
        } else {
            this = _process_list;
            while(this->next != NULL) this = this->next;
            this->next = new;
        }
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

        if(proc == _process_list) { /* If this is the first process */
            _process_list = _process_list->next;
        } else {
            last = proc;
            while(this != NULL) {
                this = proc->next;
                if(this == proc) {
                    _free_process(proc);

                    return 0;
                }
                last = this;
                this = this->next;
            }
        }
    }
    return ERR_ARG;
}


/* Start all of the processes in the process list */
void
process_start_all(void)
{
    dax_process *this;
    struct timespec timeout, rem;
    int result;

    /* In case we ain't go no list */
    if(_process_list == NULL) return;

    this = _process_list;
    while(this != NULL) {
        process_start(this);

        if(this->delay > 0) {
            rem.tv_sec = this->delay / 1000;
            rem.tv_nsec = (this->delay % 1000) * 1000000;
            result = 1;
            while(result) {
                timeout = rem;
                result = nanosleep(&timeout, &rem);
                if(errno == EFAULT)
                    xfatal("Serious problem with nanosleep in function process_start_all()");
                assert(errno != EINVAL);
            }
        }
        this = this->next;
    }
}

/* This function is used to start a module */
pid_t
process_start(dax_process *proc)
{
    pid_t child_pid;
    int result = 0;

    if(proc) { /* We are the parent */
        if(result) xerror("Unable to properly set up pipes for %s\n", proc->name);
        child_pid = fork();
        if(child_pid > 0) { /* This is the parent */
            proc->pid = child_pid;
            proc->exit_status = 0;

            xlog(LOG_VERBOSE, "Starting Process - %s - %d",proc->path,child_pid);
            gettimeofday(&proc->starttime, NULL);
            proc->state = PSTATE_STARTED;
            return child_pid;
        } else if(child_pid == 0) { /* Child */
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

/* Finds and returns a pointer to the process with the given PID */
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
        xlog(LOG_MINOR, "Cleaning up Process %d - Returned Status %d", pid, status);
        proc->pid = 0;
        proc->last_etime = 0;
        proc->last_ptime = 0;
        proc->exit_status = status;
        proc->state = PSTATE_DEAD;
        gettimeofday(&proc->deadtime, NULL);
        return 0;
    } else {
        xerror("Process %d not found for cleanup", pid);
        return ERR_NOTFOUND;
    }
    return 0;
}

#ifdef HAVE_PROCDIR
/* reads the /proc/stat and proc/[pid]/stat files and gets the relevant
 * process information and writes it into the process pointed to by proc */
static int
_process_get_cpu_stat(dax_process *proc)
{
    FILE *f;
    char filename[128];
    char line[1024];
    int pid, result;
    unsigned long utime, stime, ptime, etime, times[4];
    long rss;

    snprintf(filename, 128, "/proc/%d/stat", proc->pid);
    f = fopen(filename, "r");
    if(f == NULL) {
        xerror("Unable to open file %s", filename);
        return errno;
    }
    result = fscanf(f, "%d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu %lu %*d %*d %*d %*d %*d %*d %*u %*u %ld", &pid, &utime, &stime, &rss);
    if(result < 4) return -1;
    ptime = utime + stime;
    fclose(f);
    /* Now read and parse the cpu stat file */
    f = fopen("/proc/stat", "r");
    if(f == NULL) {
        xerror("Unable to open file proc/stat for pid %d", proc->pid);
        return errno;
    }
    fgets(line, 1024, f); /* For now we are ignoring the first line of the file. */
    /* TODO: We should determine which cpu we are running on and read that line from the file.
     * For now we are just going to assume that all the cpus run at the same rate */
    result = fscanf(f, "%*s %lu %lu %lu %lu", &times[0], &times[1], &times[2], &times[3]);
    fclose(f);
    if(result < 4) return -1;
    etime = times[0] + times[1]+ times[2] + times[3];
    proc->pcpu = ((float)ptime - (float)proc->last_ptime) / ((float)etime - (float)proc->last_etime) * 100.0;

    proc->last_ptime = ptime;
    proc->last_etime = etime;
    proc->rss = rss * pagesize / 1024;
    return 0;
}
#else
/* San's a good alternative we'll just set everything to zero. */
static int
_process_get_cpu_stat(dax_process *proc)
{
    proc->pcpu = 0.0;
    proc->rss = 0;
    return 0;
}
#endif


static inline void
_process_restart(dax_process *proc)
{
    xlog(LOG_MAJOR, "Restarting Process - %s - Delay = %d mSec", proc->name, proc->restartdelay);
    proc->restartcount++;
    process_start(proc);
    /* Now we increase the restart delay */
    if(proc->restartdelay == 0) {
        proc->restartdelay = 100;
    } else {
        proc->restartdelay *= 2;
    }
    /* Maximum of one minute */
    if(proc->restartdelay > 60000) {
        proc->restartdelay = 60000;
    }
}

/* This function scans the modules to see if there are any that need
   to be cleaned up or restarted.  This function should never be called
   from the start_module process or the child signal handler to avoid a
   race condition. */
/* TODO: Make this function return the number of milliseconds to sleep for the main
 * routine so that we can more precisely control the restart timing. */
int
process_scan(void)
{
    int n, restart, interval;
    long runtime;
    dax_process *this;
    struct timeval now;
    static unsigned int count;

    /* Check the dead module queue for pid's that need cleaning */
    for(n = 0; n < DPQ_SIZE; n++) {
        if(_dpq[n].pid != 0) {
            _cleanup_process(_dpq[n].pid, _dpq[n].status);
            _dpq[n].pid = 0;
        }
    }

    this = _process_list;
    restart = 60000;
    while(this != NULL) {
        /* See if the process needs restarting. */
        if((this->flags & PFLAG_RESTART) && this->state == PSTATE_DEAD ) {
            gettimeofday(&now, NULL);
            runtime = _difftimeval(&this->starttime, &this->deadtime);
            if(runtime > 60000) this->restartdelay = 0;
            /* interval is how long it's been since the process died */
            interval = _difftimeval(&this->deadtime, &now);
            if(interval > this->restartdelay) {
                _process_restart(this);
            } else {
                if((this->restartdelay - interval) < restart) {
                    restart = this->restartdelay - interval;
                }
            }
        } else {
            /* approximately every minute we'll get cpu stats and deal with
             * the processes that misbehave.  We add the pid so that we don't
             * do every process in the same scan */
            if((count + this->pid) % 64 == 0) {
                /* If we come back around and have been condemned then we
                 * need to kill it the hard way.*/
                if(this->state == PSTATE_CONDEMNED) {
                    xlog(LOG_MODULE, "Killing process %s [%d]", this->name, this->pid);
                    kill(this->pid, SIGKILL);
                }
                _process_get_cpu_stat(this);
                if(this->cpu > 0.0 && this->pcpu > this->cpu) {
                    xlog(LOG_MODULE, "Process %s [%d] has exceeded CPU usage of %f",
                            this->name, this->pid, this->cpu);
                    this->state = PSTATE_CONDEMNED;
                }
                if(this->mem > 0 && this->rss > this->mem) {
                    xlog(LOG_MODULE, "Process %s [%d] has exceeded Memory usage of %ld",
                            this->name, this->pid, this->mem);
                    this->state = PSTATE_CONDEMNED;
                }
                /* The first time through we terminate the process the easy way */
                if(this->state == PSTATE_CONDEMNED) {
                    xlog(LOG_MODULE, "Commanding process %s [%d] to quit",
                            this->name, this->pid);
                    kill(this->pid, SIGQUIT);
                }
                xlog(LOG_VERBOSE | LOG_MODULE, "%s: PID = %d, CPU = %f%%, Memory = %ld kb", this->name, this->pid, this->pcpu, this->rss);
            }
        }
        this = this->next;
    }
    count++;
    return restart;
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
        printf("  flags    = 0x%X\n", this->flags);
        printf("  delay    = %d\n", this->delay);
        printf("  rsdelay  = %ld\n", this->restartdelay);
        printf("  cpu      = %f\n", this->cpu);
        printf("  mem      = %d kB\n", this->mem);
        printf("  pid      = %d\n", this->pid);

        this = this->next;
    }
}


