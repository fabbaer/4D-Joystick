/*******************************************************************************
* @file         : remoteIO.c
* @project      : 4D-Joystick, Remote-Unit
* @author       : Fabian Baer
* @brief        : Connections to remote
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include <stm32f4xx_hal.h>
#include <board.h>
#include <system.h>
#include <remoteIO.h>
#include <ioExpander.h>
#include <dac.h>


/* Defines -------------------------------------------------------------------*/
#define ENABLE_EXTERNAL_GPIOS
#define DEBUG_PREFIX      "RemoteIO - "
#define ADC_SAMPLE_TIME   ADC_SAMPLETIME_56CYCLES


/* Prototypes ----------------------------------------------------------------*/
static inline void remoteIO_initGPIOs(void);
static inline void remoteIO_initADC(void);
static inline void remoteIO_getGPIOs(RemoteIO_States_t* states);
static inline void remoteIO_getExternalGPIOs(RemoteIO_States_t* states);
static inline void remoteIO_startADC(RemoteIO_States_t* states);
static inline void remoteIO_waitForADCs(void);
static inline void remoteIO_setGPIOs(RemoteIO_States_t* states);
static inline void remoteIO_setExternalGPIOs(RemoteIO_States_t* states);
static inline void remoteIO_setDACs(RemoteIO_States_t* states);


/* Variables -----------------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma2_0;
static uint32_t adcConvFinishedFlag = 0;


/* Code ----------------------------------------------------------------------*/

/*******************************************************************************
 * Initializes all peripherals needed by the I/Os.
 *  - GPIOs
 *  - GPIO-Expander
 *  - ADCs
 *  - DACs
 *
 * @return nothing
 *******************************************************************************/
void remoteIO_init( void ) {
  remoteIO_initGPIOs();
  remoteIO_initADC();
#ifdef ENABLE_EXTERNAL_GPIOS
  ioExpander_init();
#endif
  dac_init();
}


/*******************************************************************************
 * Initializes the needed GPIOs
 *
 * @return nothing
 *******************************************************************************/
static inline void remoteIO_initGPIOs( void ) {
  GPIO_InitTypeDef gpioInitStruct = {0};

  //Set default value of outputs
  HAL_GPIO_WritePin(GPIOA, DO2_Pin | DO1_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOB, DO21_Pin | DO20_Pin | DO19_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOC, DO11_Pin | DO10_Pin | DO9_Pin | DO6_Pin | DO5_Pin
      | DO3_Pin | DO4_Pin | DO22_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOH, DO7_Pin | DO8_Pin, GPIO_PIN_SET);

  //Initialize outputs
  gpioInitStruct.Pin = DO2_Pin | DO1_Pin;
  gpioInitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  gpioInitStruct.Pull = GPIO_NOPULL;
  gpioInitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &gpioInitStruct);

  gpioInitStruct.Pin = DO21_Pin | DO20_Pin | DO19_Pin;
  gpioInitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  gpioInitStruct.Pull = GPIO_NOPULL;
  gpioInitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &gpioInitStruct);

  gpioInitStruct.Pin = DO11_Pin | DO10_Pin | DO9_Pin | DO6_Pin | DO5_Pin
      | DO3_Pin | DO4_Pin | DO22_Pin;
  gpioInitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  gpioInitStruct.Pull = GPIO_NOPULL;
  gpioInitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &gpioInitStruct);

  gpioInitStruct.Pin = DO7_Pin | DO8_Pin;
  gpioInitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  gpioInitStruct.Pull = GPIO_NOPULL;
  gpioInitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOH, &gpioInitStruct);

#ifndef USE_DEBUG_UART
  HAL_GPIO_WritePin(GPIOA, DO23_Pin, GPIO_PIN_SET);

  gpioInitStruct.Pin = DO23_Pin;
  gpioInitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  gpioInitStruct.Pull = GPIO_NOPULL;
  gpioInitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &gpioInitStruct);
#endif

  //Initialize inputs
  gpioInitStruct.Pin = DI7_Pin | DI6_Pin | DI5_Pin;
  gpioInitStruct.Mode = GPIO_MODE_INPUT;
  gpioInitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &gpioInitStruct);

  gpioInitStruct.Pin = DI2_Pin | DI4_Pin | DI3_Pin | DI21_Pin
      | DI22_Pin | DI1_Pin;
  gpioInitStruct.Mode = GPIO_MODE_INPUT;
  gpioInitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &gpioInitStruct);

  gpioInitStruct.Pin = DI11_Pin | DI10_Pin | DI9_Pin | DI8_Pin | DI19_Pin
      | DI20_Pin;
  gpioInitStruct.Mode = GPIO_MODE_INPUT;
  gpioInitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &gpioInitStruct);

