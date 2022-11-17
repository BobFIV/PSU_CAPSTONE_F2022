/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <caf/led_effect.h>
#include "events/led_state_event.h"

/* This configuration file is included only once from led_state module and holds
 * information about LED effects associated with Asset Tracker v2 states.
 */

/* This structure enforces the header file to be included only once in the build.
 * Violating this requirement triggers a multiple definition error at link time.
 */
const struct {} led_state_def_include_once;

enum led_id {
	LED_ID_1,

	LED_ID_COUNT
};

#define LED_PERIOD_NORMAL	350
#define LED_PERIOD_RAPID	100
#define LED_TICKS_DOUBLE	2
#define LED_TICKS_TRIPLE	3

static const struct led_effect traffic_light_led_effect[] = {
	// yellow breath
	[LED_STATE_YELLOW_BREATH]	= LED_EFFECT_LED_BREATH(LED_PERIOD_NORMAL,
								LED_COLOR(120, 80, 0)),
	// green breath
	[LED_STATE_GREEN_BREATH]	= LED_EFFECT_LED_BREATH(LED_PERIOD_NORMAL,
								LED_COLOR(0, 200, 0)),
	// teal breath
	[LED_STATE_TEAL_BREATH]		= LED_EFFECT_LED_BREATH(LED_PERIOD_NORMAL,
								LED_COLOR(0, 150, 150)),
	// blue breath
	[LED_STATE_BLUE_BREATH]	= LED_EFFECT_LED_BREATH(LED_PERIOD_NORMAL,
								LED_COLOR(0, 0, 250)),
	// purple breath
	[LED_STATE_PURPLE_BREATH]	= LED_EFFECT_LED_BREATH(LED_PERIOD_NORMAL,
								LED_COLOR(120, 0, 120)),
	// triple blink green
	[LED_STATE_GREEN_TRIPLE_BLINK]	= LED_EFFECT_LED_CLOCK(LED_TICKS_TRIPLE,
							       LED_COLOR(0, 250, 0)),
	// double blink white
	[LED_STATE_WHITE_DOUBLE_BLINK]	= LED_EFFECT_LED_CLOCK(LED_TICKS_DOUBLE,
							       LED_COLOR(255, 255, 255)),
	// rapid white to off
	[LED_STATE_RAPID_WHITE_OFF]	= LED_EFFECT_LED_ON_GO_OFF(LED_COLOR(255, 255, 255),
								   LED_PERIOD_NORMAL,
								   LED_PERIOD_RAPID),

	// red solid
	[LED_STATE_RED_BREATH]	= LED_EFFECT_LED_BREATH(LED_PERIOD_NORMAL, 
								   LED_COLOR(255, 0, 0)),
	// fast yellow/orange breath
	[LED_STATE_ORANGE_QUICK]	= LED_EFFECT_LED_BREATH(LED_PERIOD_RAPID,
								LED_COLOR(100, 100, 0)),
	// solid yellow/orange
	[LED_STATE_ORANGE_SOLID]	= LED_EFFECT_LED_ON(LED_COLOR(255, 165, 0)),
	[LED_STATE_TURN_OFF]		= LED_EFFECT_LED_OFF(),
};
