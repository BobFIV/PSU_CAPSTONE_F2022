/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _LED_STATE_EVENT_H_
#define _LED_STATE_EVENT_H_

/**
 * @brief Led State Event
 * @defgroup asset Tracker Led State Event
 * @{
 */

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>


/** @brief Asset Tracker led states in the application. */
enum led_state {
	LED_STATE_YELLOW_BREATH,
	LED_STATE_PURPLE_BREATH,
	LED_STATE_GREEN_BREATH,
	LED_STATE_GREEN_TRIPLE_BLINK,
	LED_STATE_WHITE_DOUBLE_BLINK,
	LED_STATE_RAPID_WHITE_OFF,
	LED_STATE_TEAL_BREATH,
	LED_STATE_BLUE_BREATH,
	LED_STATE_RED_SOLID,
	LED_STATE_ORANGE_QUICK,
	LED_STATE_ORANGE_SOLID,
	LED_STATE_TURN_OFF,
	LED_STATE_COUNT
};

/** @brief Led state event. */
struct led_state_event {
	struct app_event_header header; /**< Event header. */

	enum led_state state;
};

APP_EVENT_TYPE_DECLARE(led_state_event);

/** @} */

#endif /* _LED_STATE_EVENT_H_ */
