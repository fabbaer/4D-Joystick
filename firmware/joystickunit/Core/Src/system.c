/*******************************************************************************
* @file         : system.c
* @project      : 4D-Joystick, Joystick-Unit
* @author       : Fabian Baer
* @brief        : General system tools
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include <stm32l4xx_hal.h>
#include <cmsis_os.h>
#include <board.h>
#include <system.h>
#include <configHandler.h>
#include <adc.h>


/* Defines -------------------------------------------------------------------*/
#define VECT_TAB_OFFSET   0x00
#define WD_BUTTON_MIN_EX  60
#define WD_REMOTE_MIN_EX  30
#define WD_UI_MIN_EX      1


/* Prototypes ----------------------------------------------------------------*/
static void system_setupClock( void );
void system_task(void const *argument);


/* Variables -----------------------------------------------------------------*/
uint32_t SystemCoreClock = 4000000U;
const uint8_t AHBPrescTable[16] = { 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 1U, 2U, 3U,
    4U, 6U, 7U, 8U, 9U };
const uint8_t APBPrescTable[8] = { 0U, 0U, 0U, 0U, 1U, 2U, 3U, 4U };
const uint32_t MSIRangeTable[12] = { 100000U, 200000U, 400000U, 800000U,
    1000000U, 2000000U, 4000000U, 8000000U, 16000000U, 24000000U, 32000000U,
    48000000U };
static osThreadId htask_system;
static uint32_t usbVoltage_mV = 0;
static uint32_t rj12Voltage_mV = 0;
static uint32_t supplyVoltage_lsb = 0;

uint32_t system_watchdog_buttonTask = 0;
uint32_t system_watchdog_uiTask = 0;
uint32_t system_watchdog_remoteunitTask = 0;

#ifdef ENABLE_WATCHDOG
static IWDG_HandleTypeDef hiwdg;
#endif


/* Code ----------------------------------------------------------------------*/

/*******************************************************************************
 * Initializes all general system resources:
 *  - clocks
 *  - debug LED
 *  - unused GPIOs
 *  - ADCs
 *  - Watchdog
 *
 * @return nothing
 *******************************************************************************/
void system_init( void ) {
  GPIO_InitTypeDef GPIO_InitStruct = { 0 };

  //Init clocks
  system_setupClock();

  //System interrupt init
  __HAL_RCC_SYSCFG_CLK_ENABLE();
  __HAL_RCC_PWR_CLK_ENABLE();
  HAL_NVIC_SetPriority(PendSV_IRQn, 15, 0);

  //Enable clocks
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  //Setup GPIOs
  HAL_GPIO_WritePin(GPIOB, DEBUG_LED_Pin, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = DEBUG_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = SPARE2_Pin | SPARE4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = SPARE3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = SPARE6_Pin | SPARE1_Pin | SPARE5_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = NC1_Pin | NC2_Pin | NC3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = NC4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  //Init ADC
  adc1_init();
  adc2_init();

  //Enable watchdog for setup (1s)
#ifdef ENABLE_WATCHDOG
  hiwdg.Instance = IWDG;
  hiwdg.Init.Prescaler = IWDG_PRESCALER_32;
  hiwdg.Init.Window = 1000;
  hiwdg.Init.Reload = 1000;
  if (HAL_IWDG_Init(&hiwdg) != HAL_OK) {
    system_errorHandler();
  }
#endif
}


/*******************************************************************************
 * Initializes the system clocks
 *
 * @return nothing
 *******************************************************************************/
static void system_setupClock( void ) {
  RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
  RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };
  RCC_PeriphCLKInitTypeDef PeriphClkInit = { 0 };

  ///Init the CPU, AHB and APB busses clocks
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI | RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 40;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    system_errorHandler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
      | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV8;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK) {
    system_errorHandler();
  }

  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1 | RCC_PERIPHCLK_I2C1
      | RCC_PERIPHCLK_USB | RCC_PERIPHCLK_ADC;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_PCLK1;
  PeriphClkInit.AdcClockSelection = RCC_ADCCLKSOURCE_PLLSAI1;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLLSAI1;
  PeriphClkInit.PLLSAI1.PLLSAI1Source = RCC_PLLSOURCE_MSI;
  PeriphClkInit.PLLSAI1.PLLSAI1M = 1;
  PeriphClkInit.PLLSAI1.PLLSAI1N = 24;
  PeriphClkInit.PLLSAI1.PLLSAI1P = RCC_PLLP_DIV7;
  PeriphClkInit.PLLSAI1.PLLSAI1Q = RCC_PLLQ_DIV2;
  PeriphClkInit.PLLSAI1.PLLSAI1R = RCC_PLLR_DIV2;
  PeriphClkInit.PLLSAI1.PLLSAI1ClockOut = RCC_PLLSAI1_48M2CLK | RCC_PLLSAI1_ADC1CLK;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
    system_errorHandler();
  }

  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK) {
    system_errorHandler();
  }
}


