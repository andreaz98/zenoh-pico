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

#define DIM_LOCAL_RESOURCES 512
#define DIM_REMOTE_RESOURCES 512

RTC_DATA_ATTR static uint8_t local_resources[DIM_LOCAL_RESOURCES];
RTC_DATA_ATTR static uint8_t remote_resources[DIM_REMOTE_RESOURCES];

int zp_prepare_to_sleep(z_owned_session_t zs){
    _serialize_z_resource_list_t(zs._value->_local_resources, local_resources);
    _serialize_z_resource_list_t(zs._value->_remote_resources, remote_resources);

    return 0;
}

z_owned_session_t zp_wake_up(){
    z_owned_session_t zs = {._value = (_z_session_t *)z_malloc(sizeof(_z_session_t))};
    memset(zs._value, 0, sizeof(_z_session_t));

    if (zs._value != NULL) {
        zs._value->_local_resources = _deserialize_z_resource_list_t(local_resources);
        zs._value->_remote_resources = _deserialize_z_resource_list_t(remote_resources);
    }

    return zs;
}

//size_t dimension_resource_list;
//uint8_t * _serialize_z_resource_list_t(_z_resource_list_t *list, size_t *size){
size_t _serialize_z_resource_list_t(_z_resource_list_t *list, uint8_t *resources){    
    _z_resource_t *element;
    
    size_t no_of_elements = _z_resource_list_len(list);
    uint dim_rnames = 0;

    _z_resource_list_t *calc_list = list;

    while(!_z_resource_list_is_empty(calc_list)){
        element = _z_resource_list_head(calc_list);

        dim_rnames += strlen(element->_key._suffix) + 1;

        calc_list = _z_resource_list_tail(calc_list);
    }

    size_t dimension = sizeof(size_t) + no_of_elements * (sizeof(size_t) + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint16_t)) + dim_rnames;
    
    //uint8_t *buffer = (uint8_t *)malloc(dimension);
    uint8_t *_buffer = resources;
    //*size = dimension;

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

    return dimension;
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


// uint8_t * _serialize_zn_subscriber_list_t(_z_subscription_sptr_list_t * list, size_t *len){
//     _z_subscription_sptr_t *element;
    
//     size_t no_of_elements = _z_subscription_sptr_list_len(list);
//     uint dim_rnames = 0;

//     _z_subscription_sptr_list_t *calc_list = list;

//     _z_subscription_sptr_list_head
//     while(!_zn_subscriber_list_is_empty(calc_list)){
//         element = _z_subscription_sptr_list_head(calc_list);

//         if(element->rname != NULL) dim_rnames += strlen(element->rname) + 1;
        
//         if(element->key.rname != NULL) dim_rnames += strlen(element->key.rname) + 1;

//         calc_list = _zn_subscriber_list_tail(calc_list);
//     }

//     size_t dimension = sizeof(size_t) + no_of_elements * (sizeof(size_t) + sizeof(unsigned long) + sizeof(zn_subinfo_t) + sizeof(zn_reliability_t) + sizeof(zn_submode_t) + sizeof(zn_period_t)) + dim_rnames;
//     uint8_t *buffer = (uint8_t *)malloc(dimension);
//     uint8_t *_buffer = buffer;
//     *len = dimension;

//     //Serialization
//     memcpy(_buffer, &no_of_elements, sizeof(size_t));
//     _buffer += sizeof(size_t);

//     _zn_subscriber_list_t *iterate_list = list;
//     while(!_zn_subscriber_list_is_empty(iterate_list))
//     {
//         element = _zn_subscriber_list_head(iterate_list);

//         memcpy(_buffer, &element->id, sizeof(size_t));
//         _buffer += sizeof(size_t);

//         strcpy((char *)_buffer, element->rname);
//         _buffer += strlen(element->rname) + 1;

//         memcpy(_buffer, &element->key.rid, sizeof(unsigned long));
//         _buffer += sizeof(unsigned long);

//         if(element->key.rname != NULL){
//             strcpy((char *)_buffer, element->key.rname);
//             _buffer += strlen(element->key.rname) + 1;
//         }
//         else{// how to manage this change in the deserialization?
//             strcpy((char *)_buffer, "0");
//             _buffer += 2;
//         }
        
//         memcpy(_buffer, &element->info.reliability, sizeof(zn_reliability_t));
//         _buffer += sizeof(zn_reliability_t);

//         memcpy(_buffer, &element->info.mode, sizeof(zn_submode_t));
//         _buffer += sizeof(zn_submode_t);

//         memcpy(_buffer, &element->info.period, sizeof(zn_period_t));
//         _buffer += sizeof(zn_period_t);

//         iterate_list = _zn_subscriber_list_tail(iterate_list);
//     }

//     return buffer;
// }