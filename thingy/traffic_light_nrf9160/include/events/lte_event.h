/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _LTE_EVENT_H_
#define _LTE_EVENT_H_

/**
 * @brief Peer Connection Event
 * @defgroup peer_conn_event Peer Connection Event
 * @{
 */

#include <string.h>
#include <zephyr/toolchain/common.h>

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

#ifdef __cplusplus
extern "C" {
#endif


enum lte_conn_state {
	LTE_CONNECTED,
	LTE_DISCONNECTED
};

/** Peer connection event. */
struct lte_event {
	struct app_event_header header;

	enum lte_conn_state conn_state;
};

APP_EVENT_TYPE_DECLARE(lte_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _PEER_CONN_EVENT_H_ */
