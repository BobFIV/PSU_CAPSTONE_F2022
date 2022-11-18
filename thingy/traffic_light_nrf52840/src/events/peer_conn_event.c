/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <assert.h>

#include "events/peer_conn_event.h"


static void log_peer_conn_event(const struct app_event_header *aeh)
{
	const struct peer_conn_event *event = cast_peer_conn_event(aeh);


	APP_EVENT_MANAGER_LOG(aeh,
		"%s:_%d baud:%d",
		event->conn_state == PEER_STATE_CONNECTED ?
			"CONNECTED" : "DISCONNECTED",
		event->dev_idx,
		event->baudrate);
}

APP_EVENT_TYPE_DEFINE(peer_conn_event,
		  log_peer_conn_event,
		  NULL,
		  APP_EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_LOG_PEER_CONN_EVENT,
				(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));
