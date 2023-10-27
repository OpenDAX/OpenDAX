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

/*
 *  This file is a replacement for the Paho library for testing.  We build a
 *  test-only mqtt module that is linked with this file instead of with the Paho
 *  library so that we don't have to have a broker installed and so we can
 *  intercept and manipulate the messages.
 *  The idea is that we start a thread on connect and keep track of how many
 *  subscriptions happen
 */


#include "mqtt_test.h"
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

/*  Callback functions */
static MQTTClient_connectionLost *_cl;
static MQTTClient_messageArrived *_ma;
static MQTTClient_deliveryComplete *_dc;
static void *_context;
static pthread_t _test_thread;

static thread_args_t thread_args;


LIBMQTT_API void
MQTTClient_freeMessage(MQTTClient_message** msg) {
    ;
}

LIBMQTT_API void
MQTTClient_free(void* ptr) {
    ;
}

LIBMQTT_API int
MQTTClient_subscribe(MQTTClient handle, const char* topic, int qos) {
    DF("Subscribe to %s", topic);
    pthread_mutex_lock(&thread_args.sub_lock);
    thread_args.subscriptions++;
    pthread_cond_signal(&thread_args.sub_cond);
    pthread_mutex_unlock(&thread_args.sub_lock);
    return MQTTCLIENT_SUCCESS;
}

LIBMQTT_API int
MQTTClient_publish(MQTTClient handle, const char* topicName, int payloadlen, const void* payload, int qos, int retained, MQTTClient_deliveryToken* dt) {

    return MQTTCLIENT_SUCCESS;
}

LIBMQTT_API int
test_publish(MQTTClient handle, const char* topicName, int payloadlen, const void* payload, int qos, int retained, MQTTClient_deliveryToken* dt) {
    MQTTClient_message msg;
    msg.struct_id[0] = 'M';
    msg.struct_id[1] = 'Q';
    msg.struct_id[2] = 'T';
    msg.struct_id[3] = 'M';
    msg.struct_version = 0;
    msg.payloadlen = payloadlen;
    msg.payload = (void *)payload;
    msg.qos = qos;
    msg.retained = retained;
    _ma(_context, (char *)topicName, strlen(topicName), &msg);
    return MQTTCLIENT_SUCCESS;
}


LIBMQTT_API int
MQTTClient_create(MQTTClient* handle, const char* serverURI, const char* clientId, int persistence_type, void* persistence_context) {
   return MQTTCLIENT_SUCCESS;
}

LIBMQTT_API int
MQTTClient_setCallbacks(MQTTClient handle, void* context, MQTTClient_connectionLost* cl,
						MQTTClient_messageArrived* ma, MQTTClient_deliveryComplete* dc) {
    _cl = cl;
    _ma = ma;
    _dc = dc;
    _context = context;
    return MQTTCLIENT_SUCCESS;
}


LIBMQTT_API int
MQTTClient_connect(MQTTClient handle, MQTTClient_connectOptions* options) {
    int result;

    result = pthread_create(&_test_thread, NULL, test_thread, &thread_args);
    if(result) {
        dax_log(DAX_LOG_ERROR, "Unable to create thread");
    }
    return MQTTCLIENT_SUCCESS;
}


LIBMQTT_API int MQTTClient_disconnect(MQTTClient handle, int timeout) {
    int status;

    kill(getpid(), SIGQUIT);

    return MQTTCLIENT_SUCCESS;
}


LIBMQTT_API void
MQTTClient_destroy(MQTTClient* handle) {
    ;
}
