/*******************************************************************************
* @file         : spiDMA.c
* @project      : 4D-Joystick, Joystick-Unit
* @author       : Fabian Baer
* @brief        : SPI (TX only) interface via DMA and bitbanging
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include <board.h>
#include <stm32l4xx_hal.h>
#include <spiDMA.h>
#include <cmsis_os.h>


/* Defines -------------------------------------------------------------------*/
#define TX_BUFFERSIZE       256
#define OUTPUT_PORT         DISP_MOSI_Port
#define MOSI_PIN            DISP_MOSI_Pin
#define SCLK_PIN            DISP_CLK_Pin


/* Macros --------------------------------------------------------------------*/
#define BITMASK_MOSI_SET    (MOSI_PIN <<  0)
#define BITMASK_MOSI_RESET  (MOSI_PIN << 16)
#define BITMASK_SCLK_SET    (SCLK_PIN <<  0)
#define BITMASK_SCLK_RESET  (SCLK_PIN << 16)


/* Prototypes ----------------------------------------------------------------*/
static inline HAL_StatusTypeDef spiDMA_initTimer( SPI_DMA_HandleTypeDef* hspi );
static inline HAL_StatusTypeDef spiDMA_initDMA( SPI_DMA_HandleTypeDef* hspi );


/* Variables -----------------------------------------------------------------*/
static TIM_HandleTypeDef htim1;
static DMA_HandleTypeDef hdma;
static SPI_DMA_HandleTypeDef* pHspi;
static uint32_t txBuffer[TX_BUFFERSIZE*8*2];


/* Code ----------------------------------------------------------------------*/

/*******************************************************************************
 * Initializes all peripherals for the SPI interface.
 *
 * @param hspi A pointer to the SPI handle
 * @return HAL_OK in case of success.
 *******************************************************************************/
HAL_StatusTypeDef spiDMA_init( SPI_DMA_HandleTypeDef* hspi ) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  //Init handle
  if(hspi == NULL) {
    return HAL_ERROR;
  } else {
    pHspi = hspi;
    hspi->state = SPI_DMA_STATE_RESET;
  }

  //Init peripherals

  __DMA1_CLK_ENABLE();
  __GPIOB_CLK_ENABLE();

  //Init GPIOs
  HAL_GPIO_WritePin(OUTPUT_PORT, SCLK_PIN, GPIO_PIN_SET);
  HAL_GPIO_WritePin(OUTPUT_PORT, MOSI_PIN, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = MOSI_PIN | SCLK_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
  HAL_GPIO_Init(OUTPUT_PORT, &GPIO_InitStruct);

  //Init Timer
  if(spiDMA_initTimer(hspi) != HAL_OK) {
    hspi->state = SPI_DMA_STATE_ERROR;
    return HAL_ERROR;
  }

  //Init DMA
  if(spiDMA_initDMA(hspi) != HAL_OK) {
    hspi->state = SPI_DMA_STATE_ERROR;
    return HAL_ERROR;
  }

  hspi->state = SPI_DMA_STATE_READY;
  return HAL_OK;
}


/*******************************************************************************
 * Initializes all peripherals for the timer which is used as timesource.
 *
 * @param hspi A pointer to the SPI handle
 * @return HAL_OK in case of success.
 *******************************************************************************/
static inline HAL_StatusTypeDef spiDMA_initTimer( SPI_DMA_HandleTypeDef* hspi ) {
  uint32_t bitTime;

  //Enable timer1
  __TIM1_CLK_ENABLE();

  //Calculate bit-time
  bitTime = ((uint32_t) ((HAL_RCC_GetSysClockFreq() / hspi->speed) - 1));

  //Init timer
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.ClockDivision = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  if(HAL_TIM_Base_Init(&htim1) != HAL_OK) {
    return HAL_ERROR;
  }

  //Set values
  htim1.Instance->ARR = bitTime;
  htim1.Instance->CCR2 = bitTime/2;
  htim1.Instance->EGR = 0x02;

  if(hspi->edge == SPI_DMA_EDGE_SECOND) {
    htim1.Instance->CCR1 = bitTime;
  } else {
    htim1.Instance->CCR1 = bitTime/2;
  }

  return HAL_OK;
}


/*******************************************************************************
 * Initializes all peripherals for the DMA which handles the transfer.
 *
 * @param hspi A pointer to the SPI handle
 * @return HAL_OK in case of success.
 *******************************************************************************/
