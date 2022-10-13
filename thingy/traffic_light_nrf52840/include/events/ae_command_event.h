/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _AE_COMMAND_DATA_EVENT_H_
#define _AE_COMMAND_DATA_EVENT_H_

/**
 * @brief AE Command Event
 * @defgroup ae_command_data_event AE Command Data Event
 * @{
 */

#include <string.h>
#include <zephyr/toolchain/common.h>

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

#ifdef __cplusplus
extern "C" {
#endif

enum ae_commands {
	SET_STATE1, SET_STATE2
};

enum light_states {
    LIGHT_OFF, LIGHT_RED, LIGHT_YELLOW, LIGHT_GREEN, LIGHT_CYCLE
};

/** Command event. */
struct ae_command_event {
	struct app_event_header header;
	enum ae_commands cmd;
	enum light_states light_state;
};

APP_EVENT_TYPE_DECLARE(ae_command_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _AE_COMMAND_DATA_EVENT_H_ */
