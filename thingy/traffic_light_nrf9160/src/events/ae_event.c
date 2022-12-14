/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "events/ae_event.h"

enum ae_light_states string_to_light_state(char* str, size_t max_length) {
	if (strncmp(str, "green", max_length) == 0) {
		return AE_LIGHT_GREEN;
	}
	if (strncmp(str, "yellow", max_length) == 0) {
		return AE_LIGHT_YELLOW;
	}
	if (strncmp(str, "red", max_length) == 0) {
		return AE_LIGHT_RED;
	}
	if (strncmp(str, "off", max_length) == 0) {
		return AE_LIGHT_OFF;
	}
	return AE_LIGHT_STATE_NONE;
}

void light_state_to_string(enum ae_light_states state, char* output) {
	switch(state) {
		case AE_LIGHT_GREEN:
			strcpy(output, "green");
		break;
		case AE_LIGHT_YELLOW:
			strcpy(output, "yellow");
		break;
		case AE_LIGHT_RED:
			strcpy(output, "red");
		break;
		case AE_LIGHT_OFF:
			strcpy(output, "off");
		break;
		case AE_LIGHT_STATE_NONE:
		default:
			strcpy(output, "none");
		break;
	}
}

static void log_ae_event(const struct app_event_header *aeh)
{
	/*
	const struct ae_event *event = cast_ae_event(aeh);

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
	*/
	ARG_UNUSED(aeh);
}

APP_EVENT_TYPE_DEFINE(ae_event,
		  log_ae_event,
		  NULL,
		  APP_EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_LOG_PEER_CONN_EVENT,
				(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));
