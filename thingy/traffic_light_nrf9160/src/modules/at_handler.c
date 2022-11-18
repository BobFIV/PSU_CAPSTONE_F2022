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
#include <caf/events/module_state_event.h>
#include "events/uart_data_event.h"
#include "events/ae_event.h"
#include "onem2m.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE);

#define AT_PARSE_BUFFER_SIZE 256
bool in_test_mode = false;
char at_parse_buf[AT_PARSE_BUFFER_SIZE];
bool at_started = false;
bool at_ended = false;
size_t parse_buf_idx = 0; // the location in the cmd_parse_buff that we are writing to

void parse_at_command(char c) {

	//Message has been started, but we are not at the end
	if (at_started && c != ';') {
		
		//Then add to our current mesasge
		at_parse_buf[parse_buf_idx] = c;
		parse_buf_idx++;
	
	//Start of a message
	} else if (!at_started && c == '!') {

		//Set index to 0, reset any junk that might be in our buffer.
		parse_buf_idx = 0;
		memset(at_parse_buf, 0, AT_PARSE_BUFFER_SIZE);
		at_started = true;
	
	//End of a message
	} else if (at_started && c == ';') {

		//Then print what the message was
		if (strncmp(at_parse_buf, "reset", AT_PARSE_BUFFER_SIZE) == 0) {
			LOG_INF("Got reset command!");
			if (in_test_mode){
				struct ae_event* a = new_ae_event();
				a->cmd = AE_EVENT_DEREGISTER;
				a->reset = true;
				APP_EVENT_SUBMIT(a);

			}
			else{
				LOG_INF("Not In Test Mode!");
			}
		
		} else if (strncmp(at_parse_buf, "register", AT_PARSE_BUFFER_SIZE) == 0) {
			LOG_INF("Got register command!");
			if (in_test_mode){
				struct ae_event* a = new_ae_event();
				a->cmd = AE_EVENT_TEST_REGISTER;
				APP_EVENT_SUBMIT(a);
			}
			else{
				LOG_INF("Not In Test Mode!");
			}
		
		} else if (strncmp(at_parse_buf, "createDataModel", AT_PARSE_BUFFER_SIZE) == 0) {
			LOG_INF("Got create data model command!");
			if (in_test_mode){
				struct ae_event* a = new_ae_event();
				a->cmd = AE_EVENT_TEST_CREATE_DATA;
				APP_EVENT_SUBMIT(a);
			}
			else{
				LOG_INF("Not In Test Mode!");
			}
		
		} else if (strncmp(at_parse_buf, "retrieveNotifications", AT_PARSE_BUFFER_SIZE) == 0) {
			LOG_INF("Got retrieve notifications command!");
			if (in_test_mode){
				retrieveFlexContainer();
			}
			else{
				LOG_INF("Not In Test Mode!");
			}

		} else if (strncmp(at_parse_buf, "updateDataModel", AT_PARSE_BUFFER_SIZE) == 0) {
			LOG_INF("Got update data model command!");
			if (in_test_mode){
				push_flex_container();
			}
			else{
				LOG_INF("Not In Test Mode!");
			}
		
		} else if (strncmp(at_parse_buf, "deregister", AT_PARSE_BUFFER_SIZE) == 0) {
			LOG_INF("Got deregister command!");
			if (in_test_mode){
				struct ae_event* a = new_ae_event();
				a->cmd = AE_EVENT_DEREGISTER;
				a->reset = false;
				APP_EVENT_SUBMIT(a);
			}
			else{
				LOG_INF("Not In Test Mode!");
			}

		} else if (strncmp(at_parse_buf, "testBegin", AT_PARSE_BUFFER_SIZE) == 0){
			LOG_INF("Got Begin Test Command command!");
			if (!in_test_mode){
				//trigger begining of the test;
				in_test_mode = true;
				struct ae_event* a = new_ae_event();
				a->cmd = AE_EVENT_TEST_MODE;
				APP_EVENT_SUBMIT(a);
			}
			else{
				LOG_INF("Already In Test Mode!");
			}

		} else if (strncmp(at_parse_buf, "testEnd", AT_PARSE_BUFFER_SIZE) == 0){
			LOG_INF("Got Begin End Command command!");
			if (in_test_mode){
				//trigger end of the test;
				in_test_mode = false;
				struct ae_event* a = new_ae_event();
				a->cmd = AE_EVENT_TEST_MODE;
				APP_EVENT_SUBMIT(a);
			}
			else{
				LOG_INF("Not In Test Mode!");
			}

		} else {
			LOG_ERR("Failed to parse message from UART 0! %s", at_parse_buf);
		}

		at_started = false;
	}

	if (parse_buf_idx >= AT_PARSE_BUFFER_SIZE) {
		LOG_WRN("nRF52840 parse buf overrun!");
		at_started = false;
	}
}


static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_uart_data_event(aeh)) {
		const struct uart_data_event *event =
			cast_uart_data_event(aeh);
		if (event->dev_idx == 0) {

			//This is coming from the upper tester
			//we will build a string byte by byte
            for (size_t i = 0; i < event->len; i++) {
				parse_at_command(event->buf[i]);
			}
        }

		return false;
	}

	if (is_module_state_event(aeh)) {
		const struct module_state_event *event =
			cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			// Setup stuff goes in here if we need it
			in_test_mode = false;
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