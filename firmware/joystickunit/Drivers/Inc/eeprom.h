/*******************************************************************************
* @file         : eeprom.h
* @project      : 4D-Joystick, Joystick-Unit
* @author       : Fabian Baer
* @brief        : Communication with EEPROM
*******************************************************************************/
#ifndef __DRIVERS_INC_EEPROM_H_
#define __DRIVERS_INC_EEPROM_H_

void eeprom_init(void);
void eeprom_read(uint32_t startAddress, uint8_t* pData, uint32_t length);
void eeprom_write(uint32_t startAddress, uint8_t* pData, uint32_t length);

#endif /* __DRIVERS_INC_EEPROM_H_ */
