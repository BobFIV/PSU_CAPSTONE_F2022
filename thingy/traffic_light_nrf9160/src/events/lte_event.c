/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <assert.h>

#include "events/lte_event.h"

static void log_lte_event(const struct app_event_header *aeh)
{
	const struct lte_event *event = cast_lte_event(aeh);

	APP_EVENT_MANAGER_LOG(aeh,
		"LTE Event: %s",
		event->conn_state == LTE_CONNECTED ?
			"CONNECTED" : "DISCONNECTED");
}

APP_EVENT_TYPE_DEFINE(lte_event,
		  log_lte_event,
		  NULL,
		  APP_EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_LOG_PEER_CONN_EVENT,
				(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));
