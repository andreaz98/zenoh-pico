//
// Copyright (c) 2023 ZettaScale Technology
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
// which is available at https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
//
// Contributors:
//   ZettaScale Zenoh Team, <zenoh@zettascale.tech>
//   Andrea Zanni, <andrea.zanni8@studio.unibo.it>
//

#include "zenoh-pico/collections/element.h"
#include "zenoh-pico/collections/list.h"
#include "zenoh-pico/session/utils.h"

#include "zenoh-pico/api/types.h"
#include <netdb.h>

// These are the dimensions for the arrays in the RTC Slow RAM that will
// contain your serialized structs while in deep sleep/hibernation mode.
// Feel free to re-dimension these values accordingly to the purpose of
// your application. Remember, those values must be at least 4 as it is
// the size of a size_t which represents the number of elements in a list.
// The sum of these dimensions must not exceed 8 KiB.

#define DIM_LOCAL_RESOURCES 512
#define DIM_REMOTE_RESOURCES 512

#define DIM_LOCAL_SUBSCRIPTIONS 512
#define DIM_REMOTE_SUBSCRIPTIONS 512

#define DIM_LOCAL_QUESTIONABLE 512

#define DIM_PENDING_QUERIES 1024

#define DIM_TRANSPORT 512

int zp_prepare_to_sleep(z_owned_session_t session);
z_owned_session_t zp_wake_up();

int _serialize_z_resource_list_t(_z_resource_list_t *list, uint8_t *resources);
_z_resource_list_t * _deserialize_z_resource_list_t(uint8_t *buffer);

int _serialize_z_subscription_sptr_list_t(_z_subscription_sptr_list_t * list, int8_t (*write)(void *writer, const char *serialized, int serialized_len), uint8_t * subscriptions);
_z_subscription_sptr_list_t * _deserialize_z_subscription_sptr_list_t(uint8_t * buffer);

int8_t _write_subscription_local(void * writer, const char * serialized, int serialized_len);
int8_t _write_subscription_remote(void * writer, const char * serialized, int serialized_len);
int8_t _write_questionable_local(void * writer, const char * serialized, int serialized_len);

int8_t _write_call_arg(void * writer, const char * serialized, int serialized_len);
int8_t _write_drop_arg(void * writer, const char * serialized, int serialized_len);

int _serialize_z_questionable_sptr_list_t(_z_questionable_sptr_list_t * list, int8_t (*write)(void *writer, const char *serialized, int serialized_len), uint8_t * questionable);
_z_questionable_sptr_list_t * _deserialize_z_questionable_sptr_list_t(uint8_t * buffer);

int _serialize_z_pending_reply_list_t(_z_pending_reply_list_t * list, uint8_t ** pending_replies);
_z_pending_reply_list_t * _deserialize_z_pending_reply_list_t(uint8_t **buffer);

int _serialize_z_pending_query_list_t(_z_pending_query_list_t *list, int8_t (*write_call_arg)(void *writer, const char *serialized, int serialized_len), int8_t (*write_drop_arg)(void *writer, const char *serialized, int serialized_len), uint8_t *pending_queries);
_z_pending_query_list_t * _deserialize_z_pending_query_list_t(uint8_t *buffer);

int _serialize_z_transport_t(_z_transport_t tp, uint8_t *transport);
_z_transport_t _deserialize_z_transport_t(uint8_t *buffer);