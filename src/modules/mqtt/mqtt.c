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
 *
 *  Main source code file for the OpenDAX MQTT module
 */


#include "mqtt.h"
#include <MQTTClient.h>
#include <libdaxlua.h>


void quit_signal(int sig);
static void getout(int exitstatus);

dax_state *ds;
static int _quitsignal;
static int _connected = 0;

MQTTClient client;
MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
MQTTClient_message pubmsg = MQTTClient_message_initializer;
volatile MQTTClient_deliveryToken deliveredtoken;


static int
_write_lua_to_dax(lua_State *L, tag_handle *h) {
    void *data, *mask;
    int q = 0;
    int n, result;

    data = malloc(h->size);
    if(data == NULL) {
        dax_log(LOG_ERROR, "Unable to allocate data area");
    }

    mask = malloc(h->size);
    if(mask == NULL) {
        free(data);
        dax_log(LOG_ERROR, "Unable to allocate mask memory");
    }
    bzero(mask, h->size);

    result = daxlua_lua_to_dax(L, *h, data, mask);
    if(result) {
        free(data);
        free(mask);
        /* The error message should be on top of the stack */
        dax_log(LOG_ERROR, lua_tostring(L, -1));
    }

    /* This checks the mask to determine which function to use
     * to write the data to the server */
    /* TODO: Might want to scan through and find the beginning and end of
     * any changed data and send less through the socket.  */
    for(n = 0; n < h->size && q == 0; n++) {
        if( ((unsigned char *)mask)[n] != 0xFF) {
            q = 1;
        }
    }
    if(q) {
        result = dax_mask_tag(ds, *h, data, mask);
    } else {
        result = dax_write_tag(ds, *h, data);
    }

    free(data);
    free(mask);
    return result;
}


void delivered(void *context, MQTTClient_deliveryToken dt)
{
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}

/* This callback function is called by the PAHO MQTT library whenever a message arrives on
 * a topic that we have subscribed to. */ 
