/*******************************************************************************
* @file         : ioExpander.c
* @project      : 4D-Joystick, Remote-Unit
* @author       : Fabian Baer
* @brief        : Communication with the IO-expander
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include <stm32f4xx_hal.h>
#include <board.h>
#include <system.h>
#include <remoteIO.h>


/* Defines -------------------------------------------------------------------*/
#define DEBUG_PREFIX        "ioExp - "

#define I2C_ADDRESS   (uint16_t)0b01000000
#define I2C_TIMEOUT   10

#define IODIR_A       0x00
#define IODIR_B       0x01
#define IODIR_OUT     0x0
#define IODIR_IN      0x1

#define GPPU_A        0x0C
#define GPPU_B        0x0D
#define GPPU_DIS      0x0
#define GPPU_EN       0x1

#define GPIO_A        0x12
#define GPIO_B        0x13
#define OLAT_A        0x14
#define OLAT_B        0x15

#define WHOLE_BYTE(x)   (x | (x<<1) | (x<<2) | (x<<3) | (x<<4) | (x<<5) | (x<<6) | (x<<7))


/* Variables -----------------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;


/* Code ----------------------------------------------------------------------*/

/*******************************************************************************
 * Initalizes I2C and the GPIO expander
 *
 * @return nothing
 *******************************************************************************/
void ioExpander_init(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  uint8_t data[2];

  //Init peripherals
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_I2C1_CLK_ENABLE();

  //Init GPIO
  GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  //Init I2C
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 400000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
    system_debugMessage(DEBUG_PREFIX "Cannot init I2C");
    NVIC_SystemReset();
  }

  //Configure outputs
  data[0] = IODIR_A;
  data[1] = WHOLE_BYTE(IODIR_OUT);
  if(HAL_I2C_Master_Transmit(&hi2c1, I2C_ADDRESS, data, 2, I2C_TIMEOUT) != HAL_OK) {
    system_debugMessage(DEBUG_PREFIX "Communication error (1)");
    NVIC_SystemReset();
  }

  //Configure inputs
  data[0] = IODIR_B;
  data[1] = WHOLE_BYTE(IODIR_IN);
  if (HAL_I2C_Master_Transmit(&hi2c1, I2C_ADDRESS, data, 2, I2C_TIMEOUT) != HAL_OK) {
    system_debugMessage(DEBUG_PREFIX "Communication error (2)");
    NVIC_SystemReset();
  }

  //Enable pullup
  data[0] = GPPU_B;
  data[1] = WHOLE_BYTE(GPPU_EN);
  if (HAL_I2C_Master_Transmit(&hi2c1, I2C_ADDRESS, data, 2, I2C_TIMEOUT) != HAL_OK) {
    system_debugMessage(DEBUG_PREFIX "Communication error (3)");
    NVIC_SystemReset();
  }
}


/*******************************************************************************
 * Writes to the GPIO-expander
 *
 * @param values The value of the ouptut port.
 * @return nothing
 *******************************************************************************/
void ioExpander_setOutputs(uint8_t values) {
  uint8_t data[2];

  data[0] = OLAT_A;
  data[1] = values;
  if (HAL_I2C_Master_Transmit(&hi2c1, I2C_ADDRESS, data, 2, I2C_TIMEOUT) != HAL_OK) {
    system_debugMessage(DEBUG_PREFIX "Communication error (4)");
    NVIC_SystemReset();
  }
}


/*******************************************************************************
 * Reads from the GPIO-expander
 *
 * @return The states of the input-port
 *******************************************************************************/
uint8_t ioExpander_getInputs(void) {
  uint8_t data;

  data = GPIO_B;
  if (HAL_I2C_Master_Transmit(&hi2c1, I2C_ADDRESS, &data, 1, I2C_TIMEOUT) != HAL_OK) {
    system_debugMessage(DEBUG_PREFIX "Communication error (5)");
    NVIC_SystemReset();
  }

  if (HAL_I2C_Master_Receive(&hi2c1, I2C_ADDRESS, &data, 1, I2C_TIMEOUT) != HAL_OK) {
    system_debugMessage(DEBUG_PREFIX "Communication error (6)");
    NVIC_SystemReset();
  }

  return data;
}

