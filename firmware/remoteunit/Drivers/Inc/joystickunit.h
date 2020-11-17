/*******************************************************************************
* @file         : joystickunit.h
* @project      : 4D-Joystick, Remote-Unit
* @author       : Fabian Baer
* @brief        : Handles communication with joystick-unit.
*******************************************************************************/

#ifndef _DRIVER_INC_JOYSTICKUNIT_H
#define _DRIVER_INC_JOYSTICKUNIT_H

#include <remoteIO.h>

typedef enum {
  JOY_STATE_NOT_AVAILABLE,
  JOY_STATE_OK,
  JOY_STATE_ERROR
} Joystickunit_State_t;

void joystickunit_init(void);
Joystickunit_State_t joystickunit_communicate( RemoteIO_States_t* in, RemoteIO_States_t* out );

#endif /* _DRIVER_INC_JOYSTICKUNIT_H */

