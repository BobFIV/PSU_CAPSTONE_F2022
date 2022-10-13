/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/pm/device.h>

#define MODULE at_handler
#include "events/module_state_event.h"
#include "events/uart_data_event.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE);

#define AT_PARSE_BUFFER_SIZE 256
char at_parse_buf[AT_PARSE_BUFFER_SIZE];
bool at_started = false;
bool at_ended = false;
size_t parse_buf_idx = 0; // the location in the cmd_parse_buff that we are writing to

static bool app_event_handler(const struct app_event_header *aeh)
{
	int err;

	if (is_uart_data_event(aeh)) {
		const struct uart_data_event *event =
			cast_uart_data_event(aeh);
		if (event->dev_idx == 0) {
            // This is coming from the PC/upper tester
            LOG_INF("UART0 data event: %c", event->buf);
			// TODO: Parse AT commands.
        }

		return false;
	}

	if (is_module_state_event(aeh)) {
		const struct module_state_event *event =
			cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			// Setup stuff goes in here if we need it
            LOG_INF("at handler setup");
		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
APP_EVENT_SUBSCRIBE(MODULE, uart_data_event);