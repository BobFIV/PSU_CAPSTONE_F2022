/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _AE_EVENT_H_
#define _AE_EVENT_H_

/**
 * @brief Application Entity Event
 * @defgroup ae_event AE Event
 * @{
 */

#include <string.h>
#include <zephyr/toolchain/common.h>

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

#ifdef __cplusplus
extern "C" {
#endif

enum ae_lights {
	AE_LIGHT_NONE, AE_LIGHT1, AE_LIGHT2
};

enum ae_light_states {
	AE_LIGHT_STATE_NONE, AE_LIGHT_OFF, AE_LIGHT_RED, AE_LIGHT_YELLOW, AE_LIGHT_GREEN
};

enum ae_event_types {
	AE_EVENT_LIGHT_CMD, AE_EVENT_REGISTER, AE_EVENT_CREATE_DATA_MODEL, AE_EVENT_POLL, AE_EVENT_TEST_MODE, AE_EVENT_DEREGISTER, AE_EVENT_TEST_CREATE_DATA, AE_EVENT_TEST_REGISTER
};

/** Peer connection event. */
struct ae_event {
	struct app_event_header header;

	// Command type
	enum ae_event_types cmd;
	// State that the lights should be put into
	enum ae_light_states new_light1_state;
	enum ae_light_states new_light2_state;
	// If set to true, then AE_EVENT_REGISTER should then trigger an AE_EVENT_CREATE_DATA_MODEL
	bool do_init_sequence;
	bool reset;
};

enum ae_light_states string_to_light_state(char* str, size_t max_length);
void light_state_to_string(enum ae_light_states state, char* output);

APP_EVENT_TYPE_DECLARE(ae_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _AE_EVENT_H_ */
