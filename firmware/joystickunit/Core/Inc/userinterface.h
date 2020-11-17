/*******************************************************************************
* @file         : userinterface.h
* @project      : 4D-Joystick, Joystick-Unit
* @author       : Fabian Baer
* @brief        : Handles user buttons and display
*******************************************************************************/

#ifndef __CORE_INC_USERINTERFACE_H_
#define __CORE_INC_USERINTERFACE_H_

#include <cmsis_os.h>

void ui_init( void );
void ui_setRotaryEncoderMQ( osMessageQId msgQueue );
void ui_setupTask( osPriority priority );
void ui_updateDisplay( void );

#endif /* __CORE_INC_USERINTERFACE_H_ */
