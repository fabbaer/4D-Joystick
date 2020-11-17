/*******************************************************************************
* @file         : remoteIO.h
* @project      : 4D-Joystick, Remote-Unit
* @author       : Fabian Baer
* @brief        : Connections to remote
*******************************************************************************/

#ifndef _DRIVER_INC_REMOTEIO_H
#define _DRIVER_INC_REMOTEIO_H

#include "board.h"
#include "stm32f4xx_hal.h"

typedef struct {
  uint32_t analog[4];
  GPIO_PinState digital[24];
} RemoteIO_States_t;

void remoteIO_init(void);
void remoteIO_getStates(RemoteIO_States_t* states);
void remoteIO_setStates(RemoteIO_States_t* states);

#endif /* _DRIVER_INC_REMOTEIO_H */