/*******************************************************************************
 * Basic configuration of controller. This function is called before main() is
 * started.
 *
 * @return nothing
 *******************************************************************************/
void system_configureController(void) {
  RCC->CR |= RCC_CR_MSION;
  RCC->CFGR = 0x00000000U;
  RCC->CR &= 0xEAF6FFFFU;
  RCC->PLLCFGR = 0x00001000U;
  RCC->CR &= 0xFFFBFFFFU;
  RCC->CIER = 0x00000000U;
  SCB->VTOR = FLASH_BASE | VECT_TAB_OFFSET;
}


/*******************************************************************************
 * Initializes the task.
 *
 * @param priority The task priority
 * @return nothing
 *******************************************************************************/
void system_setupTask(osPriority priority) {
  osThreadDef(systemTask, system_task, priority, 0, configMINIMAL_STACK_SIZE);
  htask_system = osThreadCreate(osThread(systemTask), NULL);
}


/*******************************************************************************
 * The System-task.
 *
 * @return nothing
 *******************************************************************************/
void system_task(void const *argument) {
  (void)argument;
  uint32_t adcResults[3];

  //Change watchdog for main operation (min 150ms, max 300ms)
#ifdef ENABLE_WATCHDOG
  hiwdg.Instance = IWDG;
  hiwdg.Init.Prescaler = IWDG_PRESCALER_32;
  hiwdg.Init.Window = 150;
  hiwdg.Init.Reload = 300;
  if (HAL_IWDG_Init(&hiwdg) != HAL_OK) {
    system_errorHandler();
  }
#endif

  while(1) {
    //Get ADC values
    adc2_start();
    adc2_getADC(adcResults);

    //Calculate voltages
    usbVoltage_mV = (adcResults[0] * 18810) / 4096;
    rj12Voltage_mV = (adcResults[1] * 2 * 3300) / 4096;
    supplyVoltage_lsb = adcResults[2];

    //Set debug-LED
    HAL_GPIO_TogglePin(DEBUG_LED_Port, DEBUG_LED_Pin);

    osDelay(250);

    //Refresh watchdog
#ifdef ENABLE_WATCHDOG
    HAL_IWDG_Refresh(&hiwdg);
#endif

    //Check if all tasks are running
    if(system_watchdog_buttonTask < WD_BUTTON_MIN_EX || system_watchdog_uiTask < WD_UI_MIN_EX
        || (system_watchdog_remoteunitTask < WD_REMOTE_MIN_EX &&
        configHandler_getCurrentConfig()->type == configType_remoteunit)) {
      NVIC_SystemReset();
    } else {
      system_watchdog_buttonTask = 0;
      system_watchdog_uiTask = 0;
      system_watchdog_remoteunitTask = 0;
    }
  }
}


/*******************************************************************************
 * Check if USB is connected (USB voltage is available)
 *
 * @return true If USB-voltage is >4V
 *******************************************************************************/