#ifndef USE_DEBUG_UART
  gpioInitStruct.Pin = DI23_Pin;
  gpioInitStruct.Mode = GPIO_MODE_INPUT;
  gpioInitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &gpioInitStruct);
#endif
}


/*******************************************************************************
 * Initializes the needed ADCs
 *
 * @return nothing
 *******************************************************************************/
static inline void remoteIO_initADC(void) {
  ADC_ChannelConfTypeDef sConfig = {0};
  GPIO_InitTypeDef gpioInitStruct = {0};

  //Enable peripherals
  __HAL_RCC_ADC1_CLK_ENABLE();
  __DMA2_CLK_ENABLE();

  //Init GPIOs for ADC
  gpioInitStruct.Pin = AI3_Pin | AI4_Pin;
  gpioInitStruct.Mode = GPIO_MODE_ANALOG;
  gpioInitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &gpioInitStruct);

  gpioInitStruct.Pin = AI2_Pin | AI1_Pin;
  gpioInitStruct.Mode = GPIO_MODE_ANALOG;
  gpioInitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &gpioInitStruct);

  //Init ADC
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = ENABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 4;
  hadc1.Init.DMAContinuousRequests = ENABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK) {
    system_debugMessage(DEBUG_PREFIX "Cannot init ADC");
    NVIC_SystemReset();
  }

  //Configure sequencer
  sConfig.Channel = AI1_Channel;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLE_TIME;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
    system_debugMessage(DEBUG_PREFIX "Sequencer config error (1)");
    NVIC_SystemReset();
  }

  sConfig.Channel = AI2_Channel;
  sConfig.Rank = 2;
  sConfig.SamplingTime = ADC_SAMPLE_TIME;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
    system_debugMessage(DEBUG_PREFIX "Sequencer config error (2)");
    NVIC_SystemReset();
  }

  sConfig.Channel = AI3_Channel;
  sConfig.Rank = 3;
  sConfig.SamplingTime = ADC_SAMPLE_TIME;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
    system_debugMessage(DEBUG_PREFIX "Sequencer config error (3)");
    NVIC_SystemReset();
  }

  sConfig.Channel = AI4_Channel;
  sConfig.Rank = 4;
  sConfig.SamplingTime = ADC_SAMPLE_TIME;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
    system_debugMessage(DEBUG_PREFIX "Sequencer config error (4)");
    NVIC_SystemReset();
  }

  //Configure DMA
  hdma2_0.Instance = DMA2_Stream0;
  hdma2_0.Init.Channel  = DMA_CHANNEL_0;
  hdma2_0.Init.Direction = DMA_PERIPH_TO_MEMORY;
  hdma2_0.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma2_0.Init.MemInc = DMA_MINC_ENABLE;
  hdma2_0.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
  hdma2_0.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
  hdma2_0.Init.Mode = DMA_NORMAL;
  hdma2_0.Init.Priority = DMA_PRIORITY_HIGH;
  hdma2_0.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
  if (HAL_DMA_Init(&hdma2_0) != HAL_OK) {
    system_debugMessage(DEBUG_PREFIX "Cannot init DMA");
    NVIC_SystemReset();
  }

  __HAL_LINKDMA(&hadc1, DMA_Handle, hdma2_0);
  NVIC_SetPriority(DMA2_Stream0_IRQn, 1);
  NVIC_EnableIRQ(DMA2_Stream0_IRQn);
}


/*******************************************************************************
 * Reads all inputs and stores it the states struct
 *
 * @param states The input-state-struct to write data to
 * @return nothing
 *******************************************************************************/
void remoteIO_getStates(RemoteIO_States_t* states) {
  remoteIO_startADC(states);
  remoteIO_getGPIOs(states);
#ifdef ENABLE_EXTERNAL_GPIOS
  remoteIO_getExternalGPIOs(states);
#endif
  remoteIO_waitForADCs();
}


/*******************************************************************************
 * Writes to the outputs, according to the states-struct.
 *
 * @param states The data which should be written to the outputs
 * @return nothing
 *******************************************************************************/