static inline HAL_StatusTypeDef spiDMA_initDMA( SPI_DMA_HandleTypeDef* hspi ) {
  (void)hspi;

  //Init DMA
  hdma.Instance = DMA1_Channel2;
  hdma.Init.Request = DMA_REQUEST_7;
  hdma.Init.Direction = DMA_MEMORY_TO_PERIPH;
  hdma.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma.Init.MemInc = DMA_MINC_ENABLE;
  hdma.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
  hdma.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
  hdma.Init.Mode = DMA_NORMAL;
  hdma.Init.Priority = DMA_PRIORITY_HIGH;
  __HAL_LINKDMA(&htim1, hdma[1], hdma);

  if(HAL_DMA_Init(htim1.hdma[1]) != HAL_OK) {
    return HAL_ERROR;
  }

  //Set source and destination addresses
  hdma.Instance->CPAR = (uint32_t)(&(OUTPUT_PORT->BSRR));
  hdma.Instance->CMAR = (uint32_t)(txBuffer);

  //Enable DMA interrupt
  HAL_NVIC_SetPriority(DMA1_Channel2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel2_IRQn);

  return HAL_OK;
}


/*******************************************************************************
 * Transmits a message via SPI in non-blocking mode. If a transfer is already in
 * progress, the function blocks till SPI gets free.
 *
 * @param hspi A pointer to the SPI handle
 * @param data A pointer to the data-struct to send
 * @param size The number of bytes to send
 * @return HAL_OK in case of success.
 *******************************************************************************/
HAL_StatusTypeDef spiDMA_transmit( SPI_DMA_HandleTypeDef* hspi, uint8_t* data, uint8_t size ) {
  if (size > TX_BUFFERSIZE || size == 0 || hspi->state == SPI_DMA_STATE_ERROR
      || hspi->state == SPI_DMA_STATE_RESET) {
    return HAL_ERROR;
  }

  //Wait till last transmission has finished
  spiDMA_waitTillReady();
  hspi->state = SPI_DMA_STATE_BUSY;

  //Prepare data
  uint32_t buffer_cnt = 0;
  for(uint32_t byte_cnt = 0; byte_cnt < size; byte_cnt++) {
    for(uint32_t bit_cnt = 0; bit_cnt < 8; bit_cnt++) {
      if((data[byte_cnt] & (1 << (7-bit_cnt))) != 0) {
        txBuffer[buffer_cnt++] = BITMASK_SCLK_RESET | BITMASK_MOSI_SET ;
      } else {
        txBuffer[buffer_cnt++] = BITMASK_SCLK_RESET | BITMASK_MOSI_RESET;
      }

      if((data[byte_cnt] & (1 << (7-bit_cnt))) != 0) {
        txBuffer[buffer_cnt++] = BITMASK_SCLK_SET | BITMASK_MOSI_SET;
      } else {
        txBuffer[buffer_cnt++] = BITMASK_SCLK_SET | BITMASK_MOSI_RESET;
      }
    }
  }

  //Configure DMA transfer length
  hdma.Instance->CNDTR = buffer_cnt;

  //Enable peripherals
  __HAL_DMA_ENABLE_IT(&hdma, DMA_IT_TC);
  __HAL_TIM_ENABLE_DMA(&htim1, TIM_DMA_CC1);
  __HAL_DMA_ENABLE(&hdma);
  __HAL_TIM_ENABLE(&htim1);

  return HAL_OK;
}


/*******************************************************************************
 * Waits for the end of the current transfer. If RTOS is active, it will use
 * osDelay(1) for this, else it uses a blocking while-loop.
 *
 * @return nothing
 *******************************************************************************/
void spiDMA_waitTillReady( void ) {
  while(pHspi->state == SPI_DMA_STATE_BUSY) {
    if(osKernelRunning()) {
      osDelay(1);
    }
  }
}


/*******************************************************************************
 * Interrupt callback after transfer completes (disables all peripherals)
 *
 * @return nothing
 *******************************************************************************/
void DMA1_Channel2_IRQHandler( void ) {
  //Disable peripherals
  __HAL_TIM_DISABLE(&htim1);
  __HAL_DMA_DISABLE_IT(htim1.hdma[1], DMA_IT_TC);
  __HAL_DMA_DISABLE(htim1.hdma[1]);

  //Set new state
  pHspi->state = SPI_DMA_STATE_READY;
  __HAL_DMA_CLEAR_FLAG(&hdma, __HAL_DMA_GET_TC_FLAG_INDEX(&hdma));
}
