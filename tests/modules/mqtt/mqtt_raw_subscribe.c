/*  OpenDAX - An open source data acquisition and control system
 *  Copyright (c) 2022 Phil Birkelbach
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
#include <opendax.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "../modtest_common.h"
#include <MQTTClient.h>

void getout(int);


MQTTClient client;
MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
MQTTClient_message pubmsg = MQTTClient_message_initializer;
volatile MQTTClient_deliveryToken deliveredtoken;
pid_t server_pid, mod_pid;
dax_state *ds;
tag_handle h1, h2, h3, h4;



/* Tries to read the tag, if failure waits a few mSec and tries
 * again.  After so many tries we exit with failure*/
static void
_get_tag(tag_handle *h, char *tagname, int count) {
    int n;
    int result;

    for(n = 0; n<5;n++) {
        result =  dax_tag_handle(ds, h, tagname, 0);
        if(result) {
            DF("CAN'T FIND TAG");
            usleep(10000);
        } else {
            break;
        }
    }
    if(n==5) {
        DF("Could never get the tag");
        getout(-1);
    }
}

static void
_publish(char *topic, int len, void *buff) {
    int result;

    DF("Publish");
    result = MQTTClient_publish(client, topic, len, buff, 0, 0, NULL);
    if(result != MQTTCLIENT_SUCCESS) {
        dax_log(LOG_FATAL, "Unable to publish data to %s", topic);
        getout(-1);
    }

}

static int
_test_single(void) {
    dax_dint test;

    test = 0x12345678;
    _publish("dax_topic_1", 4, &test);
    test = 0;
    usleep(10000);
    dax_read_tag(ds, h1, &test);
    
    if(test != 0x12345678) {
        DF("Single Test Fail");
        return -1;
    }
    return 0;
}

static int
_test_multiple(void) {
    dax_dint values[3];
    dax_dint test[] = {0,0,0};

    values[0] = 1234;
    values[1] = 5678;
    values[2] = 3344;
    _publish("dax_topic_2", 12, values);
    usleep(10000);
    dax_read_tag(ds, h1, &test[0]);
    dax_read_tag(ds, h2, &test[1]);
    dax_read_tag(ds, h3, &test[2]);
    
    for(int n=0;n<3;n++) {
        if(test[n] != values[n]) {
            DF("test[%d] != values[%d], %d, %d", n, n, test[n], values[n]);
            return -1;
        }
    }
    return 0;
}

static int
_test_array(void) {
    dax_dint values[4];
    dax_dint test[] = {0,0,0,0};

    values[0] = 0x12345678;
    values[1] = 0xabcdef12;
    values[2] = 0x77777777;
    values[3] = 0x88888888;
    _publish("dax_topic_3", 16, values);
    usleep(10000);
    dax_read_tag(ds, h4, test);
    
    for(int n=0;n<4;n++) {
        if(test[n] != values[n]) {
            DF("test[%d] != values[%d], %d, %d", n, n, test[n], values[n]);
            return -1;
        }
    }
    return 0;
}

static int
_test_hybrid(void) {
    dax_dint values[5];
    dax_dint test[] = {0,0,0,0,0};

    values[0] = 0x12345678;
    values[1] = 0xabcdef12;
    values[2] = 0x77777777;
    values[3] = 0x88888888;
    values[4] = 0x99999999;
    _publish("dax_topic_4", 20, values);
    usleep(10000);
    dax_read_tag(ds, h1, &test[0]);
    dax_read_tag(ds, h4, &test[1]);
    
    for(int n=0;n<5;n++) {
        if(test[n] != values[n]) {
            DF("test[%d] != values[%d], %d, %d", n, n, test[n], values[n]);
            return -1;
        }
    }
    return 0;
}


void delivered(void *context, MQTTClient_deliveryToken dt)
{
    DF("Message with token value %d delivery confirmed", dt);
    deliveredtoken = dt;
}

int
msgarrived(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    return 0;
}


int
main(int argc, char *argv[])
{
    int s;
    uint8_t buff[1024];
    char conn_str[64];
    int status, n, i;
    int result;

    /* Run the tag server and the modbus module */
    server_pid = run_server();
    mod_pid = run_module("../../../src/modules/mqtt/daxmqtt", "conf/raw_subscribe.conf");
    /* Connect to the tag server */
    ds = dax_init("test");
    if(ds == NULL) {
        dax_log(LOG_FATAL, "Unable to Allocate DaxState Object\n");
        kill(getpid(), SIGQUIT);
    }
    dax_init_config(ds, "test");
    dax_configure(ds, argc, argv, CFG_CMDLINE);
    result = dax_connect(ds);
    if(result) {
        DF("Can't connect to tagserver");
        getout(-1);
    }
    snprintf(conn_str, 64, "tcp://%s:%s", "127.0.0.1", "1883");
    MQTTClient_create(&client, conn_str, "OpenDAX MQTT Test Client", MQTTCLIENT_PERSISTENCE_NONE, NULL);
    MQTTClient_setCallbacks(client, NULL, NULL, msgarrived, delivered);
    conn_opts.keepAliveInterval = 20;
    conn_opts.connectTimeout = 10;
    conn_opts.cleansession = 1;
 
    if ((result = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        dax_log(LOG_FATAL, "Failed to connect, return code %d", result);
        getout(-1);
    }

    _get_tag(&h1, "tag_1", 0);
    _get_tag(&h2, "tag_2", 0);
    _get_tag(&h3, "tag_3", 0);
    _get_tag(&h4, "tag_4", 0);

    if(_test_single() != 0) getout(-1);
    if(_test_multiple() != 0) getout(-1);
    if(_test_array() != 0) getout(-1);
    if(_test_hybrid() != 0) getout(-1);
    DF("PASSED!");
    getout(0);
    
}

void
getout(int exit_status) {
    int status;

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    dax_disconnect(ds);
    kill(mod_pid, SIGINT);
    kill(server_pid, SIGINT);
    if( waitpid(mod_pid, &status, 0) != mod_pid )
        fprintf(stderr, "Error killing modbus module\n");
    if( waitpid(server_pid, &status, 0) != server_pid )
        fprintf(stderr, "Error killing tag server\n");

    exit(exit_status);
}