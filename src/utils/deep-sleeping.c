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

#include "zenoh-pico/utils/deep-sleeping.h"

#include <esp_log.h>

RTC_DATA_ATTR static uint8_t local_resources[DIM_LOCAL_RESOURCES];
RTC_DATA_ATTR static uint8_t remote_resources[DIM_REMOTE_RESOURCES];

RTC_DATA_ATTR static uint8_t local_subscriptions[DIM_LOCAL_SUBSCRIPTIONS];
RTC_DATA_ATTR static uint8_t remote_subscriptions[DIM_REMOTE_SUBSCRIPTIONS];

RTC_DATA_ATTR static uint8_t local_questionable[DIM_LOCAL_QUESTIONABLE];

RTC_DATA_ATTR static uint8_t pending_replies[DIM_PENDING_REPLIES];

int zp_prepare_to_sleep(z_owned_session_t zs){
    _serialize_z_resource_list_t(zs._value->_local_resources, local_resources);
    _serialize_z_resource_list_t(zs._value->_remote_resources, remote_resources);

    //TODO check that serialize/deserialize for arg are not null if arg is not null
    memset(local_subscriptions, 0, DIM_LOCAL_SUBSCRIPTIONS);
    memset(remote_subscriptions, 0, DIM_REMOTE_SUBSCRIPTIONS);
    _serialize_z_subscription_sptr_list_t(zs._value->_local_subscriptions, _write_subscription_local, local_subscriptions);
    _serialize_z_subscription_sptr_list_t(zs._value->_remote_subscriptions, _write_subscription_remote, remote_subscriptions);

    memset(local_questionable, 0, DIM_LOCAL_QUESTIONABLE);
    _serialize_z_questionable_sptr_list_t(zs._value->_local_questionable, _write_questionable_local, local_questionable);

    return 0;
}

z_owned_session_t zp_wake_up(){
    z_owned_session_t zs = {._value = (_z_session_t *)z_malloc(sizeof(_z_session_t))};
    memset(zs._value, 0, sizeof(_z_session_t));

    if (zs._value != NULL) {
        zs._value->_local_resources = _deserialize_z_resource_list_t(local_resources);
        zs._value->_remote_resources = _deserialize_z_resource_list_t(remote_resources);

        //solves assert failed: block_trim_free tlsf.c:502 (block_is_free(block) && "block must be free") but zeros the buffer for deserialization
        // I'll keep the error above.
        // memset(local_subscriptions, 0, DIM_LOCAL_SUBSCRIPTIONS);
        // memset(remote_subscriptions, 0, DIM_REMOTE_SUBSCRIPTIONS);
        zs._value->_local_subscriptions = _deserialize_z_subscription_sptr_list_t(local_subscriptions);
        zs._value->_remote_subscriptions = _deserialize_z_subscription_sptr_list_t(remote_subscriptions);
    
        // memset(local_questionable, 0, DIM_LOCAL_QUESTIONABLE);
        zs._value->_local_questionable = _deserialize_z_questionable_sptr_list_t(local_questionable);
    }

    return zs;
}

int _serialize_z_resource_list_t(_z_resource_list_t *list, uint8_t *resources){  
    int ret = _Z_RES_OK;  
    _z_resource_t *element;
    size_t no_of_elements = _z_resource_list_len(list);

    uint8_t *_buffer = resources;

    //Serialization no_of_elements _id _key._id _key._mapping._val _key._suffix _refcount
    memcpy(_buffer, &no_of_elements, sizeof(size_t));
    _buffer += sizeof(size_t);

    _z_resource_list_t *iterate_list = list;
    while(!_z_resource_list_is_empty(iterate_list)){
        element = _z_resource_list_head(iterate_list);

        memcpy(_buffer, &element->_id, sizeof(size_t));
        _buffer += sizeof(size_t);

        memcpy(_buffer, &element->_key._id, sizeof(uint16_t));
        _buffer += sizeof(uint16_t);

        memcpy(_buffer, &element->_key._mapping._val, sizeof(uint16_t));
        _buffer += sizeof(uint16_t);

        strcpy((char *)_buffer, element->_key._suffix);
        _buffer += strlen(element->_key._suffix) + 1;

        memcpy(_buffer, &element->_refcount, sizeof(uint16_t));
        _buffer += sizeof(uint16_t);

        iterate_list = _z_resource_list_tail(iterate_list);
    }

    return ret;
}

