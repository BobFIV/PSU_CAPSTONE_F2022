/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _PEER_CONN_EVENT_H_
#define _PEER_CONN_EVENT_H_

/**
 * @brief Peer Connection Event
 * @defgroup peer_conn_event Peer Connection Event
 * @{
 */

#include <string.h>
#include <zephyr/toolchain/common.h>

#include <app_event_manager.h>

#ifdef __cplusplus
extern "C" {
#endif

enum peer_conn_state {
	PEER_STATE_CONNECTED,
	PEER_STATE_DISCONNECTED
};

/** Peer connection event. */
struct peer_conn_event {
	struct app_event_header header;
	enum peer_conn_state conn_state;
};

APP_EVENT_TYPE_DECLARE(peer_conn_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _PEER_CONN_EVENT_H_ */
