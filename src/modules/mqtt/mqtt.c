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
#include "MQTTClient.h"


void quit_signal(int sig);
static void getout(int exitstatus);

dax_state *ds;
static int _quitsignal;
static int _connected = 0;

MQTTClient client;
MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
MQTTClient_message pubmsg = MQTTClient_message_initializer;
volatile MQTTClient_deliveryToken deliveredtoken;

void delivered(void *context, MQTTClient_deliveryToken dt)
{
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}

/* Cheating for now and just writing one and ignoring the formatting */
void
_write_formatted_string(subscriber_t *sub, char *payload)
{
    u_int8_t v[8];
    
    dax_string_to_val(payload, sub->h[0].type, v, NULL, 0);
    dax_write_tag(ds, sub->h[0], v);
}

int
msgarrived(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    int i;
    char* payload;
    subscriber_t *sub;
    
    sub = get_sub(topicName);
    if(sub != NULL) {
        payload = strndup(message->payload, message->payloadlen);
        if(sub->format_type == CFG_STR) {
            _write_formatted_string(sub, payload);
        }

        printf("Message arrived\n");
        printf("     topic: %s\n", sub->topic);
        printf("   message: %s\n", payload);

        free(payload);
    }

    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

void
connlost(void *context, char *cause)
{
    dax_log(ds, "Connection lost - %s", cause);
    _connected = 0;
}

/* Setup one subscriber and subscribe.  Returns a 1 if we are still not
 * properly initialized.  The parent counts these and will continue to 
 * try these until they either all work or some kind of permanent failure
 * keeps them from ever working */
static int
subscribe(subscriber_t *sub) {
    int result;
    if(sub->tag_count == 0) {
        dax_error(ds, "No tags given for topic %s", sub->topic);
        sub->enabled = ENABLE_FAIL; /* Can't recover from this */
        return 0; /* We return zero because there is no need to come back here for this one */
    }
    if(sub->h == NULL) { /* We need to allocate our array of tag handles */
        sub->h = (tag_handle *)malloc(sizeof(tag_handle) * sub->tag_count);
    }
    /* Now search through the tagnames and get handles for all of the tags */
    for(int n=0;n<sub->tag_count;n++) {
        result = dax_tag_handle(ds, &sub->h[n], sub->tagnames[n], 1);
        if(result) {
            dax_error(ds, "Unable to add tag %s as subscription", sub->tagnames[n]);
            return 1;
        }
    }
    // printf("Subscribe to %s\n", sub->topic);
    MQTTClient_subscribe(client, sub->topic, 0);
    sub->enabled = ENABLE_GOOD; /* We're rolling now */
    return 0;
}




/* Does the initial subscription of all the configured subscribers */
static int
setup_subscribers() {
    int result;
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
        dax_error(ds, "No tags given for topic %s", pub->topic);
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
            dax_error(ds, "Unable to add tag %s to publication", pub->tagnames[n]);
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
                dax_error(ds, "Problem adding update event for tag %s", pub->tagnames[n]);
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
    int result;
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
            dax_debug(ds, LOG_VERBOSE, "Attempting to connect to broker at %s", conn_str);
            if ((result = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
                dax_log(ds, "Failed to connect, return code %d", result);
                connect_cycle*=2;
                if(connect_cycle > 60) connect_cycle = 60; /* We'll try once a minute as a maximum for now */
            } else { /* Connection sucess */
                dax_log(ds, "Connection successful");
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
            dax_debug(ds, LOG_MAJOR, "Quitting due to signal %d", _quitsignal);
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
    int flags, result = 0, scan = 0, n;
    char *str, *tagname, *event_tag, *event_type;
    tag_handle h_full, h_part;
    dax_dint data[5];

    /* Set up the signal handlers for controlled exit*/
    memset (&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = &quit_signal;
    sigaction (SIGQUIT, &sa, NULL);
    sigaction (SIGINT, &sa, NULL);
    sigaction (SIGTERM, &sa, NULL);

    /* Create and Initialize the OpenDAX library state object */
    ds = dax_init("daxmqtt");
    if(ds == NULL) {
        dax_fatal(ds, "Unable to Allocate Dax State Object\n");
    }

    configure(argc, argv);
    
    /* Set the logging flags to show all the messages */
    dax_set_debug_topic(ds, LOG_ALL);

    /* Check for OpenDAX and register the module */
    if( dax_connect(ds) ) {
        dax_fatal(ds, "Unable to find OpenDAX");
    }

    dax_mod_set(ds, MOD_CMD_RUNNING, NULL);
    dax_log(ds, "MQTT Module Starting");
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

