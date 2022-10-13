/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>

#include <modem/nrf_modem_lib.h>
#include <modem/lte_lc.h>
#include <modem/at_cmd.h>
#include <modem/at_notif.h>

#define MODULE lte_module
#include "events/module_state_event.h"
#include "events/uart_data_event.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE);

static bool app_event_handler(const struct app_event_header *aeh)
{
	int err;

	if (is_module_state_event(aeh)) {
		const struct module_state_event *event =
			cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			int err;
			err = nrf_modem_lib_init(NORMAL_MODE);
			if (err)
			{
				LOG_ERR("Failed to initialize modem library! %d", err);
				return false;
			}
			/* Initialize AT comms in order to set up LTE */
			err = at_comms_init();
			if (err)
			{
				LOG_ERR("Error at_comms_init: %d", err);
				return false;
			}
			LOG_INF("Waiting for network.. ");
			err = lte_lc_init_and_connect();
			if (err)
			{
				LOG_ERR("Failed to connect to the LTE network: %d", err);
				return false;
			}
		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);

K_THREAD_DEFINE(web_poll_thread, 4096, web_poll, NULL, NULL, NULL, 10, 0, 0);