_z_resource_list_t * _deserialize_z_resource_list_t(uint8_t *buffer){
    _z_resource_list_t *list = _z_resource_list_new();
    size_t no_of_elements;
    uint8_t * _buffer = buffer;

    //Deserialization no_of_elements _id _key._id _key._mapping._val _key._suffix _refcount
    memcpy(&no_of_elements, _buffer, sizeof(size_t));
    _buffer += sizeof(size_t);

    for(size_t i = 0; i < no_of_elements; i++){
        _z_resource_t *element = (_z_resource_t *)malloc(sizeof(_z_resource_t));
        
        memcpy(&element->_id, _buffer, sizeof(size_t));
        _buffer += sizeof(size_t);

        element->_key = *((_z_keyexpr_t *)malloc(sizeof(_z_keyexpr_t)));

        memcpy(&element->_key._id, _buffer, sizeof(uint16_t));
        _buffer += sizeof(uint16_t);

        memcpy(&element->_key._mapping._val, _buffer, sizeof(uint16_t));
        _buffer += sizeof(uint16_t);

        size_t len = strlen((char *)_buffer) + 1;

        element->_key._suffix = malloc(len);
        memcpy(element->_key._suffix, _buffer, len);
        _buffer += len;

        memcpy(&element->_refcount, _buffer, sizeof(uint16_t));
        _buffer += sizeof(uint16_t);

        list = _z_resource_list_push(list, element);
    }


    return list;
}

int8_t _write_subscription_local(void * writer, const char * serialized, int serialized_len){
    int8_t ret = 0;
    memcpy(local_subscriptions, &serialized_len, sizeof(serialized_len));
    memcpy(local_subscriptions, serialized, serialized_len);

    return ret;
}

int8_t _write_subscription_remote(void * writer, const char * serialized, int serialized_len){
    int8_t ret = 0;
    memcpy(remote_subscriptions, &serialized_len, sizeof(serialized_len));
    memcpy(remote_subscriptions, serialized, serialized_len);

    return ret;
}

int8_t _write_questionable_local(void * writer, const char * serialized, int serialized_len){
    int8_t ret = 0;
    memcpy(local_questionable, &serialized_len, sizeof(serialized_len));
    memcpy(local_questionable, serialized, serialized_len);

    return ret;
}

int _serialize_z_subscription_sptr_list_t(_z_subscription_sptr_list_t * list, int8_t (*write)(void *writer, const char *serialized, int serialized_len), uint8_t * subscriptions){
    int ret = _Z_RES_OK;
    _z_subscription_sptr_t *element;

    size_t no_of_elements = _z_subscription_sptr_list_len(list);

    uint8_t *_buffer = subscriptions;

    //Serialization
    memcpy(_buffer, &no_of_elements, sizeof(size_t));
    _buffer += sizeof(size_t);

    _z_subscription_sptr_list_t *iterate_list = list;
    while(!_z_subscription_sptr_list_is_empty(iterate_list))
    {
        element = _z_subscription_sptr_list_head(iterate_list);

        //_key._id _key._mapping._val _key._suffix _id _info.period.origin _info.period.period _info.period.duration reliability mode _callback _dropper serialize deserialize _arg_len _arg (the latter 2 by write functions)
        memcpy(_buffer, &element->ptr->_key._id, sizeof(uint16_t));
        _buffer += sizeof(uint16_t);

        memcpy(_buffer, &element->ptr->_key._mapping._val, sizeof(uint16_t));
        _buffer += sizeof(uint16_t);

        memcpy(_buffer, element->ptr->_key._suffix, strlen(element->ptr->_key._suffix) + 1);
        _buffer += strlen(element->ptr->_key._suffix) + 1;

        memcpy(_buffer, &element->ptr->_id, sizeof(uint32_t));
        _buffer += sizeof(uint32_t);

        memcpy(_buffer, &element->ptr->_info.period.origin, sizeof(unsigned int));
        _buffer += sizeof(unsigned int);

        memcpy(_buffer, &element->ptr->_info.period.period, sizeof(unsigned int));
        _buffer += sizeof(unsigned int);

        memcpy(_buffer, &element->ptr->_info.period.duration, sizeof(unsigned int));
        _buffer += sizeof(unsigned int);

        memcpy(_buffer, &element->ptr->_info.reliability, sizeof(z_reliability_t));
        _buffer += sizeof(z_reliability_t);

        memcpy(_buffer, &element->ptr->_info.mode, sizeof(z_submode_t));
        _buffer += sizeof(z_submode_t);

        memcpy(_buffer, &element->ptr->_callback, 4); //cannot be NULL
        _buffer += 4;

        if(element->ptr->_dropper != NULL) memcpy(_buffer, &element->ptr->_dropper, 4);
        _buffer += 4;

        if(element->ptr->serde_functions.serialize != NULL) memcpy(_buffer, &element->ptr->serde_functions.serialize, 4);
        _buffer += 4;

        if(element->ptr->serde_functions.deserialize != NULL) memcpy(_buffer, &element->ptr->serde_functions.deserialize, 4);
        _buffer += 4;

        if(element->ptr->_arg != NULL) element->ptr->serde_functions.serialize(write, NULL, element->ptr->_arg);
        _buffer += 4;

        iterate_list = _z_subscription_sptr_list_tail(iterate_list);
    }

    return ret;
}