void remoteIO_setStates(RemoteIO_States_t* states) {
  remoteIO_setGPIOs(states);
#ifdef ENABLE_EXTERNAL_GPIOS
  remoteIO_setExternalGPIOs(states);
#endif
  remoteIO_setDACs(states);
}


/*******************************************************************************
 * Reads in the GPIOs and stores there values to the state-struct
 *
 * @param states The state struct
 * @return nothing
 *******************************************************************************/
static inline void remoteIO_getGPIOs(RemoteIO_States_t* states) {
  states->digital[ 0] = HAL_GPIO_ReadPin( DI1_Port,  DI1_Pin );
  states->digital[ 1] = HAL_GPIO_ReadPin( DI2_Port,  DI2_Pin );
  states->digital[ 2] = HAL_GPIO_ReadPin( DI3_Port,  DI3_Pin );
  states->digital[ 3] = HAL_GPIO_ReadPin( DI4_Port,  DI4_Pin );
  states->digital[ 4] = HAL_GPIO_ReadPin( DI5_Port,  DI5_Pin );
  states->digital[ 5] = HAL_GPIO_ReadPin( DI6_Port,  DI6_Pin );
  states->digital[ 6] = HAL_GPIO_ReadPin( DI7_Port,  DI7_Pin );
  states->digital[ 7] = HAL_GPIO_ReadPin( DI8_Port,  DI8_Pin );
  states->digital[ 8] = HAL_GPIO_ReadPin( DI9_Port,  DI9_Pin );
  states->digital[ 9] = HAL_GPIO_ReadPin( DI10_Port, DI10_Pin);
  states->digital[10] = HAL_GPIO_ReadPin( DI11_Port, DI11_Pin);
  states->digital[18] = HAL_GPIO_ReadPin( DI19_Port, DI19_Pin);
  states->digital[19] = HAL_GPIO_ReadPin( DI20_Port, DI20_Pin);
  states->digital[20] = HAL_GPIO_ReadPin( DI21_Port, DI21_Pin);
  states->digital[21] = HAL_GPIO_ReadPin( DI22_Port, DI22_Pin);

#ifndef USE_DEBUG_UART
  states->digital[22] = HAL_GPIO_ReadPin( DI23_Port, DI23_Pin);
#else
  states->digital[22] = GPIO_PIN_SET;
#endif
}


/*******************************************************************************
 * Reads in the external GPIOs and stores there values to the state-struct
 *
 * @param states The state struct
 * @return nothing
 *******************************************************************************/
static inline void remoteIO_getExternalGPIOs(RemoteIO_States_t* states) {
  uint8_t data = ioExpander_getInputs();

  states->digital[11] = (data & DI12_Pin) ? GPIO_PIN_SET : GPIO_PIN_RESET;
  states->digital[12] = (data & DI13_Pin) ? GPIO_PIN_SET : GPIO_PIN_RESET;
  states->digital[13] = (data & DI14_Pin) ? GPIO_PIN_SET : GPIO_PIN_RESET;
  states->digital[14] = (data & DI15_Pin) ? GPIO_PIN_SET : GPIO_PIN_RESET;
  states->digital[15] = (data & DI16_Pin) ? GPIO_PIN_SET : GPIO_PIN_RESET;
  states->digital[16] = (data & DI17_Pin) ? GPIO_PIN_SET : GPIO_PIN_RESET;
  states->digital[17] = (data & DI18_Pin) ? GPIO_PIN_SET : GPIO_PIN_RESET;
  states->digital[23] = (data & DI24_Pin) ? GPIO_PIN_SET : GPIO_PIN_RESET;
}


/*******************************************************************************
 * Starts an ADC converstion with the state struct as destination.
 *
 * @param states The state struct
 * @return nothing
 *******************************************************************************/
static inline void remoteIO_startADC(RemoteIO_States_t* states) {
  HAL_ADC_Start_DMA(&hadc1, states->analog, 4);
}


/*******************************************************************************
 * DMA callback of ADC
 *
 * @return nothing
 *******************************************************************************/