int
msgarrived(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    int size;
    subscriber_t *sub;
    lua_State* L;

    dax_log(LOG_PROTOCOL, "Message arrived on topic '%s'", topicName);

    sub = get_sub(topicName);
    if(sub != NULL) {

        L = dax_get_luastate(ds);

        if(sub->formatter != 0) { /* We need to run our formatter function */
            /* Get the formatter function from the registry and push it onto the stack */
            lua_rawgeti(L, LUA_REGISTRYINDEX, sub->formatter);
            lua_pushstring(L, topicName); /* First argument is the topic */
            lua_pushlstring(L, message->payload, message->payloadlen);
            if(lua_pcall(L, 2, 1, 0) != LUA_OK) {
                dax_log(LOG_ERROR, "Formatter funtion for %s - %s", topicName, lua_tostring(L, -1));
            } else { /* Success */
            
            }
        } else {
            /* No formatter function configured so we just do a raw
             * binary copy to the tag or the tag group */
            if(message->payloadlen != sub->buff_size) {
                dax_log(LOG_WARN, "Message payload is not the same size as the buffer");
            }
            size = MIN(message->payloadlen, sub->buff_size);
            /* TODO we could avoid this memory copy if we know the payload is big enough */
            memcpy(sub->buff, message->payload, size);

            if(sub->group != NULL) { /* We are using a tag group */
                dax_group_write(ds, sub->group, sub->buff);
            } else { /* just one tag */
                dax_write_tag(ds, sub->h[0], sub->buff);
            }
        }
        //payload = strndup(message->payload, message->payloadlen);
        //DF("  sub topic: %s", sub->topic);
        //DF("    message: %s", payload);
        ///free(payload);
    } else {
        /* This is a bad error and it means that we have a problem with how we are finding
         * the subscription topic that matches the one that we received from the broker */
        DF("We received topic '%s' with no subscription match", topicName);
        dax_log(LOG_ERROR, "We received topic '%s' with no subscription match", topicName);
    }

    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

void
connlost(void *context, char *cause)
{
    dax_log(LOG_MINOR, "Connection lost - %s", cause);
    _connected = 0;
}

/* Setup one subscriber and subscribe.  Returns a 1 if we are still not
 * properly initialized.  The parent counts these and will continue to 
 * try these until they either all work or some kind of permanent failure
 * keeps them from ever working */
static int
subscribe(subscriber_t *sub) {
    int result;
    if(sub->tag_count == 0 && sub->formatter == 0) {
        dax_log(LOG_ERROR, "No tags or formatter function given for topic %s", sub->topic);
        sub->enabled = ENABLE_FAIL; /* Can't recover from this */
        return 0; /* We return zero because there is no need to come back here for this one */
    }
    if(sub->tag_count > 0) {
        if(sub->h == NULL) { /* We need to allocate our array of tag handles */
            sub->h = (tag_handle *)malloc(sizeof(tag_handle) * sub->tag_count);
            assert(sub->h != NULL);
        }
        /* Now search through the tagnames and get handles for all of the tags */
        for(int n=0;n<sub->tag_count;n++) {
            result = dax_tag_handle(ds, &sub->h[n], sub->tagnames[n], 1);
            if(result) {
                dax_log(LOG_ERROR, "Unable to add tag %s as subscription", sub->tagnames[n]);
                return 1;
            }
        }
    }
    /* TODO: How should errors be handled here */
    if(sub->tag_count > 1) {
        sub->group = dax_group_add(ds, &result, sub->h, sub->tag_count, 0);
        if(result) {
            dax_log(LOG_ERROR, "Unable to create tag group - %d", result);
        }
        sub->buff = malloc(dax_group_get_size(sub->group));
        if(sub->buff == NULL) {
            dax_log(LOG_ERROR, "Unable to allocate tag data buffer");
        }
        sub->buff_size = dax_group_get_size(sub->group);
    } else if(sub->tag_count == 1) { /* We just have the one tag */
        DF("sub->h = %p", sub->h);
        sub->buff = malloc(sub->h[0].size);
        if(sub->buff == NULL) {
            dax_log(LOG_ERROR, "Unable to allocate tag data buffer");
        }
        sub->buff_size = sub->h[0].size;
    }
    DF("Subscribe to %s", sub->topic);
    result = MQTTClient_subscribe(client, sub->topic, 0);
    if(result != MQTTCLIENT_SUCCESS) {
        dax_log(LOG_ERROR, "Unable to subscribe to %s", sub->topic);
        sub->enabled = ENABLE_FAIL; /* Can't recover from this */
    } else {
        sub->enabled = ENABLE_GOOD; /* We're rolling now */
    }
    return 0;
}


/* Does the initial subscription of all the configured subscribers */
static int
setup_subscribers() {
    subscriber_t *sub;
    int bad_subs = 0;
    
    while((sub = get_sub_iter()) != NULL) {
        if(sub->enabled == ENABLE_UNINIT) {
            bad_subs += subscribe(sub);
        }
    }
    
    return bad_subs;
}

void
event_callback(dax_state *ds, void *udata) {
    publisher_t *pub = (publisher_t *)udata;
    
    if(pub->format_type == CFG_STR) {
        
    }
    printf("Event hit on %s\n", pub->topic);
}

static int
publish(publisher_t *pub) {
    int result;
    dax_type_union val;
    
    if(pub->tag_count == 0) {
        dax_log(LOG_ERROR, "No tags given for topic %s", pub->topic);
        pub->enabled = ENABLE_FAIL; /* Can't recover from this */
        return 0; /* We return zero because there is no need to come back here for this one */
    }
    if(pub->h == NULL) { /* We need to allocate our array of tag handles */
        pub->h = (tag_handle *)malloc(sizeof(tag_handle) * pub->tag_count);
    }
    /* Now search through the tagnames and get handles for all of the tags */
    for(int n=0;n<pub->tag_count;n++) {
        result = dax_tag_handle(ds, &pub->h[n], pub->tagnames[n], 1);
        if(result) {
            dax_log(LOG_ERROR, "Unable to add tag %s to publication", pub->tagnames[n]);
            return 1;
        }            
    }
    if(pub->update_tag == NULL) {
        for(int n=0;n<pub->tag_count;n++) {
            if( pub->event_data != NULL ) {
                printf("Add event for tag %s, with val %s\n", pub->tagnames[n], pub->event_data);
                dax_string_to_val(pub->event_data, pub->h[n].type, &val, NULL, 0);
                result = dax_event_add(ds, &pub->h[n], pub->update_mode, &val, NULL, event_callback, pub, NULL);
            } else {
                printf("Add event for tag %s\n", pub->tagnames[n]);
                result = dax_event_add(ds, &pub->h[n], pub->update_mode, NULL, NULL, event_callback, pub, NULL);
            }
            if(result) {
                dax_log(LOG_ERROR, "Problem adding update event for tag %s", pub->tagnames[n]);
                return 1;
            }
        }
    }
    
    // printf("Set to publish to %s\n", pub->topic);
    pub->enabled = ENABLE_GOOD; /* We're rolling now */
    return 0;
}

/* Does the initial subscription of all the configured subscribers */
static int
setup_publishers() {
    publisher_t *pub;
    int bad_pubs = 0;
    
    while((pub = get_pub_iter()) != NULL) {
        if(pub->enabled == ENABLE_UNINIT) {
            bad_pubs += publish(pub);
        }
    }
    
    return bad_pubs;
}


void
client_loop(void) {
    int result;
    unsigned int cycle_count = 1;
    unsigned int connect_cycle = 1;
    char *attr;
    char conn_str[64];
    int bad_subs = 1;
    int bad_pubs = 1;
    
    snprintf(conn_str, 64, "tcp://%s:%s", dax_get_attr(ds, "broker_ip"), dax_get_attr(ds, "broker_port"));
    MQTTClient_create(&client, conn_str, dax_get_attr(ds, "clientid"), MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 20;
    attr = dax_get_attr(ds, "broker_timeout");
    conn_opts.connectTimeout = atoi(attr);
    conn_opts.cleansession = 1;
    MQTTClient_setCallbacks(client, NULL, connlost, msgarrived, delivered);
    
    while(1) {
        if(!_connected && cycle_count >= connect_cycle) {
            dax_log(LOG_COMM, "Attempting to connect to broker at %s", conn_str);
            if ((result = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
                dax_log(LOG_MINOR, "Failed to connect, return code %d", result);
                connect_cycle*=2;
                if(connect_cycle > 60) connect_cycle = 60; /* We'll try once a minute as a maximum for now */
            } else { /* Connection sucess */
                dax_log(LOG_MINOR, "Connection successful");
                _connected = 1;
                connect_cycle = 1;
                //bad_subs = setup_subscribers();
                //setup_publishers();
            }
            cycle_count = 0;
        }
        /* Every now and then we retry the subscriptions.  This is usually because
         * the tags don't exist in the tag server when we start */
        if(_connected && bad_subs !=0 && (cycle_count % 10) == 0) {
            bad_subs = setup_subscribers();
        }
        if(_connected && bad_pubs !=0 && (cycle_count % 10) == 0) {
            bad_pubs = setup_publishers();
        }

        /* Check to see if the quit flag is set.  If it is then bail */
        if(_quitsignal) {
            dax_log(LOG_MAJOR, "Quitting due to signal %d", _quitsignal);
            getout(_quitsignal);
        }
        dax_event_wait(ds, 1000, NULL);
        cycle_count++;
    }

}

/* main inits and then calls run */
int
main(int argc,char *argv[]) {
    struct sigaction sa;
    
    /* Set up the signal handlers for controlled exit*/
    memset (&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = &quit_signal;
    sigaction (SIGQUIT, &sa, NULL);
    sigaction (SIGINT, &sa, NULL);
    sigaction (SIGTERM, &sa, NULL);

    /* Create and Initialize the OpenDAX library state object */
    ds = dax_init("daxmqtt");
    if(ds == NULL) {
        dax_log(LOG_FATAL, "Unable to Allocate Dax State Object\n");
        exit(-1);
    }

    configure(argc, argv);
    
    /* Check for OpenDAX and register the module */
    if( dax_connect(ds) ) {
        dax_log(LOG_FATAL, "Unable to find OpenDAX");
        exit(-1);
    }

    dax_mod_set(ds, MOD_CMD_RUNNING, NULL);
    dax_log(LOG_MINOR, "MQTT Module Starting");
    client_loop();
    
 /* This is just to make the compiler happy */
    return(0);
}

/* Signal handler */
void
quit_signal(int sig)
{
    _quitsignal = sig;
}

/* We call this function to exit the program */
static void
getout(int exitstatus)
{
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);

    dax_disconnect(ds);
    exit(exitstatus);
}

