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
 *  Main source code file for the OpenDAX MQTT module
 */


#include "mqtt.h"

void quit_signal(int sig);
static void getout(int exitstatus);

dax_state *ds;
static int _quitsignal;

/* TODO: Event notification logic, creating and manipulating
 * a compound data type.  I might use a separate function to
 * show some of the more advanced features of the API. */

/* main inits and then calls run */
int main(int argc,char *argv[]) {
    struct sigaction sa;
    int flags, result = 0, scan = 0, n;
    char *str, *tagname, *event_tag, *event_type;
    Handle h_full, h_part;
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
    while(1) {
        /* Check to see if the quit flag is set.  If it is then bail */
        if(_quitsignal) {
            dax_debug(ds, LOG_MAJOR, "Quitting due to signal %d", _quitsignal);
            getout(_quitsignal);
        }
        sleep(1);
    }

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
    dax_disconnect(ds);
    exit(exitstatus);
}


// #include "stdio.h"
// #include "stdlib.h"
// #include "string.h"
// #include "MQTTClient.h"
// 
// #define ADDRESS     "tcp://192.168.1.157:1883"
// #define CLIENTID    "ExampleClientPub"
// #define TOPIC       "MQTT Examples"
// #define PAYLOAD     "Hello World! Again!!"
// #define QOS         1
// #define TIMEOUT     10000L
// 
// int main(int argc, char* argv[])
// {
//     MQTTClient client;
//     MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
//     MQTTClient_message pubmsg = MQTTClient_message_initializer;
//     MQTTClient_deliveryToken token;
//     int rc;
// 
//     MQTTClient_create(&client, ADDRESS, CLIENTID,
//         MQTTCLIENT_PERSISTENCE_NONE, NULL);
//     conn_opts.keepAliveInterval = 20;
//     conn_opts.cleansession = 1;
// 
//     if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
//     {
//         printf("Failed to connect, return code %d\n", rc);
//         exit(-1);
//     }
//     pubmsg.payload = PAYLOAD;
//     pubmsg.payloadlen = strlen(PAYLOAD);
//     pubmsg.qos = QOS;
//     pubmsg.retained = 0;
//     MQTTClient_publishMessage(client, TOPIC, &pubmsg, &token);
//     printf("Waiting for up to %d seconds for publication of %s\n"
//             "on topic %s for client with ClientID: %s\n",
//             (int)(TIMEOUT/1000), PAYLOAD, TOPIC, CLIENTID);
//     rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
//     printf("Message with delivery token %d delivered\n", token);
//     MQTTClient_disconnect(client, 10000);
//     MQTTClient_destroy(&client);
//     return rc;
// }
