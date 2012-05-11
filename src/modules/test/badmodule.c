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
 *
 *  Main source code file for the OpenDAX Bad Module
 */

/* This module is designed to misbehave.  It's purpose is for load testing
 * or testing features of the system that are supposed to detect misbehaviour
 * and act accordingly.  It can be made to flood the system with messages,
 * use up all the memory or use up as much CPU as possible.  It reads tags
 * from the tagserver to determine how to act so that other modules can 
 * force the behavior.
 */

#include <badmodule.h>

static dax_state *ds;
static Handle quit_tag;
static Handle cpu_tag;
static Handle mem_tag;
static Handle msg_tag;
static dax_dint quit_time;
static dax_int cpu_percent;
static dax_dint mem_usage;
static dax_dint msg_storm;

/* Return the difference between the two times in mSec */
static long
__difftimeval(struct timeval *start, struct timeval *end)
{
    long result;
    result = (end->tv_sec - start->tv_sec) * 1000;
    result += (end->tv_usec - start->tv_usec) / 1000;
    return result;
}

/* This is our event callback.  It is called whenever one of our tags has changed */
static void
__update_tags(void *x)
{
    dax_read_tag(ds, quit_tag, &quit_time);
    dax_read_tag(ds, cpu_tag, &cpu_percent);
    if(cpu_percent > 100) cpu_percent = 100;
    if(cpu_percent < 0) cpu_percent = 0;
    dax_read_tag(ds, mem_tag, &mem_usage);
    dax_read_tag(ds, msg_tag, &msg_storm);
}

/* Build tagnames, create tags and add the events */
static int
__add_tags(void)
{
    int result;
    char tagname[256];
    snprintf(tagname, 256, "%s_quit", dax_get_attr(ds, "tagprefix"));
    result = dax_tag_add(ds,&quit_tag, tagname, DAX_DINT, 1);
    snprintf(tagname, 256, "%s_cpu", dax_get_attr(ds, "tagprefix"));
    result += dax_tag_add(ds,&cpu_tag, tagname, DAX_INT, 1);
    snprintf(tagname, 256, "%s_mem", dax_get_attr(ds, "tagprefix"));
    result += dax_tag_add(ds,&mem_tag, tagname, DAX_DINT, 1);
    snprintf(tagname, 256, "%s_msg", dax_get_attr(ds, "tagprefix"));
    result += dax_tag_add(ds,&msg_tag, tagname, DAX_DINT, 1);
    if(result) dax_fatal(ds, "Can't Add Tags");
    dax_event_add(ds, &quit_tag, EVENT_CHANGE, NULL, NULL, __update_tags, NULL, NULL);
    dax_event_add(ds, &cpu_tag, EVENT_CHANGE, NULL, NULL, __update_tags, NULL, NULL);
    dax_event_add(ds, &mem_tag, EVENT_CHANGE, NULL, NULL, __update_tags, NULL, NULL);
    dax_event_add(ds, &msg_tag, EVENT_CHANGE, NULL, NULL, __update_tags, NULL, NULL);
    return result;
}

/* This function is to waste memory.  If it fails we don't really care.  That
 * just means that we've got all we can get.  The right thing to do is keep running
 * because that might be the misbehaviour that we are looling for.
 */
static inline void
__waste_memory(size_t size)
{
    static int *waste, *result;
    static size_t current_size;
    size_t n;

    /* if it's not a new size then we do nothing */
    if(current_size != size) {
        result = realloc(waste, size * 1024);
        if(result != NULL) {
            waste = result;
            /* exercise the memory so the kernel doesn't cheat us */
            for(n = 0; n < ((size * 1024) / sizeof(int)); n++) {
                waste[n] = n;
            }
        }
        current_size = size;
    }
}

int
main(int argc,char *argv[])
{
    int result=0, flags;
    struct timeval start, now, last;

    ds = dax_init("badmodule");
    if(ds == NULL) {
        fprintf(stderr, "Unable to Allocate DaxState Object\n");
        return ERR_ALLOC;
    }
    
    dax_log(ds, "Starting bad module");
    dax_set_debug_topic(ds, 0xFFFF); /* This should get them all out there */
        
    dax_init_config(ds, "badmodule");
    flags = CFG_CMDLINE | CFG_DAXCONF | CFG_ARG_REQUIRED;
    result += dax_add_attribute(ds, "tagprefix","tagprefix", 't', flags, "bad");
    
    dax_configure(ds, argc, argv, CFG_CMDLINE | CFG_DAXCONF | CFG_MODCONF);
    
    if(dax_connect(ds))
        dax_fatal(ds, "Unable to register with the server");
    __add_tags();
    
    /* Free the configuration memory once we are done with it */
    dax_free_config(ds);
    /* Read all the tags from the server */
    __update_tags(NULL);
    /* If we have a quit time in the server then we'll just wait it out then bail */
    if(quit_time > 0) {
        usleep(quit_time);
        dax_fatal(ds, "Quiting because I'm out of time");
    }
    gettimeofday(&start, NULL);
    last = start;
    while(1) {
        gettimeofday(&now, NULL);
        /* Here we check the quit time again in case it has changed since we started */
        if(quit_time > 0 && __difftimeval(&start, &now) > quit_time) {
            dax_fatal(ds, "Quiting because I'm out of time");
        }
        __waste_memory(mem_usage);
        if(cpu_percent > 0) {
            while(__difftimeval(&last, &now) < (cpu_percent *10)) {
                gettimeofday(&now, NULL);
            }
            last = now;
            usleep((100 - cpu_percent)*10000);
        } else {
            sleep(1);
        }
        dax_event_poll(ds, NULL);
	}
    dax_disconnect(ds);
    
    return 0;
}
