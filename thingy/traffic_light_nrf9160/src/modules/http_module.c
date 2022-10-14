/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>
#include <zephyr/sys/ring_buffer.h>

#define MODULE http_module
#include <caf/events/module_state_event.h>
#include "events/lte_event.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE);

void web_poll() {
	while(true) {
		k_sleep(K_SECONDS(1));
		// TODO: Poll the ACME CSE
	}
}

static bool app_event_handler(const struct app_event_header *aeh)
{

	if (is_module_state_event(aeh)) {
		const struct module_state_event *event =
			cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			LOG_INF("HTTP module setup");
		}

		return false;
	}

	if (is_lte_event(aeh)) {
		const struct lte_event *event =
			cast_lte_event(aeh);
		if (event->conn_state == LTE_CONNECTED) {
			LOG_INF("Got LTE_CONNECTED");
        }
		else if (event->conn_state == LTE_DISCONNECTED) {
			LOG_INF("Got LTE_DISCONNECTED");
        }

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
APP_EVENT_SUBSCRIBE(MODULE, lte_state_event);

K_THREAD_DEFINE(web_poll_thread, 4096, web_poll, NULL, NULL, NULL, 10, 0, 0);