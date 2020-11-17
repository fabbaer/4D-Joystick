/*******************************************************************************
* @file         : dac.c
* @project      : 4D-Joystick, Remote-Unit
* @author       : Fabian Baer
* @brief        : Communication with DAC
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include <stm32f4xx_hal.h>
#include <board.h>
#include <system.h>
#include <dac.h>


/* Defines -------------------------------------------------------------------*/
#define DEBUG_PREFIX        "DAC - "

#define DAC_A               0x0000
#define DAC_B               0x4000
#define DAC_C               0x8000
#define DAC_D               0xC000
#define WRITE_CH_NO_UPDATE  0x0000
#define WRITE_CH_UPDATE     0x1000
#define WRITE_ALL_UPDATE    0x2000
#define POWER_DOWN          0x3000

#define SPI_TIMEOUT_MS      10


/* Variables -----------------------------------------------------------------*/
SPI_HandleTypeDef hspi5;


/* Code ----------------------------------------------------------------------*/

/*******************************************************************************
 * Initalizes SPI and the DAC
 *
 * @return nothing
 *******************************************************************************/
void dac_init(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  //Enable peripherals
  __HAL_RCC_SPI5_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  //Init GPIOs
  HAL_GPIO_WritePin(GPIOB, DAC_SYNC_Pin, GPIO_PIN_SET);

  GPIO_InitStruct.Pin = DAC_SYNC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = DAC_SCLK_Pin | DAC_DIN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF6_SPI5;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  //Init SPI
  hspi5.Instance = SPI5;
  hspi5.Init.Mode = SPI_MODE_MASTER;
  hspi5.Init.Direction = SPI_DIRECTION_2LINES;
  hspi5.Init.DataSize = SPI_DATASIZE_16BIT;
  hspi5.Init.CLKPolarity = SPI_POLARITY_HIGH;
  hspi5.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi5.Init.NSS = SPI_NSS_SOFT;
  hspi5.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi5.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi5.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi5.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  if(HAL_SPI_Init(&hspi5) != HAL_OK) {
    system_debugMessage(DEBUG_PREFIX "Cannot init SPI");
    NVIC_SystemReset();
  }
}


/*******************************************************************************
 * Write to the DAC channels
 *
 * @return nothing
 *******************************************************************************/
void dac_setAllChannels(uint16_t ch1_val, uint16_t ch2_val, uint16_t ch3_val, uint16_t ch4_val) {
  uint16_t data;

  //Channel A
  HAL_GPIO_WritePin(GPIOB, DAC_SYNC_Pin, GPIO_PIN_RESET);
  data = (ch1_val & 0x0FFF) | WRITE_CH_UPDATE | DAC_A;
  if(HAL_SPI_Transmit(&hspi5, (uint8_t *)&data, 1, SPI_TIMEOUT_MS) != HAL_OK) {
    system_debugMessage(DEBUG_PREFIX "Communication error (setAllChannels - A)");
    NVIC_SystemReset();
  }
  HAL_GPIO_WritePin(GPIOB, DAC_SYNC_Pin, GPIO_PIN_SET);

  //Channel B
  HAL_GPIO_WritePin(GPIOB, DAC_SYNC_Pin, GPIO_PIN_RESET);
  data = (ch2_val & 0x0FFF) | WRITE_CH_UPDATE | DAC_B;
  if(HAL_SPI_Transmit(&hspi5, (uint8_t *)&data, 1, SPI_TIMEOUT_MS) != HAL_OK) {
    system_debugMessage(DEBUG_PREFIX "Communication error (setAllChannels - B)");
    NVIC_SystemReset();
  }
  HAL_GPIO_WritePin(GPIOB, DAC_SYNC_Pin, GPIO_PIN_SET);

  //Channel C
  HAL_GPIO_WritePin(GPIOB, DAC_SYNC_Pin, GPIO_PIN_RESET);
  data = (ch3_val & 0x0FFF) | WRITE_CH_UPDATE | DAC_C;
  if(HAL_SPI_Transmit(&hspi5, (uint8_t *)&data, 1, SPI_TIMEOUT_MS) != HAL_OK) {
    system_debugMessage(DEBUG_PREFIX "Communication error (setAllChannels - C)");
    NVIC_SystemReset();
  }
  HAL_GPIO_WritePin(GPIOB, DAC_SYNC_Pin, GPIO_PIN_SET);

  //Channel D
  HAL_GPIO_WritePin(GPIOB, DAC_SYNC_Pin, GPIO_PIN_RESET);
  data = (ch4_val & 0x0FFF) | WRITE_CH_UPDATE | DAC_D;
  if(HAL_SPI_Transmit(&hspi5, (uint8_t *)&data, 1, SPI_TIMEOUT_MS) != HAL_OK) {
    system_debugMessage(DEBUG_PREFIX "Communication error (setAllChannels - D)");
    NVIC_SystemReset();
  }
  HAL_GPIO_WritePin(GPIOB, DAC_SYNC_Pin, GPIO_PIN_SET);

}
