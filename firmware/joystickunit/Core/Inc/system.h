/*******************************************************************************
* @file         : system.h
* @project      : 4D-Joystick, Joystick-Unit
* @author       : Fabian Baer
* @brief        : General system tools
*******************************************************************************/
#ifndef __CORE_INC_SYSTEM_H_
#define __CORE_INC_SYSTEM_H_

#include <stdbool.h>
#include <cmsis_os.h>

#define FW_VERSION_STRING "v0.1"

void system_init(void);
void system_setupTask( osPriority priority );
bool system_isUSBConnected(void);
bool system_isRemoteConnected(void);
bool system_isPoweredViaUSB(void);
uint32_t system_getSupplyVoltage(void);
void system_errorHandler(void);
void system_checkBootloader(void);
void system_loadBootloader(void);

#endif /* __CORE_INC_SYSTEM_H_ */