void DMA2_Stream0_IRQHandler(DMA_HandleTypeDef* hdma) {
  uint32_t* isrRegister  = ((uint32_t*)DMA2_BASE) + 0;
  uint32_t* ifcrRegister = ((uint32_t*)DMA2_BASE) + 2;

  if((*isrRegister & DMA_FLAG_HTIF0_4) != RESET) {
    *ifcrRegister = DMA_FLAG_HTIF0_4;
  }

  if((*isrRegister & DMA_FLAG_TCIF0_4) != RESET) {
    *ifcrRegister = DMA_FLAG_TCIF0_4;
    adcConvFinishedFlag = 1;
    HAL_ADC_Stop_DMA(&hadc1);
  }
}


/*******************************************************************************
 * Blocking wait for ADCs to finish conversion.
 *
 * @return nothing
 *******************************************************************************/
static inline void remoteIO_waitForADCs( void ) {
  while(adcConvFinishedFlag == 0);

  //Reset flag
  adcConvFinishedFlag = 0;
}


/*******************************************************************************
 * Sets all GPIOs according to theire states in the state-vector.
 *
 * @param states The state struct
 * @return nothing
 *******************************************************************************/
static inline void remoteIO_setGPIOs(RemoteIO_States_t* states) {
  HAL_GPIO_WritePin( DO1_Port,  DO1_Pin,  states->digital[ 0] );
  HAL_GPIO_WritePin( DO2_Port,  DO2_Pin,  states->digital[ 1] );
  HAL_GPIO_WritePin( DO3_Port,  DO3_Pin,  states->digital[ 2] );
  HAL_GPIO_WritePin( DO4_Port,  DO4_Pin,  states->digital[ 3] );
  HAL_GPIO_WritePin( DO5_Port,  DO5_Pin,  states->digital[ 4] );
  HAL_GPIO_WritePin( DO6_Port,  DO6_Pin,  states->digital[ 5] );
  HAL_GPIO_WritePin( DO7_Port,  DO7_Pin,  states->digital[ 6] );
  HAL_GPIO_WritePin( DO8_Port,  DO8_Pin,  states->digital[ 7] );
  HAL_GPIO_WritePin( DO9_Port,  DO9_Pin,  states->digital[ 8] );
  HAL_GPIO_WritePin( DO10_Port, DO10_Pin, states->digital[ 9] );
  HAL_GPIO_WritePin( DO11_Port, DO11_Pin, states->digital[10] );
  HAL_GPIO_WritePin( DO19_Port, DO19_Pin, states->digital[18] );
  HAL_GPIO_WritePin( DO20_Port, DO20_Pin, states->digital[19] );
  HAL_GPIO_WritePin( DO21_Port, DO21_Pin, states->digital[20] );
  HAL_GPIO_WritePin( DO22_Port, DO22_Pin, states->digital[21] );

#ifndef USE_DEBUG_UART
  HAL_GPIO_WritePin(DO23_Port, DO23_Pin, states->digital[22]);
#else
  HAL_GPIO_WritePin(DO23_Port, DO23_Pin, GPIO_PIN_SET);
#endif
}


/*******************************************************************************
 * Sets all external GPIOs according to theire states in the state-vector.
 *
 * @param states The state struct
 * @return nothing
 *******************************************************************************/
static inline void remoteIO_setExternalGPIOs(RemoteIO_States_t* states) {
  uint8_t data = 0;

  data |= (states->digital[11] == GPIO_PIN_SET) ? DO12_Pin : 0x00;
  data |= (states->digital[12] == GPIO_PIN_SET) ? DO13_Pin : 0x00;
  data |= (states->digital[13] == GPIO_PIN_SET) ? DO14_Pin : 0x00;
  data |= (states->digital[14] == GPIO_PIN_SET) ? DO15_Pin : 0x00;
  data |= (states->digital[15] == GPIO_PIN_SET) ? DO16_Pin : 0x00;
  data |= (states->digital[16] == GPIO_PIN_SET) ? DO17_Pin : 0x00;
  data |= (states->digital[17] == GPIO_PIN_SET) ? DO18_Pin : 0x00;
  data |= (states->digital[23] == GPIO_PIN_SET) ? DO24_Pin : 0x00;

  ioExpander_setOutputs(data);
}


/*******************************************************************************
 * Sets all DACs according to theire states in the state-vector.
 *
 * @param states The state struct
 * @return nothing
 *******************************************************************************/
static inline void remoteIO_setDACs(RemoteIO_States_t* states) {
  dac_setAllChannels(states->analog[2], states->analog[3], states->analog[0], states->analog[1]);
}
