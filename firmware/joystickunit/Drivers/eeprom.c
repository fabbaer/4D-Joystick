/*******************************************************************************
* @file         : eeprom.c
* @project      : 4D-Joystick, Joystick-Unit
* @author       : Fabian Baer
* @brief        : Communication with EEPROM
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include <stm32l4xx_hal.h>
#include <board.h>
#include <system.h>
#include <eeprom.h>


/* Defines -------------------------------------------------------------------*/
#define I2C_ADDRESS   0xA0
#define DELAY(x)      if(osKernelRunning()) {osDelay(x);} else {HAL_Delay(x);}


/* Variables -----------------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;


/* Code ----------------------------------------------------------------------*/

/*******************************************************************************
 * Initializes peripherals for EEPROM
 *
 * @return nothing
 *******************************************************************************/
void eeprom_init( void ) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  //Init clocks
  __HAL_RCC_I2C1_CLK_ENABLE();

  //Init GPIOs
  GPIO_InitStruct.Pin = I2C_SCL_Pin | I2C_SDA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  //Init I2C
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x10909CEC;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
    system_errorHandler();
  }

  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK) {
    system_errorHandler();
  }

  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK) {
    system_errorHandler();
  }
}


/*******************************************************************************
 * Read data from the EEPROM
 *
 * @param startAddress The address of the first byte to read
 * @param data A pointer to a array, where the data is stored
 * @param length The amount of bytes to read.
 * @return nothing
 *******************************************************************************/
void eeprom_read(uint32_t startAddress, uint8_t* pData, uint32_t length) {
  uint8_t address[2];

  address[0] = (startAddress & 0xFF00) >> 8;
  address[1] = (startAddress & 0x00FF);

  if(HAL_I2C_Master_Transmit(&hi2c1, I2C_ADDRESS, address, 2, 100) != HAL_OK) {
    //Wait till last write cycle has ended
    DELAY(5);
    if(HAL_I2C_Master_Transmit(&hi2c1, I2C_ADDRESS, address, 2, 100) != HAL_OK) {
      system_errorHandler();
    }
  }
  if(HAL_I2C_Master_Receive(&hi2c1, I2C_ADDRESS, pData, length, 100) != HAL_OK) {
    system_errorHandler();
  }
}


/*******************************************************************************
 * Write data to the EEPROM (in blocks of 64 byte)
 *
 * @param startAddress The address of the first byte to read
 * @param data A pointer to a array, where the data is stored
 * @param length The amount of bytes to write.
 * @return nothing
 *******************************************************************************/
void eeprom_write(uint32_t startAddress, uint8_t* pData, uint32_t length) {
  uint8_t pkg[66];
  uint32_t cnt = 0;
  uint32_t cycle = 0;

  //Transmit up to 64 bytes per cycle
  do {
    cnt = 0;

    //Add address
    pkg[cnt++] = ((startAddress + (cycle*64)) & 0x7F00) >> 8;
    pkg[cnt++] = ((startAddress + (cycle*64)) & 0x00FF);

    //Add pData
    for(uint32_t i = 0; (i<length && i<64); i++) {
      pkg[cnt++] = pData[i+(cycle*64)];
    }

    if(HAL_I2C_Master_Transmit(&hi2c1, I2C_ADDRESS, pkg, cnt, 100) != HAL_OK) {
      //Wait till last write cycle has ended
      DELAY(5);
      if(HAL_I2C_Master_Transmit(&hi2c1, I2C_ADDRESS, pkg, cnt, 100) != HAL_OK) {
        system_errorHandler();
      }
    }

    //Wait if another cycle is required (>64 byte transmitted)
    if(length > 64) {
      length -= 64;
      cycle++;
      DELAY(5);
    } else {
      length = 0;
    }
  } while(length != 0);
}
