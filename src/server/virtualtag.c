/*  OpenDAX - An open source data acquisition and control system
 *  Copyright (c) 2020 Phil Birkelbach
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

 *  Source file that contains all of the virtual tag callback functions.
 */


/* Virtual tags are ones whose data is not stored in memory in the tag
 * database.  Instead the .data field of the tag database entry, which
 * is normally a pointer to the allocated data, is used as a pointer to
 * one of these functions.  They should all have the same prototype and
 * that prototype is...
 * 
 * int vfunction(tag_index idx, int offset, void *data, int size, void *userdata);
 * 
 * the functions should check that size is correct and then write the
 * information into the memory pointed to by *data.  offset may be
 * used as a parameter which would allow these tags to appear to be
 * arrays.
 */

#include "virtualtag.h"
#include "tagbase.h"
#include "message.h"
#include "module.h"

/* This is the fd of the module of the current request.  This is so
 * that virtual functions in tagbase.c can know which module they are
 * dealing with.  There is probably a better way to do this but this works
 * for now. */
static int _current_fd;


void
virt_set_fd(int fd) {
    _current_fd = fd;
}

int
server_time(tag_index idx, int offset, void *data, int size, void *userdata)
{
    dax_time t;

    t = xtime();
    memcpy(data, &t, size);
    return 0;
}

int
get_module_tag_name(tag_index idx, int offset, void *data, int size, void *userdata) {
    int result;
    dax_tag tag;
    dax_module *mod;

    mod = module_find_fd(_current_fd);
    if(mod == NULL) return ERR_NOTFOUND;
    result = tag_get_index(mod->tagindex, &tag);
    if(result) return result;
    bzero(data, DAX_TAGNAME_SIZE);
    memcpy(data, tag.name, strlen(tag.name)+1);
    return 0;
}


/* These functions deal with tag based queues */
int
write_queue(tag_index idx, int offset, void *data, int size, void *userdata) {
    tag_queue *q;

    int next, newqsize;
    u_int8_t *new_queue;

    /* We only allow writing the entire tag.  Doing otherwise
     * would be ambiguous */
    q = (tag_queue *)userdata;
    if(offset != 0 || size != q->size) return ERR_ILLEGAL;
    /* We need to grow the queue */
    if(q->qcount == q->qsize) {
        newqsize = q->qsize* 2;
        new_queue = realloc(q->queue, sizeof(void *) * newqsize);
        if(new_queue == NULL) return ERR_ALLOC;
        q->queue = (void **)new_queue;
        /* After the queue size is increased we need to move the fragment that is now
         * at the beginning of the queue to the start of the new area */
        if(q->qread > 0) {
            memcpy(&q->queue[q->qsize], &q->queue[0], q->qread*sizeof(void *));
        }
        q->qsize = newqsize;
    }
    next = (q->qread + q->qcount) % q->qsize;
    if(q->queue[next] == NULL) {
        q->queue[next] = malloc(q->size);
        if(q->queue[next] == NULL) return ERR_ALLOC;
    }
    memcpy(q->queue[next], data, q->size);
    q->qcount++;
    /* This will break if we have any event other than "WRITE" */
    event_check(idx, offset, size);

    return 0;
}

int
read_queue(tag_index idx, int offset, void *data, int size, void *userdata) {
    tag_queue *q;

    q = (tag_queue *)userdata;
    if(q->qcount == 0) {
        return ERR_EMPTY;
    }
    memcpy(data, q->queue[q->qread], q->size);
    q->qread++;
    if(q->qread == q->qsize)  {
        q->qread = 0;
    }
    q->qcount--;
    return 0;
}

