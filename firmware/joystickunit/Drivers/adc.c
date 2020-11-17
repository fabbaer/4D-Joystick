/*******************************************************************************
* @file         : adc.c
* @project      : 4D-Joystick, Joystick-Unit
* @author       : Fabian Baer
* @brief        : ADC abstraction layer
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include <stm32l4xx_hal.h>
#include <cmsis_os.h>
#include <board.h>
#include <system.h>
#include <adc.h>


/* Defines -------------------------------------------------------------------*/
#define ADC_SAMPLE_TIME   ADC_SAMPLETIME_47CYCLES_5


/* Variables -----------------------------------------------------------------*/
static ADC_HandleTypeDef hadc1;
static ADC_HandleTypeDef hadc2;
static DMA_HandleTypeDef hdma2_3;
static DMA_HandleTypeDef hdma2_4;
static uint32_t adc1_results[4];
static uint32_t adc2_results[3];
static uint32_t adc1_convFinishFlag = 0;
static uint32_t adc2_convFinishFlag = 0;


/* Code ----------------------------------------------------------------------*/

/*******************************************************************************
 * Initializes peripherals for ADC1
 *
 * @return nothing
 *******************************************************************************/
void adc1_init( void ) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  ADC_ChannelConfTypeDef sConfig = {0};
  ADC_MultiModeTypeDef multimode = {0};

  //Init peripherals
  __HAL_RCC_ADC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_DMA2_CLK_ENABLE();

  //Init GPIOs
  GPIO_InitStruct.Pin = JOYSTICK_Z_Pin | JOYSTICK_X_Pin | JOYSTICK_Y_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG_ADC_CONTROL;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = JOYSTICK_W_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG_ADC_CONTROL;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  //Init ADC
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV16;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc1.Init.LowPowerAutoWait = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.NbrOfConversion = 4;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.DMAContinuousRequests = ENABLE;
  hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc1.Init.OversamplingMode = DISABLE;
  if (HAL_ADC_Init(&hadc1) != HAL_OK) {
    system_errorHandler();
  }

  //Enable independent mulitmode
  multimode.Mode = ADC_MODE_INDEPENDENT;
  if (HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimode) != HAL_OK) {
    system_errorHandler();
  }

  //Configure sequencer
  sConfig.Channel = JOYSTICK_X_Channel;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLE_TIME;
  sConfig.SingleDiff  = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
    system_errorHandler();
  }

  sConfig.Channel = JOYSTICK_Y_Channel;
  sConfig.Rank = ADC_REGULAR_RANK_2;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
    system_errorHandler();
  }

  sConfig.Channel = JOYSTICK_Z_Channel;
  sConfig.Rank = ADC_REGULAR_RANK_3;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
    system_errorHandler();
  }

  sConfig.Channel = JOYSTICK_W_Channel;
  sConfig.Rank = ADC_REGULAR_RANK_4;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
    system_errorHandler();
  }

  //Init DMA
  hdma2_3.Instance = DMA2_Channel3;
  hdma2_3.Init.Request = DMA_REQUEST_0;
  hdma2_3.Init.Direction = DMA_PERIPH_TO_MEMORY;
  hdma2_3.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma2_3.Init.MemInc = DMA_MINC_ENABLE;
  hdma2_3.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
  hdma2_3.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
  hdma2_3.Init.Mode = DMA_NORMAL;
  hdma2_3.Init.Priority = DMA_PRIORITY_HIGH;
  if (HAL_DMA_Init(&hdma2_3) != HAL_OK) {
    system_errorHandler();
  }

  __HAL_LINKDMA(&hadc1, DMA_Handle, hdma2_3);
  HAL_NVIC_SetPriority(DMA2_Channel3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Channel3_IRQn);
}


/*******************************************************************************
 * Initializes peripherals for ADC2
 *
 * @return nothing
 *******************************************************************************/
