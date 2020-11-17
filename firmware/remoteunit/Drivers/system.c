/*******************************************************************************
* @file         : system.c
* @project      : 4D-Joystick, Remote-Unit
* @author       : Fabian Baer
* @brief        : General system tools
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include "board.h"
#include "system.h"


/* Defines -------------------------------------------------------------------*/
#define DEBUG_PREFIX        "Sys - "
#define VECT_TAB_OFFSET     0x00


/* Variables -----------------------------------------------------------------*/
uint32_t SystemCoreClock = 16000000;
const uint8_t AHBPrescTable[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9};
const uint8_t APBPrescTable[8]  = {0, 0, 0, 0, 1, 2, 3, 4};
IWDG_HandleTypeDef hiwdg;
UART_HandleTypeDef huart1;



/* Code ----------------------------------------------------------------------*/

/*******************************************************************************
 * Initializes all general system resources:
 *  - clocks
 *  - debug LED
 *  - debug UART
 *  - Watchdog
 *
 * @return nothing
 *******************************************************************************/
void system_init(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  //Enable all GPIO clocks
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();

  //Initialize debug-LED
  HAL_GPIO_WritePin(DEBUG_LED_Port, DEBUG_LED_Pin, GPIO_PIN_RESET);
  GPIO_InitStruct.Pin = DEBUG_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(DEBUG_LED_Port, &GPIO_InitStruct);

  //Init UART
#ifdef USE_DEBUG_UART
  __HAL_RCC_USART1_CLK_ENABLE();

  GPIO_InitStruct.Pin = DI23_Pin | DO23_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK) {
    system_debugMessage(DEBUG_PREFIX "Cannot init UART");
    NVIC_SystemReset();
  }
#endif

  //Init Watchdog (15ms)
#ifdef ENABLE_WATCHDOG
  hiwdg.Instance = IWDG;
  hiwdg.Init.Prescaler = IWDG_PRESCALER_4;
  hiwdg.Init.Reload = 120;
  if (HAL_IWDG_Init(&hiwdg) != HAL_OK) {
    system_debugMessage(DEBUG_PREFIX "Cannot init Watchdog");
    NVIC_SystemReset();
  }
#endif
}

void system_watchdogHandler( void ) {
#ifdef ENABLE_WATCHDOG
  HAL_IWDG_Refresh(&hiwdg);
#endif
}


/*******************************************************************************
 * Initializes the system clocks
 *
 * @return nothing
 *******************************************************************************/
void system_setupClock(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
  RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI
      | RCC_OSCILLATORTYPE_LSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    NVIC_SystemReset();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
      | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) {
    NVIC_SystemReset();
  }
}


/*******************************************************************************
 * Basic configuration of controller. This function is called before main() is
 * started.
 *
 * @return nothing
 *******************************************************************************/
void system_configureController(void) {
  RCC->CR |= (uint32_t)0x00000001;
  RCC->CFGR = 0x00000000;
  RCC->CR &= (uint32_t)0xFEF6FFFF;
  RCC->PLLCFGR = 0x24003010;
  RCC->CR &= (uint32_t)0xFFFBFFFF;
  RCC->CIR = 0x00000000;
  SCB->VTOR = FLASH_BASE | VECT_TAB_OFFSET;
}


/*******************************************************************************
 * Handles things like debug-LED and watchdog. Therefore, this function
 * should be called periodically from main program.
 *
 * @return nothing
 *******************************************************************************/
void system_handler(Joystickunit_State_t joyState) {
  uint32_t ledBlinkFreq = 0;

  //Reset Watchdog
#ifdef ENABLE_WATCHDOG
  HAL_IWDG_Refresh(&hiwdg);
#endif

  //Set debug-LED blink frequency
  switch(joyState) {
    case JOY_STATE_NOT_AVAILABLE:
      ledBlinkFreq = 1000;
      break;
    case JOY_STATE_OK:
      ledBlinkFreq = 500;
      break;
    default:
    case JOY_STATE_ERROR:
      ledBlinkFreq = 250;
      break;
  }

  //Set debug-LED
  if((HAL_GetTick()%ledBlinkFreq) < (ledBlinkFreq/2)) {
    HAL_GPIO_WritePin(DEBUG_LED_Port, DEBUG_LED_Pin, GPIO_PIN_SET);
  } else {
    HAL_GPIO_WritePin(DEBUG_LED_Port, DEBUG_LED_Pin, GPIO_PIN_RESET);
  }
}


#ifdef USE_DEBUG_UART
/*******************************************************************************
 * Returns the handle of the debug-UART-connection.
 *
 * @return The UART-handle
 *******************************************************************************/
UART_HandleTypeDef* system_getUartHandle( void ) {
  return &huart1;
}


/*******************************************************************************
 * Prints a debug message to debug-UART.
 * Do not use this function directly. Instead use the macro system_debugMessage.
 * Debug-UART can be disabled via a #define statement. If macro is used for
 * debug-messages, the message are automatically disabled.
 *
 * @return nothing
 *******************************************************************************/
void system_doNotUse_debugMessage(uint8_t* text, uint32_t size) {
  char buffer[12] = "[     .   ] ";
  uint32_t time = HAL_GetTick();
  buffer[9] = ((time % 10) / 1) + '0';
  buffer[8] = ((time % 100) / 10) + '0';
  buffer[7] = ((time % 1000) / 100) + '0';
  buffer[5] = ((time % 10000) / 1000) + '0';
  buffer[4] = ((time % 100000) / 10000) + '0';
  buffer[3] = ((time % 1000000) / 100000) + '0';
  buffer[2] = ((time % 10000000) / 1000000) + '0';
  buffer[1] = ((time % 100000000) / 10000000) + '0';

  HAL_UART_Transmit(&huart1, (uint8_t*)buffer, 12, 10);
  HAL_UART_Transmit(&huart1, text, size, 10);
  HAL_UART_Transmit(&huart1, (uint8_t*)"\n\r", 2, 10);
}
#endif


/*******************************************************************************
 * Handles the systick interrupt (forward to HAL).
 *
 * @return nothing
 *******************************************************************************/
void system_systickHandler( void ) {
  HAL_IncTick();
  HAL_SYSTICK_IRQHandler();
}


/*******************************************************************************
 * All following functions handle faults. They are called via interrupts.
 *******************************************************************************/
void NMI_Handler(void) {
  system_debugMessage(DEBUG_PREFIX "NMI interrupt");
  NVIC_SystemReset();
  while(1);
}

void HardFault_Handler(void) {
  system_debugMessage(DEBUG_PREFIX "HardFault interrupt");
  NVIC_SystemReset();
  while(1);
}

void MemManage_Handler(void) {
  system_debugMessage(DEBUG_PREFIX "MemManager interrupt");
  NVIC_SystemReset();
  while(1);
}

void BusFault_Handler(void) {
  system_debugMessage(DEBUG_PREFIX "BusFault interrupt");
  NVIC_SystemReset();
  while(1);
}

void UsageFault_Handler(void) {
  system_debugMessage(DEBUG_PREFIX "UsageFault interrupt");
  NVIC_SystemReset();
  while(1);
}

