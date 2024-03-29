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
        dax_log(DAX_LOG_ERROR, "_get_host %s", strerror(errno));
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
                dax_log(DAX_LOG_ERROR, "_get_host %s", strerror(errno));
            } else {
                addr_in = (struct sockaddr_in *)&addr;
                if(addr_in->sin_addr.s_addr == *host) {
                    *host = 0;
                }
            }
            return 0;
        } else {
            dax_log(DAX_LOG_ERROR, "Unable to identify socket type in module registration");
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
        dax_log(DAX_LOG_DEBUG, "New module '%s' created at %p", name, new);
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

/* This function checks to see if we have asked for this module to be
 * excluded from the module tag creation. Returns 0 normally or 1 if
 * the module tag should NOT be created */
int
_check_module_tag_exclusion(char *name) {
    static int firstrun = 1;
    static char **x_list; /* The exclusion list */
    static int list_size;
    char *temp, *c, *tok;
    int n;

    if(firstrun) {
        c = opt_mod_tag_exclude(); /* space delimited list of modules */
        if(c != NULL) {
            temp = strdup(c);
            if(temp == NULL) {
                dax_log(DAX_LOG_ERROR, "Unable to allocate memory to save module tag exclusion list");
                return 0;
            }
            /* Count the number of module names in the string*/
            tok = strtok(temp, " ");
            while(tok != NULL) {
                list_size++;
                tok = strtok(NULL, " ");
            }
            free(temp); /* strtok() mangles this so we need to start over */

            x_list = malloc(sizeof(char *) * list_size);
            if(x_list == NULL) {
                dax_log(DAX_LOG_ERROR, "Unable to allocate memory for module tag exclusion list");
                return 0;
            }
            temp = strdup(c); /* get another copy */
            if(temp == NULL) {
                dax_log(DAX_LOG_ERROR, "Unable to allocate memory to save module tag exclusion list");
                return 0;
            }
            tok = strtok(temp, " ");
            n=0;
            while(tok != NULL) {
                dax_log(DAX_LOG_DEBUG, "Adding '%s' to the module tag exclusion list", tok);
                x_list[n++] = tok;
                tok = strtok(NULL, " ");
            }
        }
    firstrun = 0;
    }

    /* For now we are just doing a linear search.  This list will probably be pretty
     * small and module registrations are infrequent*/
    if(x_list != NULL) { /* We may not have any */
        for(n=0;n<list_size;n++) {
            if(strcmp(x_list[n], name) == 0) return 1;
        }
    }

    return 0;
}

/* The dax server will not send messages to modules that are not registered.
 * Also modules that are not started by the core need a way to announce
 * themselves. name can be NULL for modules that were started from DAX */
dax_module *
module_register(char *name, uint32_t timeout, int fd)
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
        if(! _check_module_tag_exclusion(name)) { /* Decide if we are making a tag or not */
         /* The tagname for the module tag.  It will be the module name with _m
            prepended.  If there is a duplicate (multiple modules of the same
            name) then a number will be added to the end of the name and a
            warning generated */
            strcpy(tagname, "_m");
            strncat(tagname, name, DAX_TAGNAME_SIZE - 2);
            result = tag_get_name(tagname, NULL);
            if(result==0) {
                if(_mod_uuid < 1000) {
                    x = MIN(strlen(tagname), DAX_TAGNAME_SIZE -3);
                    sprintf(&tagname[x], "%03d", _mod_uuid++);
                } else {
                    x = MIN(strlen(tagname), DAX_TAGNAME_SIZE -5);
                    sprintf(&tagname[x], "%05d", _mod_uuid++);
                }
                dax_log(DAX_LOG_WARN, "Duplicate module name.  Module tag being modified - %s", tagname);
            }
            result = tag_add(-1, tagname, cdt_get_type("_module"), 1, 0);
            if(result < 0) {
                dax_log(DAX_LOG_ERROR, "Unable to add module tag- %s", tagname);
            } else {
                mod->tagindex = result;
            }
            starttime = xtime();
            tag_write(-1, result, 0, &starttime, sizeof(dax_time));
            tag_write(-1, result, MOD_ID_OFFSET, &fd, sizeof(int));
            /* We wait until after we have written what we want to set the special flag.
            From now on we'll let the special functions handle the tag */
            tag_set_attribute(result, TAG_ATTR_SPECIAL);
            tag_write(-1, INDEX_LASTMODULE, 0, tagname, DAX_TAGNAME_SIZE);
        }
    } else {
        dax_log(DAX_LOG_ERROR, "Major problem registering module - %s:%d", name, fd);
        return NULL;
    }
    dax_log(DAX_LOG_MAJOR,"Added module '%s' at file descriptor %d", name, fd);

    return mod;
}


void
module_unregister(int fd)
{
    dax_module *mod;
    tag_index tag_count;

    mod = _get_module_fd(fd);
    if(mod != NULL) {
        dax_log(DAX_LOG_MAJOR,"Removing module '%s' at file descriptor %d", mod->name, fd);
        events_cleanup(mod);
        groups_cleanup(mod);
        /* This deletes any tag that is owned by this module. */
        tag_count = get_tagindex();
        for(tag_index n=0;n<tag_count;n++) {
            if(is_tag_owned(fd, n)) {
                tag_del(n);
            }
        }
        tag_del(mod->tagindex);
        module_del(mod);
    } else {
        dax_log(DAX_LOG_ERROR, "module_unregister() - Module File Descriptor %d Not Found", fd);
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
