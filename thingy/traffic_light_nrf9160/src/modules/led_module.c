/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <caf/events/led_event.h>
#include "events/led_state_event.h"
#include "led_state_def.h"

#define MODULE led_state
#include <caf/events/module_state_event.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(led);

static void send_led_event(size_t led_id, const struct led_effect *led_effect)
{
	__ASSERT_NO_MSG(led_effect);
	__ASSERT_NO_MSG(led_id < LED_ID_COUNT);

	struct led_event *event = new_led_event();

	event->led_id = led_id;
	event->led_effect = led_effect;
	APP_EVENT_SUBMIT(event);
}

static void update_led(enum led_state state)
{
	uint8_t led_bm = 0;

	switch (state)	{
	case LED_STATE_YELLOW_BREATH:
		send_led_event(LED_ID_1,
				&traffic_light_led_effect[LED_STATE_YELLOW_BREATH]);
		led_bm |= BIT(LED_ID_1);
		break;
	case LED_STATE_PURPLE_BREATH:
		send_led_event(LED_ID_1,
				&traffic_light_led_effect[LED_STATE_PURPLE_BREATH]);
		led_bm |= BIT(LED_ID_1);
		break;
	case LED_STATE_GREEN_BREATH:
		send_led_event(LED_ID_1,
				&traffic_light_led_effect[LED_STATE_GREEN_BREATH]);
		led_bm |= BIT(LED_ID_1);
		break;
	case LED_STATE_GREEN_TRIPLE_BLINK:
		send_led_event(LED_ID_1,
			       &traffic_light_led_effect[LED_STATE_GREEN_TRIPLE_BLINK]);
		led_bm |= BIT(LED_ID_1);
		break;
	case LED_STATE_WHITE_DOUBLE_BLINK:
		send_led_event(LED_ID_1,
			       &traffic_light_led_effect[LED_STATE_WHITE_DOUBLE_BLINK]);
		led_bm |= BIT(LED_ID_1);
		break;
	case LED_STATE_RAPID_WHITE_OFF:
		send_led_event(LED_ID_1,
			       &traffic_light_led_effect[LED_STATE_RAPID_WHITE_OFF]);
		led_bm |= BIT(LED_ID_1);
		break;
	case LED_STATE_TEAL_BREATH:
		send_led_event(LED_ID_1,
				&traffic_light_led_effect[LED_STATE_TEAL_BREATH]);
		led_bm |= BIT(LED_ID_1);
		break;
	case LED_STATE_BLUE_BREATH:
		send_led_event(LED_ID_1,
			       &traffic_light_led_effect[LED_STATE_BLUE_BREATH]);
		send_led_event(LED_ID_1,
			       &traffic_light_led_effect[LED_STATE_BLUE_BREATH]);
		led_bm |= (BIT(LED_ID_1) | BIT(LED_ID_1));
		break;
	case LED_STATE_RED_SOLID:
		for (size_t i = 0; i < LED_ID_COUNT; i++) {
			send_led_event(i,
					&traffic_light_led_effect[LED_STATE_RED_SOLID]);
			led_bm |= BIT(i);
		}
		break;
	case LED_STATE_ORANGE_QUICK:
		send_led_event(LED_ID_1,
			       &traffic_light_led_effect[LED_STATE_ORANGE_QUICK]);
		send_led_event(LED_ID_1,
			       &traffic_light_led_effect[LED_STATE_ORANGE_QUICK]);
		led_bm |= (BIT(LED_ID_1) | BIT(LED_ID_1));
		break;
	case LED_STATE_ORANGE_SOLID:
		send_led_event(LED_ID_1,
			       &traffic_light_led_effect[LED_STATE_ORANGE_SOLID]);
		send_led_event(LED_ID_1,
			       &traffic_light_led_effect[LED_STATE_ORANGE_SOLID]);
		led_bm |= (BIT(LED_ID_1) | BIT(LED_ID_1));
		break;
	case LED_STATE_TURN_OFF:
		/* Do nothing. */
		break;
	default:
		LOG_WRN("Unrecognized LED state event send");
		break;
	}

	for (size_t i = 0; i < LED_ID_COUNT; i++) {
		if (!(led_bm & BIT(i))) {
			send_led_event(i, &traffic_light_led_effect[LED_STATE_TURN_OFF]);
		}
	}
}

static bool handle_led_state_event(const struct led_state_event *event)
{
	update_led(event->state);
	return false;
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_led_state_event(aeh)) {
		return handle_led_state_event(cast_led_state_event(aeh));
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, led_state_event);