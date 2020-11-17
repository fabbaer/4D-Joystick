/*******************************************************************************
* @file         : adc.h
* @project      : 4D-Joystick, Joystick-Unit
* @author       : Fabian Baer
* @brief        : ADC abstraction layer
*******************************************************************************/
#ifndef __DRIVERS_INC_ADC_H_
#define __DRIVERS_INC_ADC_H_

void adc1_init( void );
void adc2_init( void );
void adc1_start( void );
void adc2_start( void );
void adc1_getADC( uint32_t* values );
void adc2_getADC( uint32_t* values );

#endif /* __DRIVERS_INC_ADC_H_ */