_z_subscription_sptr_list_t * _deserialize_z_subscription_sptr_list_t(uint8_t * buffer){
    _z_subscription_sptr_list_t * list = _z_subscription_sptr_list_new();
    _z_subscription_sptr_t *element;
    size_t no_of_elements;
    uint8_t * _buffer = buffer;

    memcpy(&no_of_elements, _buffer, sizeof(size_t));
    _buffer += sizeof(size_t);

    for (size_t i = 0; i < no_of_elements; i++)
    {
        element = (_z_subscription_sptr_t *) malloc(sizeof(_z_subscription_sptr_t));
        memset(element->ptr, 0, sizeof(_z_subscription_t));

        //_key._id _key._mapping._val _key._suffix _id _info.period.origin _info.period.period _info.period.duration reliability mode _callback _dropper serialize deserialize _arg_len _arg
        element->ptr->_key = *((_z_keyexpr_t *)malloc(sizeof(_z_keyexpr_t)));
        memcpy(&element->ptr->_key._id, _buffer, sizeof(uint16_t));
        _buffer += sizeof(uint16_t);

        memcpy(&element->ptr->_key._mapping._val, _buffer, sizeof(uint16_t));
        _buffer += sizeof(uint16_t);

        element->ptr->_key._suffix = malloc(strlen((char *)_buffer) + 1);
        memcpy(element->ptr->_key._suffix, _buffer, strlen((char *)_buffer) + 1);
        _buffer += strlen(element->ptr->_key._suffix) + 1;

        memcpy(&element->ptr->_id, _buffer, sizeof(uint32_t));
        _buffer += sizeof(uint32_t);

        memcpy(&element->ptr->_info.period.origin, _buffer, sizeof(unsigned int));
        _buffer += sizeof(unsigned int);

        memcpy(&element->ptr->_info.period.period, _buffer, sizeof(unsigned int));
        _buffer += sizeof(unsigned int);

        memcpy(&element->ptr->_info.period.duration, _buffer, sizeof(unsigned int));
        _buffer += sizeof(unsigned int);

        memcpy(&element->ptr->_info.reliability, _buffer, sizeof(z_reliability_t));
        _buffer += sizeof(z_reliability_t);

        memcpy(&element->ptr->_info.mode, _buffer, sizeof(z_submode_t));
        _buffer += sizeof(z_submode_t);

        memcpy(&element->ptr->_callback, _buffer, 4); //cannot be NULL
        _buffer += 4;

        //_dropper serialize deserialize _arg_len _arg
        size_t tmp_maybe_address;
        memcpy(&tmp_maybe_address, _buffer, 4);
        if(tmp_maybe_address != 0) memcpy(&element->ptr->_dropper, _buffer, 4);
        _buffer += 4;

        memcpy(&tmp_maybe_address, _buffer, 4);
        if(tmp_maybe_address != 0) memcpy(&element->ptr->serde_functions.serialize, _buffer, 4);
        _buffer += 4;

        memcpy(&tmp_maybe_address, _buffer, 4);
        if(tmp_maybe_address != 0) memcpy(&element->ptr->serde_functions.deserialize, _buffer, 4);
        _buffer += 4;

        int arg_len;
        //deserialization of _arg
        if(element->ptr->serde_functions.deserialize != NULL){
            memcpy(&arg_len, _buffer, sizeof(int));
            _buffer += sizeof(int);
            element->ptr->serde_functions.deserialize((char *)_buffer, arg_len, NULL);
        }

        _z_subscription_sptr_list_push(list, element);
    }
    

    return list;
}

