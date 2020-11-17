/*******************************************************************************
* @file         : spiDMA.h
* @project      : 4D-Joystick, Joystick-Unit
* @author       : Fabian Baer
* @brief        : SPI (TX only) interface via DMA and bitbanging
*******************************************************************************/

#ifndef __DRIVERS_INC_SPIDMA_H_
#define __DRIVERS_INC_SPIDMA_H_

#include <stm32l4xx_hal.h>

typedef enum {
  SPI_DMA_STATE_RESET,
  SPI_DMA_STATE_READY,
  SPI_DMA_STATE_BUSY,
  SPI_DMA_STATE_ERROR
} SPI_DMA_StateTypeDef;

typedef enum {
  SPI_DMA_POLARITY_LOW,
  SPI_DMA_POLARITY_HIGH
} SPI_DMA_PolarityTypeDef;

typedef enum {
  SPI_DMA_EDGE_FIRST,
  SPI_DMA_EDGE_SECOND
} SPI_DMA_EdgeTypeDef;

typedef struct {
  SPI_DMA_StateTypeDef state;
  uint32_t speed;
  SPI_DMA_PolarityTypeDef polarity;
  SPI_DMA_EdgeTypeDef edge;
} SPI_DMA_HandleTypeDef;


HAL_StatusTypeDef spiDMA_init( SPI_DMA_HandleTypeDef* hspi );
HAL_StatusTypeDef spiDMA_transmit( SPI_DMA_HandleTypeDef* hspi, uint8_t* data, uint8_t size);
void spiDMA_waitTillReady( void );


#endif /* __DRIVERS_INC_SPIDMA_H_ */