void adc2_init(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  ADC_ChannelConfTypeDef sConfig = {0};

  //Init peripherals
  __HAL_RCC_ADC_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  //Init GPIOs
  GPIO_InitStruct.Pin = SUPPLY_P5V_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG_ADC_CONTROL;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = USB_P5V_Pin | RJ12_P5V_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG_ADC_CONTROL;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  //Init ADC
  hadc2.Instance = ADC2;
  hadc2.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV16;
  hadc2.Init.Resolution = ADC_RESOLUTION_12B;
  hadc2.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc2.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc2.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc2.Init.LowPowerAutoWait = DISABLE;
  hadc2.Init.ContinuousConvMode = DISABLE;
  hadc2.Init.NbrOfConversion = 3;
  hadc2.Init.DiscontinuousConvMode = DISABLE;
  hadc2.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc2.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc2.Init.DMAContinuousRequests = ENABLE;
  hadc2.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc2.Init.OversamplingMode = DISABLE;
  if (HAL_ADC_Init(&hadc2) != HAL_OK) {
    system_errorHandler();
  }

  //Configure sequencer
  sConfig.Channel = USB_P5V_Channel;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLE_TIME;
  sConfig.SingleDiff  = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK) {
    system_errorHandler();
  }

  sConfig.Channel = RJ12_P5V_Channel;
  sConfig.Rank = ADC_REGULAR_RANK_2;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK) {
    system_errorHandler();
  }

  sConfig.Channel = SUPPLY_P5V_Channel;
  sConfig.Rank = ADC_REGULAR_RANK_3;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK) {
    system_errorHandler();
  }

  //Init DMA
  hdma2_4.Instance = DMA2_Channel4;
  hdma2_4.Init.Request = DMA_REQUEST_0;
  hdma2_4.Init.Direction = DMA_PERIPH_TO_MEMORY;
  hdma2_4.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma2_4.Init.MemInc = DMA_MINC_ENABLE;
  hdma2_4.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
  hdma2_4.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
  hdma2_4.Init.Mode = DMA_NORMAL;
  hdma2_4.Init.Priority = DMA_PRIORITY_HIGH;
  if (HAL_DMA_Init(&hdma2_4) != HAL_OK) {
    system_errorHandler();
  }

  __HAL_LINKDMA(&hadc2, DMA_Handle, hdma2_4);
  HAL_NVIC_SetPriority(DMA2_Channel4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Channel4_IRQn);
}


/*******************************************************************************
 * Starts a ADC1 conversion
 *
 * @return nothing
 *******************************************************************************/
void adc1_start( void ) {
  HAL_ADC_Start_DMA(&hadc1, adc1_results, 4);
}


/*******************************************************************************
 * Starts a ADC2 conversion
 *
 * @return nothing
 *******************************************************************************/
void adc2_start( void ) {
  HAL_ADC_Start_DMA(&hadc2, adc2_results, 3);
}


/*******************************************************************************
 * Callback function after ADC1 conversion finished
 *
 * @return nothing
 *******************************************************************************/
void DMA2_Channel3_IRQHandler( void ) {
  uint32_t* isrRegister  = ((uint32_t*)DMA2_BASE) + 0;
  uint32_t* ifcrRegister = ((uint32_t*)DMA2_BASE) + 2;

  if((*isrRegister & DMA_FLAG_HT3) != RESET) {
    *ifcrRegister = DMA_FLAG_HT3;
  }

  if((*isrRegister & DMA_FLAG_TC3) != RESET) {
    *ifcrRegister = DMA_FLAG_TC3;
    adc1_convFinishFlag = 1;
    HAL_ADC_Stop_DMA(&hadc1);
  }
}


/*******************************************************************************
 * Callback function after ADC2 conversion finished
 *
 * @return nothing
 *******************************************************************************/
void DMA2_Channel4_IRQHandler( void ) {
  uint32_t* isrRegister  = ((uint32_t*)DMA2_BASE) + 0;
  uint32_t* ifcrRegister = ((uint32_t*)DMA2_BASE) + 2;

  if((*isrRegister & DMA_FLAG_HT4) != RESET) {
    *ifcrRegister = DMA_FLAG_HT4;
  }

  if((*isrRegister & DMA_FLAG_TC4) != RESET) {
    *ifcrRegister = DMA_FLAG_TC4;
    adc2_convFinishFlag = 1;
    HAL_ADC_Stop_DMA(&hadc2);
  }
}


/*******************************************************************************
 * Blocking wait for ADC1 to be finished. Results are returned.
 *
 * @param values The results of the ADC conversion (array of size 4)
 * @return nothing
 *******************************************************************************/
void adc1_getADC( uint32_t* values ) {
  //Wait for flag
  while(adc1_convFinishFlag == 0);

  //Reset flag
  adc1_convFinishFlag = 0;

  //Copy result
  values[0] = adc1_results[0];
  values[1] = adc1_results[1];
  values[2] = adc1_results[2];
  values[3] = adc1_results[3];
}


/*******************************************************************************
 * Blocking wait for ADC2 to be finished. osDelay(1) is called during waiting.
 * Results are returned.
 *
 * @param values The results of the ADC conversion (array of size 4)
 * @return nothing
 *******************************************************************************/
void adc2_getADC( uint32_t* values ) {
  //Wait for flag
  while(adc2_convFinishFlag == 0) {
    osDelay(1);
  }

  //Reset flag
  adc2_convFinishFlag = 0;

  //Copy result
  values[0] = adc2_results[0];
  values[1] = adc2_results[1];
  values[2] = adc2_results[2];
}
