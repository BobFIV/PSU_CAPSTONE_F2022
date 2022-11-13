/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _BLE_EVENT_H_
#define _BLE_EVENT_H_

/**
 * @brief BLE Connection Event
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

enum ble_cmd {
	BLE_CONNECTED,
	BLE_DISCONNECTED,
	BLE_SCAN_STARTED,
	BLE_SCAN_STOPPED
};

/** Peer connection event. */
struct ble_event {
	struct app_event_header header;

	enum ble_cmd cmd;
};

APP_EVENT_TYPE_DECLARE(ble_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _BLE_EVENT_H_ */
