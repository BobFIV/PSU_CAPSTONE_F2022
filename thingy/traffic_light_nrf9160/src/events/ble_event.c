/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <assert.h>

#include "events/ble_event.h"

static void log_ble_event(const struct app_event_header *aeh)
{
	const struct ble_event *event = cast_ble_event(aeh);

	switch(event->cmd) {
		case BLE_CONNECTED:
		APP_EVENT_MANAGER_LOG(aeh, "BLE Event: CONNECTED");
		break;
		case BLE_DISCONNECTED:
		APP_EVENT_MANAGER_LOG(aeh, "BLE Event: DISCONNECTED");
		break;
		case BLE_SCAN_STARTED:
		APP_EVENT_MANAGER_LOG(aeh, "BLE Event: SCAN_STARTED");
		break;
		case BLE_SCAN_STOPPED:
		APP_EVENT_MANAGER_LOG(aeh, "BLE Event: SCAN_STOPPED");
		break;
	}

}

APP_EVENT_TYPE_DEFINE(ble_event,
		  log_ble_event,
		  NULL,
		  APP_EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_LOG_PEER_CONN_EVENT,
				(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));
