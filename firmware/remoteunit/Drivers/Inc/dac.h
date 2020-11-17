/*******************************************************************************
* @file         : dac.h
* @project      : 4D-Joystick, Remote-Unit
* @author       : Fabian Baer
* @brief        : Communication with DAC
*******************************************************************************/

#ifndef _DRIVER_INC_DAC_H
#define _DRIVER_INC_DAC_H

void dac_init(void);
void dac_setAllChannels(uint16_t ch1_val, uint16_t ch2_val, uint16_t ch3_val, uint16_t ch4_val);

#endif /* _DRIVER_INC_DAC_H */

