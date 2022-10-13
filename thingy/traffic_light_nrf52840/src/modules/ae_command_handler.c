/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/pm/device.h>

#define MODULE ae_command_handler
#include "events/module_state_event.h"
#include "events/ae_command_event.h"
#include "events/uart_data_event.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE);

#define CMD_PARSE_BUFFER_SIZE 30
char cmd_parse_buf[30];

/*
    This module parses UART event data from the uart_handler and turns it into an ae_command_event.
    Although this could be purely implemented in the uart_handler, I believe it is best to implement
    this functionality in a separate handler to reduce complexity.
*/

static bool app_event_handler(const struct app_event_header *aeh)
{
	int err;

    if (is_ae_command_event(aeh)) {
		const struct ae_command_event *event =
			cast_ae_command_event(aeh);
        // Deallocate anything we dynamically allocated for the event.
        // As of Oct. 12, 2022 we do not dynamically allocate anything.

		return true;
	}

	if (is_uart_data_event(aeh)) {
		const struct uart_data_event *event =
			cast_uart_data_event(aeh);

		// We got characters from the UART connection
        if (event->len > 0) {
            bool cmd_started = false;
            bool cmd_ended = false;
            // Zero out the parsing buffer
            memset(&cmd_parse_buf[0],0,CMD_PARSE_BUFFER_SIZE);
            size_t j = 0; // j is the location in the cmd_parse_buff that we are writing to
            for (size_t i = 0; i < event->len; i++) {
                if (!cmd_started && event->buf[i] == '!') {
                    // We found the start of a command
                    cmd_started = true;
                    continue;
                }
                else if (cmd_started && event->buf[i] == ';') {
                    // We found the end of a command
                    cmd_ended = true;
                    break;
                }
                else if (cmd_started) {
                    // copy character to the parse buffer
                    cmd_parse_buf[j] = event->buf[i];
                    j++;
                }
                else {
                    // Command not started yet, continue
                    continue;
                }

                if (j >= CMD_PARSE_BUFFER_SIZE) {
                    LOG_WRN("parse buffer overrun!");
                    break;
                }
            }

            if (!cmd_started) {
                LOG_WRN("cmd never started!");
            }
            if (!cmd_ended) {
                LOG_WRN("cmd never ended!");
            }

            if (cmd_started && cmd_ended) {
                if (j == 0) {
                    // We parsed nothing!
                    LOG_WRN("parse buff j==0");
                }
                else {
                    LOG_INF("parsed: %s", cmd_parse_buf);
                    bool valid_cmd = false;
                    enum ae_commands cmd;
                    enum light_states state;

                    if (strncmp(&cmd_parse_buf[0], "green1", CMD_PARSE_BUFFER_SIZE) == 0) {
                        cmd = SET_STATE1;
                        state = LIGHT_GREEN;
                        valid_cmd = true;
                    }
                    else if (strncmp(&cmd_parse_buf[0], "yellow1", CMD_PARSE_BUFFER_SIZE) == 0) {
                        state = LIGHT_YELLOW;
                        valid_cmd = true;
                    }
                    else if (strncmp(&cmd_parse_buf[0], "red1", CMD_PARSE_BUFFER_SIZE) == 0) {
                        cmd = SET_STATE1;
                        state = LIGHT_RED;
                        valid_cmd = true;
                    }
                    else if (strncmp(&cmd_parse_buf[0], "off1", CMD_PARSE_BUFFER_SIZE) == 0) {
                        cmd = SET_STATE1;
                        state = LIGHT_OFF;
                        valid_cmd = true;
                    }
                    else if (strncmp(&cmd_parse_buf[0], "cycle1", CMD_PARSE_BUFFER_SIZE) == 0) {
                        cmd = SET_STATE1;
                        state = LIGHT_CYCLE;
                        valid_cmd = true;
                    }
                    else if (strncmp(&cmd_parse_buf[0], "green2", CMD_PARSE_BUFFER_SIZE) == 0) {
                        cmd = SET_STATE2;
                        state = LIGHT_GREEN;
                        valid_cmd = true;
                    }
                    else if (strncmp(&cmd_parse_buf[0], "yellow2", CMD_PARSE_BUFFER_SIZE) == 0) {
                        cmd = SET_STATE2;
                        state = LIGHT_YELLOW;
                        valid_cmd = true;
                    }
                    else if (strncmp(&cmd_parse_buf[0], "red2", CMD_PARSE_BUFFER_SIZE) == 0) {
                        cmd = SET_STATE2;
                        state = LIGHT_RED;
                        valid_cmd = true;
                    }
                    else if (strncmp(&cmd_parse_buf[0], "off2", CMD_PARSE_BUFFER_SIZE) == 0) {
                        cmd = SET_STATE2;
                        state = LIGHT_OFF;
                        valid_cmd = true;
                    }
                    else if (strncmp(&cmd_parse_buf[0], "cycle2", CMD_PARSE_BUFFER_SIZE) == 0) {
                        cmd = SET_STATE2;
                        state = LIGHT_CYCLE;
                        valid_cmd = true;
                    }

                    if (valid_cmd) {
                        struct ae_command_event *event;
                        event = new_ae_command_event();
                        event->cmd = cmd;
                        event->light_state = state;
                        APP_EVENT_SUBMIT(event);
                    }
                    else {
                        LOG_WRN("Invalid AE cmd string! No event produced.");
                    }
                }
            }
            
        }

		return false;
	}

	if (is_module_state_event(aeh)) {
		const struct module_state_event *event =
			cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
            // At startup, all lights should be turned to RED.
            // TODO: Produce events for this.
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
APP_EVENT_SUBSCRIBE_FINAL(MODULE, ae_command_event);