/*******************************************************************************
* @file         : system.h
* @project      : 4D-Joystick, Remote-Unit
* @author       : Fabian Baer
* @brief        : General system tools
*******************************************************************************/

#ifndef _DRIVER_INC_SYSTEM_H
#define _DRIVER_INC_SYSTEM_H

#include <joystickunit.h>

void system_setupClock(void);
void system_init(void);
void system_watchdogHandler( void );
void system_handler(Joystickunit_State_t joyState);

#ifdef USE_DEBUG_UART
  UART_HandleTypeDef* system_getUartHandle(void);
  void system_doNotUse_debugMessage(uint8_t* text, uint32_t size);
  #define system_debugMessage(text) system_doNotUse_debugMessage((uint8_t*)text, sizeof(text))
#else
  #define system_debugMessage(text) (void)text;
#endif


#endif /* _DRIVER_INC_SYSTEM_H */

