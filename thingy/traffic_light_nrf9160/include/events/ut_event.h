/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _UT_EVENT_H_
#define _UT_EVENT_H_

/**
 * @brief Upper Tester Event
 * @defgroup ut_event UT Event
 * @{
 */

#include <string.h>
#include <zephyr/toolchain/common.h>

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

#ifdef __cplusplus
extern "C" {
#endif

enum ut_commands {
	CREATE_DATA_MODEL 
	// TODO: Noah, fill the rest of these in
};

/** Upper Tester command event. */
struct ut_event {
	struct app_event_header header;

	// Command type
	enum ut_commands cmd;
};

APP_EVENT_TYPE_DECLARE(ut_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _AE_EVENT_H_ */