bool system_isUSBConnected(void) {
  return (usbVoltage_mV > 4000);
}


/*******************************************************************************
 * Check if remote-unit is connected (remote-unit voltage is available)
 *
 * @return true If remote-voltage is >4V
 *******************************************************************************/
bool system_isRemoteConnected(void) {
  return (rj12Voltage_mV > 4000);
}


/*******************************************************************************
 * Check if power-source is USB.
 *
 * @return true if powered via USB
 *******************************************************************************/
bool system_isPoweredViaUSB(void) {
  return (usbVoltage_mV > 4360);
}


/*******************************************************************************
 * Get the P5V-voltage.
 *
 * @return the voltage (VLSB not Voltage)
 *******************************************************************************/
uint32_t system_getSupplyVoltage(void) {
  return supplyVoltage_lsb;
}

/*******************************************************************************
 * Forces the processor to stop everything and loads the default
 * stm32-bootloader.
 *
 * @return nothing
 *******************************************************************************/
void system_checkBootloader(void) {
  uint32_t data = 0;
  RTC_HandleTypeDef RtcHandle;
  RtcHandle.Instance = RTC;
  data = HAL_RTCEx_BKUPRead(&RtcHandle, RTC_BKP_DR0);

  if(data == 0xABEF05AA) {
    //Delete pattern from backup register
    __HAL_RCC_PWR_CLK_ENABLE();
    system_setupClock();
    __HAL_RCC_RTC_ENABLE();
    HAL_PWR_EnableBkUpAccess();
    HAL_RTCEx_BKUPWrite(&RtcHandle, RTC_BKP_DR0, 0xFFFFFFFF);
    HAL_PWR_DisableBkUpAccess();

    //Jump to bootloader
    void (*SysMemBootJump)(void);
    __disable_irq();
    __DSB();
    __HAL_SYSCFG_REMAPMEMORY_SYSTEMFLASH();
    __DSB();
    __ISB();
    SysMemBootJump = (void (*)(void)) (*((uint32_t *)(0x1FFF0000 + 4)));
    __set_MSP(*(volatile uint32_t*) 0x1FFF0000);

    SysMemBootJump();
  }
}


/*******************************************************************************
 * Forces the processor to stop everything and loads the default
 * stm32-bootloader.
 *
 * @return nothing
 *******************************************************************************/
void system_loadBootloader(void) {
  //Write pattern to backup register and restart
  RTC_HandleTypeDef RtcHandle;
  RtcHandle.Instance = RTC;
  HAL_PWR_EnableBkUpAccess();
  HAL_RTCEx_BKUPWrite(&RtcHandle, RTC_BKP_DR0, 0xABEF05AA);
  HAL_PWR_DisableBkUpAccess();

  NVIC_SystemReset();
}


/*******************************************************************************
 * Can be called if errors occure.
 *
 * @return nothing
 *******************************************************************************/
void system_errorHandler( void ) {
  NVIC_SystemReset();
}


/*******************************************************************************
 * All following functions handle faults. They are called via interrupts.
 *******************************************************************************/
void NMI_Handler(void) {
  //system_debugMessage(DEBUG_PREFIX "NMI interrupt");
  NVIC_SystemReset();
  while(1);
}

void HardFault_Handler(void) {
  //system_debugMessage(DEBUG_PREFIX "HardFault interrupt");
  NVIC_SystemReset();
  while(1);
}

void MemManage_Handler(void) {
  //system_debugMessage(DEBUG_PREFIX "MemManager interrupt");
  NVIC_SystemReset();
  while(1);
}

void BusFault_Handler(void) {
  //system_debugMessage(DEBUG_PREFIX "BusFault interrupt");
  NVIC_SystemReset();
  while(1);
}

void UsageFault_Handler(void) {
  //system_debugMessage(DEBUG_PREFIX "UsageFault interrupt");
  NVIC_SystemReset();
  while(1);
}

void DebugMon_Handler(void) {
  //do nothing
}
