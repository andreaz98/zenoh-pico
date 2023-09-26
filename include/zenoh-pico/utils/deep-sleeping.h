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

/// @brief Prepares the board to go to sleep.
/// @param session the session to be saved.
/// @return 0 - session has been correctly processed, -1 otherwise.
int zp_prepare_to_sleep(z_owned_session_t session);

/// @brief Prepares the session after a board wake up.
/// @return The session.
z_owned_session_t zp_wake_up();

size_t _serialize_z_resource_list_t(_z_resource_list_t *list, uint8_t *resources);
_z_resource_list_t * _deserialize_z_resource_list_t(uint8_t *buffer);

uint8_t * _serialize_zn_subscriber_list_t(_z_subscription_sptr_list_t * list, size_t *len);