int _serialize_z_questionable_sptr_list_t(_z_questionable_sptr_list_t * list, int8_t (*write)(void *writer, const char *serialized, int serialized_len), uint8_t * questionable){
    int ret = _Z_RES_OK;
    _z_questionable_sptr_t *element;

    size_t no_of_elements = _z_questionable_sptr_list_len(list);
    uint8_t *_buffer = questionable;

    //Serialization
    memcpy(_buffer, &no_of_elements, sizeof(size_t));
    _buffer += sizeof(size_t);

    _z_subscription_sptr_list_t *iterate_list = list;
    while(!_z_questionable_sptr_list_is_empty(iterate_list))
    {
        element = _z_questionable_sptr_list_head(iterate_list);

        //_key._id _key._mapping._val _key._suffix _id _complete _callback _dropper serialize deserialize _arg_len _arg (the latter 2 by write functions)
        memcpy(_buffer, &element->ptr->_key._id, sizeof(uint16_t));
        _buffer += sizeof(uint16_t);

        memcpy(_buffer, &element->ptr->_key._mapping._val, sizeof(uint16_t));
        _buffer += sizeof(uint16_t);

        memcpy(_buffer, element->ptr->_key._suffix, strlen(element->ptr->_key._suffix) + 1);
        _buffer += strlen(element->ptr->_key._suffix) + 1;

        memcpy(_buffer, &element->ptr->_id, sizeof(uint32_t));
        _buffer += sizeof(uint32_t);

        memcpy(_buffer, &element->ptr->_complete, sizeof(_Bool));
        _buffer += sizeof(_Bool);

        memcpy(_buffer, &element->ptr->_callback, 4); //cannot be NULL
        _buffer += 4;

        if(element->ptr->_dropper != NULL) memcpy(_buffer, &element->ptr->_dropper, 4);
        _buffer += 4;

        if(element->ptr->serde_functions.serialize != NULL) memcpy(_buffer, &element->ptr->serde_functions.serialize, 4);
        _buffer += 4;

        if(element->ptr->serde_functions.deserialize != NULL) memcpy(_buffer, &element->ptr->serde_functions.deserialize, 4);
        _buffer += 4;

        if(element->ptr->_arg != NULL) element->ptr->serde_functions.serialize(write, NULL, element->ptr->_arg);
        _buffer += 4;

        iterate_list = _z_questionable_sptr_list_tail(iterate_list);
    }
    return ret;
}

_z_questionable_sptr_list_t * _deserialize_z_questionable_sptr_list_t(uint8_t * buffer){
    _z_questionable_sptr_list_t * list = _z_questionable_sptr_list_new();
    _z_questionable_sptr_t *element;
    size_t no_of_elements;
    uint8_t * _buffer = buffer;

    memcpy(&no_of_elements, _buffer, sizeof(size_t));
    _buffer += sizeof(size_t);

    for (size_t i = 0; i < no_of_elements; i++)
    {
        element = (_z_questionable_sptr_t *) malloc(sizeof(_z_questionable_sptr_t));
        memset(element->ptr, 0, sizeof(_z_questionable_t));

        //_key._id _key._mapping._val _key._suffix _id _complete _callback _dropper serialize deserialize _arg_len _arg
        element->ptr->_key = *((_z_keyexpr_t *)malloc(sizeof(_z_keyexpr_t)));
        memcpy(&element->ptr->_key._id, _buffer, sizeof(uint16_t));
        _buffer += sizeof(uint16_t);

        memcpy(&element->ptr->_key._mapping._val, _buffer, sizeof(uint16_t));
        _buffer += sizeof(uint16_t);

        element->ptr->_key._suffix = malloc(strlen((char *)_buffer) + 1);
        memcpy(element->ptr->_key._suffix, _buffer, strlen((char *)_buffer) + 1);
        _buffer += strlen(element->ptr->_key._suffix) + 1;

        memcpy(&element->ptr->_id, _buffer, sizeof(uint32_t));
        _buffer += sizeof(uint32_t);

        memcpy(&element->ptr->_complete, _buffer, sizeof(_Bool));
        _buffer += sizeof(_Bool);

        memcpy(&element->ptr->_callback, _buffer, 4); //cannot be NULL
        _buffer += 4;

        //_dropper serialize deserialize _arg_len _arg
        size_t tmp_maybe_address;
        memcpy(&tmp_maybe_address, _buffer, 4);
        if(tmp_maybe_address != 0) memcpy(&element->ptr->_dropper, _buffer, 4);
        _buffer += 4;

        memcpy(&tmp_maybe_address, _buffer, 4);
        if(tmp_maybe_address != 0) memcpy(&element->ptr->serde_functions.serialize, _buffer, 4);
        _buffer += 4;

        memcpy(&tmp_maybe_address, _buffer, 4);
        if(tmp_maybe_address != 0) memcpy(&element->ptr->serde_functions.deserialize, _buffer, 4);
        _buffer += 4;

        int arg_len;
        //deserialization of _arg
        if(element->ptr->serde_functions.deserialize != NULL){
            memcpy(&arg_len, _buffer, sizeof(int));
            _buffer += sizeof(int);
            element->ptr->serde_functions.deserialize((char *)_buffer, arg_len, NULL);
        }

        _z_questionable_sptr_list_push(list, element);
    }

    return list;
}

