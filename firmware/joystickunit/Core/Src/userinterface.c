/*******************************************************************************
* @file         : userinterface.c
* @project      : 4D-Joystick, Joystick-Unit
* @author       : Fabian Baer
* @brief        : Handles user buttons and display
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <string.h>
#include <cmsis_os.h>
#include <stm32l4xx_hal.h>
#include <board.h>
#include <system.h>
#include <userinterface.h>
#include <buttons.h>
#include <configHandler.h>
#include <spiDMA.h>
#include <u8g2.h>
#include <remoteunit.h>


/* Defines -------------------------------------------------------------------*/
#define CHANGE_CONFIG_TIMEOUT   3000


/* Prototypes ----------------------------------------------------------------*/
void ui_task(void const *argument);
static inline uint32_t ui_incrementConfig(uint32_t currentConfig, bool positivIncrement);
static inline void ui_updateDisplayIfRequired(uint32_t currentConfig);
static uint8_t ui_u8g2_spiInterface(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
static uint8_t ui_u8g2_halInterface(U8X8_UNUSED u8x8_t *u8x8, U8X8_UNUSED uint8_t msg,
    U8X8_UNUSED uint8_t arg_int, U8X8_UNUSED void *arg_ptr);


/* Variables -----------------------------------------------------------------*/
static osThreadId htask_ui;
static osMessageQId rotaryEncoderMsgBox = NULL;
static u8g2_t u8g2;
static SPI_DMA_HandleTypeDef hspi;
static SystemConfiguration_t* sysConfig;
static Configuration_t* configurations;
static bool flag_refreshDisplay;
extern uint32_t system_watchdog_uiTask;

static const unsigned char ui_symbol_usb[] = { 0x00, 0x00, 0x01, 0x80, 0x03,
    0xc0, 0x01, 0x80, 0x09, 0x80, 0x1d, 0xb8, 0x1d, 0xb8, 0x19, 0x98, 0x09,
    0x98, 0x0f, 0xf0, 0x07, 0xe0, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x03,
    0xc0, 0x01, 0x80 };

static const unsigned char ui_symbol_remote[] = { 0x20, 0x08, 0x49, 0x24, 0x53,
    0x94, 0x57, 0xD4, 0x53, 0x94, 0x49, 0x24, 0x21, 0x08, 0x01, 0x00, 0x7F,
    0xFE, 0xFE, 0x7F, 0xF7, 0xEF, 0xE3, 0xC7, 0xC1, 0x83, 0xE3, 0xC7, 0xF7,
    0xEF, 0x7F, 0xFE };

static const unsigned char ui_symbol_power[] = { 0x01, 0xf0, 0x01, 0xf0, 0x03,
    0xe0, 0x03, 0xc0, 0x07, 0xc0, 0x07, 0xf0, 0x0f, 0xf0, 0x0f, 0xe0, 0x0f,
    0xc0, 0x03, 0x80, 0x03, 0x00, 0x07, 0x00, 0x06, 0x00, 0x0c, 0x00, 0x08,
    0x00, 0x00, 0x00 };

static const unsigned char ui_symbol_teacher[] = { 0x03, 0xc0, 0x0f, 0xf0, 0x0e,
    0x70, 0x1c, 0x38, 0x18, 0x18, 0x18, 0x18, 0x1f, 0xf8, 0x3f, 0xfc, 0x3f,
    0xfc, 0x3e, 0x7c, 0x3e, 0x7c, 0x3e, 0x7c, 0x3e, 0x7c, 0x3e, 0x7c, 0x3f,
    0xfc, 0x3f, 0xfc};

/* Code ----------------------------------------------------------------------*/

/*******************************************************************************
 * Initializes all peripherals needed for user-interface (display + buttons).
 *
 * @return nothing
 *******************************************************************************/
void ui_init( void ) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  //Init flags
  flag_refreshDisplay = true;

  //Init GPIOs
  __GPIOB_CLK_ENABLE();
  __GPIOC_CLK_ENABLE();
  HAL_GPIO_WritePin(DISP_CS_Port, DISP_CS_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(DISP_DC_Port, DISP_DC_Pin, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = DISP_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_MEDIUM;
  HAL_GPIO_Init(DISP_CS_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = DISP_DC_Pin;
  HAL_GPIO_Init(DISP_DC_Port, &GPIO_InitStruct);

  //Init SPI
  hspi.edge = SPI_DMA_EDGE_FIRST;
  hspi.polarity = SPI_DMA_POLARITY_HIGH;
  hspi.speed = 2000000;
  if(spiDMA_init(&hspi) != HAL_OK) {
    system_errorHandler();
  }

  //Init u8g2-library and display
  u8g2_Setup_ssd1306_128x64_noname_f(&u8g2, U8G2_R0, ui_u8g2_spiInterface, ui_u8g2_halInterface);
  u8g2_InitDisplay(&u8g2);
  u8g2_SetPowerSave(&u8g2, 0);
}


/*******************************************************************************
 * Sets the source message queue for the rotary encoder events.
 *
 * @param msgQueue
 * @return nothing
 *******************************************************************************/
void ui_setRotaryEncoderMQ( osMessageQId msgQueue ) {
  rotaryEncoderMsgBox = msgQueue;
}


/*******************************************************************************
 * Forces the display to refresh at the next task execution.
 *
 * @return nothing
 *******************************************************************************/
void ui_updateDisplay( void ) {
  flag_refreshDisplay = true;
}


/*******************************************************************************
 * Initializes the task.
 *
 * @param priority The task priority
 * @return nothing
 *******************************************************************************/
void ui_setupTask( osPriority priority ) {
  osThreadDef(uiTask, ui_task, priority, 0, 1024);
  htask_ui = osThreadCreate(osThread(uiTask), NULL);
}


/*******************************************************************************
 * The UI-task.
 *
 * @return nothing
 *******************************************************************************/
void ui_task(void const *argument) {
  (void)argument;
  uint32_t displayedConfig = 0;
  uint32_t displayedConfigTimeout = 0;
  RotaryEncoderMessage_t msg;

  sysConfig = configHandler_getSystemConfig();
  configurations = configHandler_getConfigs();

  while(1) {
    if(displayedConfigTimeout < HAL_GetTick()) {
      displayedConfig = sysConfig->currentSlot;
    }

    while(osMessageWaiting(rotaryEncoderMsgBox) > 0) {
      msg.event = osMessageGet(rotaryEncoderMsgBox, osWaitForever).value.v;
      switch(msg.event) {
        case rotaryEncoder_pressed:
          configHandler_loadConfig(displayedConfig);
          break;

        case rotaryEncoder_clockwise:
          displayedConfig = ui_incrementConfig(displayedConfig, true);
          displayedConfigTimeout = HAL_GetTick() + CHANGE_CONFIG_TIMEOUT;
          break;

        case rotaryEncoder_counterclockwise:
          displayedConfig = ui_incrementConfig(displayedConfig, false);
          displayedConfigTimeout = HAL_GetTick() + CHANGE_CONFIG_TIMEOUT;
          break;

        case rotaryEncoder_none:
        case rotaryEncoder_released:
        default:
          break;
      }
    }

    ui_updateDisplayIfRequired(displayedConfig);

    system_watchdog_uiTask++;
    osDelay(100);
  }
}


/*******************************************************************************
 * Searches for the next active slot, starting from the current slot.
 *
 * @param currentConfig The number of the config to start search from.
 * @param positiveIncrement Next config if true, last config if false
 * @return The next/last avaialbe config
 *******************************************************************************/
static inline uint32_t ui_incrementConfig(uint32_t currentConfig, bool positivIncrement) {
  int cur = currentConfig;
  int inc = positivIncrement ? 1 : -1;

  do {
    cur += inc;
    if(cur > 31) {
      cur = 0;
    } else if(cur < 0) {
      cur = 31;
    }
  } while(!((sysConfig->activeSlots) & (1 << cur)));

  return cur;
}


/*******************************************************************************
 * Check if display needs an update and updates the display.
 *
 * @param currentConfig The number of the current loaded configuration.
 * @return nothing
 *******************************************************************************/
static inline void ui_updateDisplayIfRequired( uint32_t currentConfig ) {
  uint32_t strWidth;
  char slotBuf[] = "Slot 00";

  static uint32_t displayedConfig = 0xFF;
  static bool old_usbConnected = false;
  static bool old_remoteConnected = false;
  static bool old_poweredViaUSB = false;
  static bool old_teacherMode = false;
  static uint8_t blink = 3;
  static uint8_t continuouseRefresh = 50;

  bool usbConnected = system_isUSBConnected();
  bool remoteConnected = system_isRemoteConnected();
  bool poweredViaUSB = system_isPoweredViaUSB();
  bool teacherMode = remoteunit_isTeachermodeActive();

  //Refresh display every 5s, even if nothing was changed
  if(continuouseRefresh == 0) {
    continuouseRefresh = 50;
    flag_refreshDisplay = true;
  }
  continuouseRefresh--;

  if (displayedConfig != currentConfig || old_usbConnected != usbConnected
      || old_remoteConnected != remoteConnected || displayedConfig != sysConfig->currentSlot
      || old_poweredViaUSB != poweredViaUSB || flag_refreshDisplay
      || old_teacherMode != teacherMode || blink < 3) {
    flag_refreshDisplay = false;

    //Get the configuration
    Configuration_t* config = &configurations[currentConfig];

    //Setup display
    u8g2_ClearBuffer(&u8g2);

    //Print current config
    u8g2_SetFont(&u8g2, u8g2_font_helvB18_tf);
    strWidth = u8g2_GetStrWidth(&u8g2, (char*)config->name);
    if(strWidth > 128 || strlen((char*)config->name) > 10) {
      u8g2_SetFont(&u8g2, u8g2_font_helvB14_tf);
      strWidth = u8g2_GetStrWidth(&u8g2, (char*)config->name);
      if(strWidth > 128) {
        u8g2_SetFont(&u8g2, u8g2_font_helvB10_tf);
        strWidth = u8g2_GetStrWidth(&u8g2, (char*)config->name);
      }
    }
    u8g2_DrawStr(&u8g2, (128-strWidth)/2, 40, (char*)config->name);

    //Print slot number, blink if slot is not loaded
    if(displayedConfig != sysConfig->currentSlot && blink < 2) {
      blink++;
    } else {
      u8g2_SetFont(&u8g2, u8g2_font_helvB10_tf);
      if(currentConfig < 10) {
        slotBuf[5] = currentConfig + '0';
        slotBuf[6] = '\0';
      } else {
        slotBuf[6] = (currentConfig % 10) + '0';
        slotBuf[5] = (currentConfig / 10) + '0';
      }
      strWidth = u8g2_GetStrWidth(&u8g2, slotBuf);
      u8g2_DrawStr(&u8g2, (128-strWidth)/2, 60, slotBuf);

      if(blink > 3 && displayedConfig != sysConfig->currentSlot) {
        blink = 0;
      } else if(displayedConfig != sysConfig->currentSlot) {
        blink++;
      } else {
        blink = 3;
      }
    }

    //Display connected interfaces
    if(usbConnected && remoteConnected) {
      u8g2_DrawBitmap(&u8g2, 0, 0, 2, 16, ui_symbol_usb);
      u8g2_DrawBitmap(&u8g2, 20, 0, 2, 16, ui_symbol_remote);
    } else if(usbConnected) {
      u8g2_DrawBitmap(&u8g2, 0, 0, 2, 16, ui_symbol_usb);
    } else if(remoteConnected) {
      u8g2_DrawBitmap(&u8g2, 0, 0, 2, 16, ui_symbol_remote);
    }

    //Display teacher-mode
    if(teacherMode && remoteConnected) {
      u8g2_DrawBitmap(&u8g2, 56, 0, 2, 16, ui_symbol_teacher);
    }

    //Display power-source
    u8g2_DrawBitmap(&u8g2, 128-16-16, 0, 2, 16, ui_symbol_power);
    if(poweredViaUSB) {
      u8g2_DrawBitmap(&u8g2, 128-16, 0, 2, 16, ui_symbol_usb);
    } else {
      u8g2_DrawBitmap(&u8g2, 128-16, 0, 2, 16, ui_symbol_remote);
    }

    //Update display
    u8g2_SendBuffer(&u8g2);

    //Store current display states
    displayedConfig = currentConfig;
    old_usbConnected = usbConnected;
    old_remoteConnected = remoteConnected;
    old_poweredViaUSB = poweredViaUSB;
    old_teacherMode = teacherMode;
  }
}


/*******************************************************************************
 * HAL-Interface for u8g2-library.
 *
 * @return nothing
 *******************************************************************************/
uint8_t ui_u8g2_halInterface(U8X8_UNUSED u8x8_t *u8x8, U8X8_UNUSED uint8_t msg,
    U8X8_UNUSED uint8_t arg_int, U8X8_UNUSED void *arg_ptr) {
  switch (msg) {
    case U8X8_MSG_GPIO_AND_DELAY_INIT:
      break;
    case U8X8_MSG_DELAY_MILLI:
      if(osKernelRunning()) {
        osDelay(arg_int);
      } else {
        HAL_Delay(arg_int);
      }
      break;
    case U8X8_MSG_GPIO_DC:
      spiDMA_waitTillReady();
      HAL_GPIO_WritePin(DISP_DC_Port, DISP_DC_Pin, arg_int);
      break;
    case U8X8_MSG_GPIO_RESET:
      break;
    default:
      return 0;
  }
  return 1;
}


/*******************************************************************************
 * SPI-Interface for u8g2-library.
 *
 * @return nothing
 *******************************************************************************/
uint8_t ui_u8g2_spiInterface(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
  (void)u8x8;

  switch (msg) {
    case U8X8_MSG_BYTE_SEND:
      spiDMA_transmit(&hspi, (uint8_t *) arg_ptr, arg_int);
      break;
    case U8X8_MSG_BYTE_INIT:
      break;
    case U8X8_MSG_BYTE_SET_DC:
      spiDMA_waitTillReady();
      HAL_GPIO_WritePin(DISP_DC_Port, DISP_DC_Pin, arg_int);
      break;
    case U8X8_MSG_BYTE_START_TRANSFER:
      spiDMA_waitTillReady();
      HAL_GPIO_WritePin(DISP_CS_Port, DISP_CS_Pin, GPIO_PIN_RESET);
      break;
    case U8X8_MSG_BYTE_END_TRANSFER:
      spiDMA_waitTillReady();
      HAL_GPIO_WritePin(DISP_CS_Port, DISP_CS_Pin, GPIO_PIN_SET);
      break;
    default:
      return 0;
  }
  return 1;
}
