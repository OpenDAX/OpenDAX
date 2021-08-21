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
#include "groups.h"

#include <time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <pthread.h>


static dax_module *_current_mod = NULL;
static int _module_count = 0;
static int _mod_uuid = 0;

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
            xerror("Unable to identify socket type in module registration");
        }
    }
    return ERR_NOTFOUND;
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

dax_module *
module_add(char *name, unsigned int flags)
{
    dax_module *new;

    new = xmalloc(sizeof(dax_module));
    if(new) {
        xlog(LOG_MAJOR, "New module '%s' created at %p", name, new);
        new->flags = flags;

        new->fd = 0;
        new->event_count = 0;
        new->tag_groups = NULL;
        new->groups_size = 0;

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
    mod->state |= MSTATE_RUNNING;
    tag_write(mod->tagindex, sizeof(dax_time), &mod->state, sizeof(dax_uint));
    return 0;
}

/* The dax server will not send messages to modules that are not registered.
 * Also modules that are not started by the core need a way to announce
 * themselves. name can be NULL for modules that were started from DAX */
dax_module *
module_register(char *name, u_int32_t timeout, int fd)
{
    dax_module *mod, *test;
    char tagname[DAX_TAGNAME_SIZE + 1];
    int result, x;
    dax_time starttime;

    bzero(tagname, DAX_TAGNAME_SIZE + 1);
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
        strcpy(tagname, "_m");
        strncat(tagname, name, DAX_TAGNAME_SIZE - 2);
        if(_mod_uuid < 1000) {
            x = MIN(strlen(tagname), DAX_TAGNAME_SIZE -3);
            sprintf(&tagname[x], "%03d", _mod_uuid++);
        } else {
            x = MIN(strlen(tagname), DAX_TAGNAME_SIZE -5);
            sprintf(&tagname[x], "%05d", _mod_uuid++);
        }
        result = tag_add(tagname, cdt_get_type("_module"), 1, TAG_ATTR_READONLY);
        if(result < 0) {
            xerror("Unable to add module tag- %s", tagname);
        } else {
            mod->tagindex = result;
        }
        tag_write(INDEX_LASTMODULE, 0, tagname, DAX_TAGNAME_SIZE);
        starttime = xtime();
        tag_write(result, 0, &starttime, sizeof(dax_time));
        tag_write(result, sizeof(dax_time), &mod->state, sizeof(dax_uint));
    } else {
        xerror("Major problem registering module - %s:%d", name, fd);
        return NULL;
    }
    xlog(LOG_MAJOR,"Added module '%s' at file descriptor %d", name, fd);

    return mod;
}


void
module_unregister(int fd)
{
    dax_module *mod;

    mod = _get_module_fd(fd);
    if(mod != NULL) {
        xlog(LOG_MAJOR,"Removing module '%s' at file descriptor %d", mod->name, fd);
        events_cleanup(mod);
        groups_cleanup(mod);
        tag_del(mod->tagindex);
        module_del(mod);
    } else {
        xerror("module_unregister() - Module File Descriptor %d Not Found", fd);
    }
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
