/****************************************************************************
 * (C) Tokyo Cosmos Electric, Inc. (TOCOS) - 2013 all rights reserved.
 *
 * Condition to use:
 *   - The full or part of source code is limited to use for TWE (TOCOS
 *     Wireless Engine) as compiled and flash programmed.
 *   - The full or part of source code is prohibited to distribute without
 *     permission from TOCOS.
 *
 ****************************************************************************/

/*
 * event.h
 *
 *  Created on: 2013/01/10
 *      Author: seigo13
 */

#ifndef APP_EVENT_H_
#define APP_EVENT_H_

#include "ToCoNet_event.h"

typedef enum
{
	E_EVENT_APP_BASE = ToCoNet_EVENT_APP_BASE,
	E_EVENT_APP_START_NWK,
	E_EVENT_APP_GET_IC_INFO
} teEventApp;

// STATE MACHINE
typedef enum
{
	E_STATE_APP_BASE = ToCoNet_STATE_APP_BASE,
	E_STATE_APP_WAIT_NW_START,
	E_STATE_APP_WAIT_TX,
	E_STATE_APP_IO_WAIT_RX,
	E_STATE_APP_IO_RECV_ERROR,
	E_STATE_APP_SLEEP,
	E_STATE_APP_CHAT_SLEEP,
	E_STATE_APP_PREUDO_SLEEP,
	E_STATE_APP_RETRY,
	E_STATE_APP_RETRY_TX
} teStateApp;

#endif /* EVENT_H_ */
