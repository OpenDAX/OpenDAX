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
#include "module.h"
#include "options.h"
#include "func.h"
#include "tagbase.h"

#include <time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <pthread.h>


static dax_module *_current_mod = NULL;
static int _module_count = 0;

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


/* TODO: Need to look for the PID of the module too. */
//static dax_module *
//_get_module_hostpid(in_addr_t host, pid_t pid)
//{
//    dax_module *last;
//
//    if(_current_mod == NULL) return NULL;
//    last = _current_mod;
//    do {
//        if(_current_mod->host == host) {
//            return _current_mod;
//        }
//        _current_mod = _current_mod->next;
//    } while(_current_mod != last);
//    return NULL;
//}

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
    printf(" Module Count = %d\n", _module_count);
    printf(" _current_mod = %p\n", _current_mod);
    do {
        printf("Module %s:%d\n", _current_mod->name, _current_mod->fd);
        printf(" efd = %d\n", _current_mod->fd);
        printf(" host = %d\n", _current_mod->host);
        _current_mod = _current_mod->next;
    } while (_current_mod != last);
}
//#endif


dax_module *
module_add(char *name, unsigned int flags)
{
    dax_module *new;

    new = xmalloc(sizeof(dax_module));
    if(new) {
        printf("New module '%s' created at %p  \n", name, new);
        new->flags = flags;

        new->fd = 0;
        new->event_count = 0;

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
        if(mod->name) free(mod->name);
        free(mod);
        return 0;
    }
    return ERR_ARG;
}

int
module_set_running(int fd)
{
    dax_module *mod;

    mod = _get_module_fd(fd);
    if(mod == NULL) return ERR_NOTFOUND;
    mod->flags &= MSTATE_RUNNING;
    return 0;
}

/* The dax server will not send messages to modules that are not registered.
 * Also modules that are not started by the core need a way to announce
 * themselves. name can be NULL for modules that were started from DAX */
dax_module *
module_register(char *name, u_int32_t timeout, int fd)
{
    dax_module *mod, *test;

    /* If a module with the given file descriptor already exists
     * then we need to unregister that module.  It must have failed
     * or the OS would not give us the file descriptor again. */
    test = _get_module_fd(fd);
    if(test) {
        module_unregister(test->fd);
    }

    mod = module_add(name, 0);
    if(mod) {
        mod->fd = fd;
        mod->timeout = timeout;
        _get_host(fd, &(mod->host));
        mod->state |= MSTATE_STARTED;
        mod->state |= MSTATE_REGISTERED;
    } else {
        xerror("Major problem registering module - %s:%d", name, fd);
        return NULL;
    }
    xlog(LOG_MAJOR,"Added module '%s' at file descriptor %d", name, fd);

    _print_modules();
    return mod;
}


void
module_unregister(int fd)
{
    dax_module *mod;

    mod = _get_module_fd(fd);
    if(mod) {
        events_cleanup(mod);
        module_del(mod);
    } else {
        xerror("module_unregister() - Module File Descriptor %d Not Found", fd);
    }
    _print_modules();
}


dax_module *
module_find_fd(int fd)
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