int _serialize_z_pending_reply_list_t(_z_pending_reply_list_t * list, uint8_t * pending_replies){
    int ret = _Z_RES_OK;
    _z_pending_reply_t *element;

    size_t no_of_elements = _z_pending_reply_list_len(list);
    uint8_t *_buffer = pending_replies;

    //Serialization
    memcpy(_buffer, &no_of_elements, sizeof(size_t));
    _buffer += sizeof(size_t);

    _z_pending_reply_list_t *iterate_list = list;
    while(!_z_pending_reply_list_is_empty(iterate_list)){
        element = _z_pending_reply_list_head(iterate_list);

        // typedef struct {
        //     _z_reply_t _reply;
        //     _z_timestamp_t _tstamp;
        // } _z_pending_reply_t;
        
        memcpy(_buffer, element->_tstamp.id.id, 16*sizeof(uint8_t));
        _buffer += 16*sizeof(uint8_t);

        memcpy(_buffer, &element->_tstamp.time, sizeof(uint64_t));
        _buffer += sizeof(uint64_t);

        // typedef struct {
        //     _z_reply_data_t data;
        //     z_reply_tag_t _tag;
        // } _z_reply_t;

        memcpy(_buffer, &element->_reply._tag, sizeof(z_reply_tag_t));
        _buffer += sizeof(z_reply_tag_t);

        // typedef struct {
        //     _z_sample_t sample;
        //     _z_id_t replier_id;
        // } _z_reply_data_t;

        memcpy(_buffer, element->_reply.data.replier_id.id, 16*sizeof(uint8_t));
        _buffer += 16*sizeof(uint8_t);

        // typedef struct {
        //     _z_keyexpr_t keyexpr;
        //     _z_bytes_t payload;
        //     _z_timestamp_t timestamp;
        //     _z_encoding_t encoding;
        //     z_sample_kind_t kind;
        // } _z_sample_t;
        _z_sample_t sample = element->_reply.data.sample;
        memcpy(_buffer, &sample.keyexpr._id, sizeof(uint16_t));
        _buffer += sizeof(uint16_t);

        memcpy(_buffer, &sample.keyexpr._mapping._val, sizeof(uint16_t));
        _buffer += sizeof(uint16_t);

        memcpy(_buffer, sample.keyexpr._suffix, strlen(sample.keyexpr._suffix) + 1);
        _buffer += strlen(sample.keyexpr._suffix) + 1;

        memcpy(_buffer, &sample.payload._is_alloc, sizeof(_Bool));
        _buffer += sizeof(_Bool);

        memcpy(_buffer, &sample.payload.len, sizeof(size_t));
        _buffer += sizeof(size_t);

        memcpy(_buffer, sample.payload.start, sample.payload.len);
        _buffer += sample.payload.len;

        memcpy(_buffer, sample.timestamp.id.id, 16*sizeof(uint8_t));
        _buffer += 16*sizeof(uint8_t);

        memcpy(_buffer, &sample.timestamp.time, sizeof(uint64_t));
        _buffer += sizeof(uint64_t);

        memcpy(_buffer, &sample.encoding.suffix._is_alloc, sizeof(_Bool));
        _buffer += sizeof(_Bool);

        memcpy(_buffer, &sample.encoding.suffix.len, sizeof(size_t));
        _buffer += sizeof(size_t);

        memcpy(_buffer, sample.encoding.suffix.start, sample.encoding.suffix.len);
        _buffer += sample.encoding.suffix.len;

        memcpy(_buffer, &sample.encoding.prefix, sizeof(z_encoding_prefix_t));
        _buffer += sizeof(z_encoding_prefix_t);

        memcpy(_buffer, &sample.kind, sizeof(z_sample_kind_t));
        _buffer += sizeof(z_sample_kind_t);
    }

    return ret;
}