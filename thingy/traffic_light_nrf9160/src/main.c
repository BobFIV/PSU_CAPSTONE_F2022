/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief Nordic UART Service Client sample
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>

#include <zephyr/settings/settings.h>

#include <zephyr/drivers/uart.h>

#include <zephyr/logging/log.h>
#include <zephyr/console/console.h>

#include <app_event_manager.h>

#define MODULE main
#include "events/module_state_event.h"
#define LOG_MODULE_NAME main
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

void main() {

	if (app_event_manager_init()) {
		LOG_ERR("Application Event Manager not initialized!");
		k_sleep(K_SECONDS(2));
		return;
	} 

	LOG_INF("Application Event Manager initialized.");
	module_set_state(MODULE_STATE_READY);
	// TODO: Have a while loop here.
	LOG_INF("Returned from module_set_state_ready");
	while(true) {
		k_sleep(K_SECONDS(2));
		LOG_INF("Boop from 9160!");
	}
}
