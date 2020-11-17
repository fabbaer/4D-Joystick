/*******************************************************************************
* @file         : hal_timebase.c
* @project      : 4D-Joystick, Joystick-Unit
* @author       : Fabian Baer
* @brief        : Configures Timer2 to be the time base for the HAL
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"
#include "stm32l4xx_hal_tim.h"
 

/* Variables -----------------------------------------------------------------*/
TIM_HandleTypeDef        htim2; 


/* Code ----------------------------------------------------------------------*/
HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority) {
  RCC_ClkInitTypeDef clkconfig;
  uint32_t uwTimclock = 0;
  uint32_t uwPrescalerValue = 0;
  uint32_t pFLatency;

  //Enable Timer2-IRQ
  HAL_NVIC_SetPriority(TIM2_IRQn, TickPriority, 0);
  HAL_NVIC_EnableIRQ(TIM2_IRQn);

  //Enable clock
  __HAL_RCC_TIM2_CLK_ENABLE();

  //Calculate timer values to get a 1ms interrupt
  HAL_RCC_GetClockConfig(&clkconfig, &pFLatency);
  uwTimclock = HAL_RCC_GetPCLK1Freq();
  uwPrescalerValue = (uint32_t) ((uwTimclock / 1000000) - 1);

  //Init timer
  htim2.Instance = TIM2;
  htim2.Init.Period = (1000000 / 1000) - 1;
  htim2.Init.Prescaler = uwPrescalerValue;
  htim2.Init.ClockDivision = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  if (HAL_TIM_Base_Init(&htim2) == HAL_OK) {
    return HAL_TIM_Base_Start_IT(&htim2);
  }

  return HAL_ERROR;
}

void HAL_SuspendTick(void) {
  __HAL_TIM_DISABLE_IT(&htim2, TIM_IT_UPDATE);
}

void HAL_ResumeTick(void) {
  __HAL_TIM_ENABLE_IT(&htim2, TIM_IT_UPDATE);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
  if (htim->Instance == TIM2) {
    HAL_IncTick();
  }
}

void TIM2_IRQHandler(void) {
  HAL_TIM_IRQHandler(&htim2);
}

