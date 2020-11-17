/*******************************************************************************
* @file         : ioExpander.h
* @project      : 4D-Joystick, Remote-Unit
* @author       : Fabian Baer
* @brief        : Communication with the IO-expander
*******************************************************************************/

#ifndef _DRIVER_INC_IOEXPANDER_H
#define _DRIVER_INC_IOEXPANDER_H

void ioExpander_init(void);
uint8_t ioExpander_getInputs(void);
void ioExpander_setOutputs(uint8_t values);

#endif /* _DRIVER_INC_IOEXPANDER_H */

