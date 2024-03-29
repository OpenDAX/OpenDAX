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

#include <opendax.h>
#include <pthread.h>
#include <common.h>
#include <MQTTClient.h>
#include "../../modtest_common.h"

typedef struct {
    pthread_mutex_t sub_lock;
    pthread_cond_t sub_cond;
    int subscriptions;
} thread_args_t;

LIBMQTT_API int test_publish(MQTTClient handle, const char* topicName, int payloadlen, const void* payload, int qos, int retained, MQTTClient_deliveryToken* dt);
void *test_thread(void *arg);