/*******************************************************************************
 * @file         : remoteunit.c
 * @project      : 4D-Joystick, Joystick-Unit
 * @author       : Fabian Baer
 * @brief        : Handels all interactions with remote-unit
 ******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <cmsis_os.h>
#include <stm32l4xx_hal.h>
#include <board.h>
#include <system.h>
#include <remoteunit.h>
#include <configHandler.h>
#include <buttons.h>
#include <adc.h>


/* Defines -------------------------------------------------------------------*/
#define CRC_START_VALUE         0xA5


/* Macros --------------------------------------------------------------------*/
#define ADD_GPIO_BIT(gpioval, bitnum)   (((gpioval == GPIO_PIN_SET) ? 1 : 0) << bitnum)
#define EXTRACT_GPIO_VAL(data, bitnum)  ((data & (1<<bitnum)) ? GPIO_PIN_SET : GPIO_PIN_RESET)


/* Typedefs ------------------------------------------------------------------*/
typedef struct {
  //Digital
  bool teacherPort_sw3Pos;
  uint8_t teacherPort_ch1;
  uint8_t teacherPort_ch2;
  uint8_t axis_config[4];
  SysConf_Switch_t bb_config[4];
  uint8_t bb_ch1[4];
  uint8_t bb_ch2[4];

  //Analog
  Config_Analog_In_t aOut[4];
  int32_t k_num[4];
  int32_t k_den[4];
  int32_t d_low[4];
  int32_t d_high[4];
  int32_t threas_low[4];
  int32_t threas_high[4];
  int32_t middpoint_in[4];
  int32_t middpoint[4];
  bool inInverted[4];
  bool outInverted[4];
} RemUnit_Config_t;

typedef struct {
  uint32_t analog[4];
  GPIO_PinState digital[24];
} RemUnit_IOStates_t;

typedef enum {
  bbState_off = 0,
  bbState_on_1 = 1,
  bbState_on_2 = 2
} BuddyButton_State_t;


/* Prototypes ----------------------------------------------------------------*/
static void remUnit_task( void const *argument );
static void remUnit_loadConfig( RemUnit_Config_t* pConfig );
static inline void remUnit_getAxis( RemUnit_Config_t *pConfig, RemUnit_IOStates_t *pStates );
static inline void remUnit_getBuddyButtons( RemUnit_Config_t *pConfig,
    RemUnit_IOStates_t *pIOStates, BuddyButton_State_t *pBuddyStates );
static void remUnit_resetBuddyButtons( BuddyButton_State_t* pStates );
static inline void remUnit_packData( RemUnit_IOStates_t* data, uint8_t* package );
static inline void remUnit_unpackData( RemUnit_IOStates_t* pData, uint8_t* pPackage );
static uint8_t remUnit_calcCRC( uint8_t* pData, uint32_t len );
static inline bool remUnit_checkCRC( uint8_t* data, uint32_t len );


/* Variables -----------------------------------------------------------------*/
static SPI_HandleTypeDef hspi1;
extern uint32_t system_watchdog_remoteunitTask;
static osThreadId htask_remoteunit;
static osMessageQId buddyButtonsMsgBox = NULL;
static bool flag_reloadConfig = false;
static bool flag_terminateTask = false;
static bool flag_sendADC = false;
static bool flag_teacherMode = false;
static RemoteUnit_adcStates_t adcStates = {0};
static GPIO_TypeDef* const buddyButtons_ledPorts[4][2] = {
    {LED1_G_Port, LED1_R_Port},
    {LED2_G_Port, LED2_R_Port},
    {LED3_G_Port, LED3_R_Port},
    {LED4_G_Port, LED4_R_Port}};
static uint16_t const buddyButtons_ledPins[4][2] = {
    {LED1_G_Pin, LED1_R_Pin},
    {LED2_G_Pin, LED2_R_Pin},
    {LED3_G_Pin, LED3_R_Pin},
    {LED4_G_Pin, LED4_R_Pin}};
static uint8_t const crc8_table[] =   { 0x00, 0x31, 0x62, 0x53, 0xc4, 0xf5,
    0xa6, 0x97, 0xb9, 0x88, 0xdb, 0xea, 0x7d, 0x4c, 0x1f, 0x2e, 0x43, 0x72,
    0x21, 0x10, 0x87, 0xb6, 0xe5, 0xd4, 0xfa, 0xcb, 0x98, 0xa9, 0x3e, 0x0f,
    0x5c, 0x6d, 0x86, 0xb7, 0xe4, 0xd5, 0x42, 0x73, 0x20, 0x11, 0x3f, 0x0e,
    0x5d, 0x6c, 0xfb, 0xca, 0x99, 0xa8, 0xc5, 0xf4, 0xa7, 0x96, 0x01, 0x30,
    0x63, 0x52, 0x7c, 0x4d, 0x1e, 0x2f, 0xb8, 0x89, 0xda, 0xeb, 0x3d, 0x0c,
    0x5f, 0x6e, 0xf9, 0xc8, 0x9b, 0xaa, 0x84, 0xb5, 0xe6, 0xd7, 0x40, 0x71,
    0x22, 0x13, 0x7e, 0x4f, 0x1c, 0x2d, 0xba, 0x8b, 0xd8, 0xe9, 0xc7, 0xf6,
    0xa5, 0x94, 0x03, 0x32, 0x61, 0x50, 0xbb, 0x8a, 0xd9, 0xe8, 0x7f, 0x4e,
    0x1d, 0x2c, 0x02, 0x33, 0x60, 0x51, 0xc6, 0xf7, 0xa4, 0x95, 0xf8, 0xc9,
    0x9a, 0xab, 0x3c, 0x0d, 0x5e, 0x6f, 0x41, 0x70, 0x23, 0x12, 0x85, 0xb4,
    0xe7, 0xd6, 0x7a, 0x4b, 0x18, 0x29, 0xbe, 0x8f, 0xdc, 0xed, 0xc3, 0xf2,
    0xa1, 0x90, 0x07, 0x36, 0x65, 0x54, 0x39, 0x08, 0x5b, 0x6a, 0xfd, 0xcc,
    0x9f, 0xae, 0x80, 0xb1, 0xe2, 0xd3, 0x44, 0x75, 0x26, 0x17, 0xfc, 0xcd,
    0x9e, 0xaf, 0x38, 0x09, 0x5a, 0x6b, 0x45, 0x74, 0x27, 0x16, 0x81, 0xb0,
    0xe3, 0xd2, 0xbf, 0x8e, 0xdd, 0xec, 0x7b, 0x4a, 0x19, 0x28, 0x06, 0x37,
    0x64, 0x55, 0xc2, 0xf3, 0xa0, 0x91, 0x47, 0x76, 0x25, 0x14, 0x83, 0xb2,
    0xe1, 0xd0, 0xfe, 0xcf, 0x9c, 0xad, 0x3a, 0x0b, 0x58, 0x69, 0x04, 0x35,
    0x66, 0x57, 0xc0, 0xf1, 0xa2, 0x93, 0xbd, 0x8c, 0xdf, 0xee, 0x79, 0x48,
    0x1b, 0x2a, 0xc1, 0xf0, 0xa3, 0x92, 0x05, 0x34, 0x67, 0x56, 0x78, 0x49,
    0x1a, 0x2b, 0xbc, 0x8d, 0xde, 0xef, 0x82, 0xb3, 0xe0, 0xd1, 0x46, 0x77,
    0x24, 0x15, 0x3b, 0x0a, 0x59, 0x68, 0xff, 0xce, 0x9d, 0xac };


/* Code ----------------------------------------------------------------------*/

/*******************************************************************************
 * Initializes all hardware, which is related to the remote-unit:
 *  - GPIOs for SPI
 *  - GPIOs for LEDs
 *  - ADC
 *  - SPI
 *
 * @return nothing
 *******************************************************************************/
void remoteunit_init( void ) {
  GPIO_InitTypeDef GPIO_InitStruct = { 0 };

  //Enable Clocks
  __HAL_RCC_SPI1_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  //Init SPI-Pins
  HAL_GPIO_WritePin(RJ12_CS_Port, RJ12_CS_Pin, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = RJ12_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = RJ12_SCK_Pin | RJ12_MISO_Pin | RJ12_MOSI_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  //Init LED-Pins
  HAL_GPIO_WritePin(GPIOB, LED2_G_Pin | LED2_R_Pin | LED3_G_Pin | LED3_R_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOC, LED4_G_Pin | LED1_G_Pin | LED1_R_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOA, LED4_R_Pin, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = LED4_R_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LED2_G_Pin | LED2_R_Pin | LED3_G_Pin | LED3_R_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LED4_G_Pin | LED1_G_Pin | LED1_R_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  //Init SPI
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_LSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 15;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;

  if (HAL_SPI_Init(&hspi1) != HAL_OK) {
    system_errorHandler();
  }
}


/*******************************************************************************
 * Set the message queue, which provides status of the buddybuttons.
 *
 * @param msgQueue The buddybuttons message queue
 * @return nothing
 *******************************************************************************/
void remoteunit_setBuddyButtonsMQ( osMessageQId msgQueue ) {
  buddyButtonsMsgBox = msgQueue;
}


/*******************************************************************************
 * Initializes the task.
 *
 * @param priority The task priority
 * @return nothing
 *******************************************************************************/
void remoteunit_setupTask( osPriority priority ) {
  osThreadDef(remoteunitTask, remUnit_task, priority, 0, 512);
  htask_remoteunit = osThreadCreate(osThread(remoteunitTask), NULL);
}


/*******************************************************************************
 * Terminates the task (after the end of the next execution).
 *
 * @return nothing
 *******************************************************************************/
void remoteunit_terminateTask( void ) {
  flag_terminateTask = true;
}


/*******************************************************************************
 * The remote-unit-task. This task sets up all structures an take care of
 * communication with remote-unit (including read-in of all necessary data).
 *
 * @return nothing
 *******************************************************************************/
static void remUnit_task( void const *argument ) {
  (void)argument;
  RemUnit_Config_t currentConfig = {0};
  RemUnit_IOStates_t ioStates = {0};
  BuddyButton_State_t buddyStates[4] = {0};
  uint8_t rxData[10], txData[10];

  //Enable Remote-Unit
  HAL_GPIO_WritePin(RJ12_CS_Port, RJ12_CS_Pin, GPIO_PIN_SET);

  //Disable all flags
  flag_reloadConfig = false;
  flag_terminateTask = false;
  flag_sendADC = false;
  flag_teacherMode = false;

  //Check if buddybutton-message-queue is set
  if(buddyButtonsMsgBox == NULL) {
    system_errorHandler();
  }

  //Load configuration
  remUnit_loadConfig(&currentConfig);

  //Setup structs
  remUnit_resetBuddyButtons(buddyStates);
  for(uint32_t i = 0; i<24; i++) {
    ioStates.digital[i] = GPIO_PIN_SET;
  }
  for(uint32_t i = 0; i<4; i++) {
    ioStates.analog[i] = 0;
  }

  //Task loop
  while(1) {
    //Check if teacher mode is enabled
    if(currentConfig.teacherPort_sw3Pos) {
      if(currentConfig.teacherPort_ch1 != DIGITAL_PORT_NOT_USED &&
          ioStates.digital[currentConfig.teacherPort_ch1] == GPIO_PIN_RESET &&
          currentConfig.teacherPort_ch2 != DIGITAL_PORT_NOT_USED &&
          ioStates.digital[currentConfig.teacherPort_ch2] == GPIO_PIN_SET) {
        flag_teacherMode = false;
      } else {
        flag_teacherMode = true;
      }
    } else {
      if(currentConfig.teacherPort_ch1 != DIGITAL_PORT_NOT_USED &&
          ioStates.digital[currentConfig.teacherPort_ch1] == GPIO_PIN_RESET) {
        flag_teacherMode = true;
      } else {
        flag_teacherMode = false;
      }
    }

    //Get data if teacher mode is not enabled
    if(!flag_teacherMode) {
      remUnit_getAxis(&currentConfig, &ioStates);
      remUnit_getBuddyButtons(&currentConfig, &ioStates, buddyStates);
    }

    //Communicate with remoteunit
    if(system_isRemoteConnected()) {
      HAL_GPIO_WritePin(RJ12_CS_Port, RJ12_CS_Pin, GPIO_PIN_RESET);
      remUnit_packData(&ioStates, txData);
      if (HAL_SPI_TransmitReceive(&hspi1, txData, rxData, 10, 10) == HAL_ERROR) {
        system_errorHandler();
      }
      if(remUnit_checkCRC(rxData, 9)) {
        remUnit_unpackData(&ioStates, rxData);
      }
      HAL_GPIO_WritePin(RJ12_CS_Port, RJ12_CS_Pin, GPIO_PIN_SET);
    }

    //Handle flags
    if(flag_reloadConfig) {
      flag_reloadConfig = false;
      remUnit_resetBuddyButtons(buddyStates);
      remUnit_loadConfig(&currentConfig);
    }

    if(flag_terminateTask) {
      HAL_GPIO_WritePin(RJ12_CS_Port, RJ12_CS_Pin, GPIO_PIN_RESET);
      remUnit_resetBuddyButtons(buddyStates);
      osThreadTerminate(htask_remoteunit);
      flag_teacherMode = false;
    }

    system_watchdog_remoteunitTask++;
    osDelay(4);
  }
}


/*******************************************************************************
 * Loads the current configuration in the "configHandler"-format and stores
 * necessary information in a given struct. This function is also responsible
 * for pre-calculation of the calibration data.
 *
 * @param pConfig A pointer to the local configuration struct
 * @return nothing
 *******************************************************************************/
static void remUnit_loadConfig( RemUnit_Config_t* pConfig ) {
  Configuration_t* pNewConfig = configHandler_getCurrentConfig();
  SystemConfiguration_t* pSysConfig = configHandler_getSystemConfig();

  //Get analog axis
  pConfig->axis_config[analog_out_x] = pSysConfig->axis_channels[analog_out_x];
  pConfig->axis_config[analog_out_y] = pSysConfig->axis_channels[analog_out_y];
  pConfig->axis_config[analog_out_z] = pSysConfig->axis_channels[analog_out_z];
  pConfig->axis_config[analog_out_w] = pSysConfig->axis_channels[analog_out_w];

  //Get teacher port
  if(pNewConfig->remoteunit.teacher_Port != DIGITAL_PORT_NOT_USED) {
    pConfig->teacherPort_sw3Pos = (pSysConfig->switch_types[pNewConfig->remoteunit.teacher_Port] == sysconf_switch_3pos);
    pConfig->teacherPort_ch1 = pSysConfig->switch_ch1[pNewConfig->remoteunit.teacher_Port];
    pConfig->teacherPort_ch2 = pSysConfig->switch_ch2[pNewConfig->remoteunit.teacher_Port];
  } else {
    pConfig->teacherPort_sw3Pos = false;
    pConfig->teacherPort_ch1 = DIGITAL_PORT_NOT_USED;
    pConfig->teacherPort_ch2 = DIGITAL_PORT_NOT_USED;
  }

  //Get buddybuttons
  for(Config_BuddyButton_t i = buddyButton1; i <= buddyButton4; i++) {
    if(pNewConfig->remoteunit.dOut[i] != DIGITAL_PORT_NOT_USED) {
      pConfig->bb_config[i] = pSysConfig->switch_types[pNewConfig->remoteunit.dOut[i]];
      pConfig->bb_ch1[i] = pSysConfig->switch_ch1[pNewConfig->remoteunit.dOut[i]];
      pConfig->bb_ch2[i] = pSysConfig->switch_ch2[pNewConfig->remoteunit.dOut[i]];
    } else {
      pConfig->bb_config[i] = sysconf_switch_none;
      pConfig->bb_ch1[i] = DIGITAL_PORT_NOT_USED;
      pConfig->bb_ch2[i] = DIGITAL_PORT_NOT_USED;
    }
  }

  //Analog configuration
  for(Config_Analog_Out_t out = analog_out_x; out <= analog_out_w; out++) {
    pConfig->aOut[out] = pNewConfig->remoteunit.aOut[out];

    //Calculate calibration data if required (see "/docs/calibration_formula.pdf" for more informations)
    if(pNewConfig->remoteunit.aOut[out] != analog_in_none && pNewConfig->remoteunit.aOut[out] < analog_in_rx) {
      int32_t inMid = pSysConfig->aIn_midpoint[pNewConfig->remoteunit.aOut[out]];
      int32_t inMarg = pSysConfig->aIn_margin[pNewConfig->remoteunit.aOut[out]];
      int32_t inDead = (pNewConfig->remoteunit.aIn_deadzone[pNewConfig->remoteunit.aOut[out]])*2;
      int32_t outMid = pSysConfig->aOut_midpoint[out];
      int32_t outMarg = pSysConfig->aOut_margin[out];

      pConfig->threas_low[out] = inMid - inDead;
      pConfig->threas_high[out] = inMid + inDead;
      pConfig->k_num[out] = outMarg;
      pConfig->k_den[out] = inMarg - inDead;
      pConfig->d_low[out] = ((outMarg * ( inMarg-inMid)) / (inMarg-inDead)) + outMid - outMarg;
      pConfig->d_high[out] = ((outMarg * (-inDead-inMid)) / (inMarg-inDead)) + outMid;
      pConfig->middpoint[out] = outMid;
      pConfig->middpoint_in[out] = inMid;
      pConfig->inInverted[out] = pSysConfig->aIn_inverted[pNewConfig->remoteunit.aOut[out]];
      pConfig->outInverted[out] = pSysConfig->aOut_inverted[out];
    }
  }
}


/*******************************************************************************
 * Forces the remote-unit-task to reload the configuration after the next
 * communication cycle.
 *
 * @return nothing
 *******************************************************************************/
void remoteunit_reloadConfig( void ) {
  flag_reloadConfig = true;
}


/*******************************************************************************
 * Forces the remote-unit-task to reload adc-statase. osDelay(10) is called
 * till the adc states are refreshed
 *
 * @return adc states
 *******************************************************************************/
RemoteUnit_adcStates_t remoteunit_getADC( void ) {
  flag_sendADC = true;

  while(flag_sendADC) {
    osDelay(10);
  }

  return adcStates;
}


/*******************************************************************************
 * Check if remoteunit is currently in teacher-mode
 *
 * @return true if teachermode is enabled
 *******************************************************************************/
bool remoteunit_isTeachermodeActive( void ) {
  return flag_teacherMode;
}


/*******************************************************************************
 * Reads in the ADC values, modifies the data according calibration stored in
 * the config-struct and adds the data to the states-struct
 *
 * @param pConfig A pointer to the currently loaded config.
 * @param pStates A pointer to the current states of the I/Os.
 * @return nothing
 *******************************************************************************/
static inline void remUnit_getAxis( RemUnit_Config_t* pConfig, RemUnit_IOStates_t* pStates ) {
  //Copy old analog values
  uint32_t aRemote[4] = { pStates->analog[pConfig->axis_config[analog_out_x]],
                          pStates->analog[pConfig->axis_config[analog_out_y]],
                          pStates->analog[pConfig->axis_config[analog_out_z]],
                          pStates->analog[pConfig->axis_config[analog_out_w]]};
  uint32_t aJoystick[4] = {0};
  int32_t val1, val2;

  //Get ADCs
  adc1_start();
  adc1_getADC(aJoystick);

  //Normalize pressure sensor
  val1 = system_getSupplyVoltage();
  if(val1 > 0) {
    aJoystick[3] = (aJoystick[3] * 4096) / val1;
  }

  //Check if results are needed by another task
  if(flag_sendADC) {
    adcStates.rem_x = aRemote[0];
    adcStates.rem_y = aRemote[1];
    adcStates.rem_z = aRemote[2];
    adcStates.rem_w = aRemote[3];
    adcStates.joy_x = aJoystick[0];
    adcStates.joy_y = aJoystick[1];
    adcStates.joy_z = aJoystick[2];
    adcStates.joy_w = aJoystick[3];
    flag_sendADC = false;
  }

  //Add results to sturct (incl. calc for calibration)
  for(Config_Analog_Out_t out = analog_out_x; out <= analog_out_w; out++) {
    Config_Analog_In_t in = pConfig->aOut[out];

    switch(in) {
      case analog_in_x:
      case analog_in_y:
      case analog_in_z:
      case analog_in_w:
        //calc new value according to calibration (see "/docs/calibration_formula.pdf" for more informations)
        if(pConfig->inInverted[out]) {
          val2 = (pConfig->middpoint_in[out]*2) - aJoystick[in];
        } else {
          val2 = aJoystick[in];
        }

        val1 = (pConfig->k_num[out] * val2) / pConfig->k_den[out];

        if(val2 < pConfig->threas_low[out]) {
          val1 += pConfig->d_low[out];
        } else if (val2 > pConfig->threas_high[out]) {
          val1 += pConfig->d_high[out];
        } else {
          val1 = pConfig->middpoint[out];
        }

        if(pConfig->outInverted[out]) {
          val1 = (pConfig->middpoint[out]*2) - val1;
        }

        if(val1 > 4095)  val1 = 4095;
        if(val1 < 0)     val1 = 0;

        pStates->analog[pConfig->axis_config[out]] = (uint32_t)val1;
        break;

      case analog_in_rx:
      case analog_in_ry:
      case analog_in_rz:
      case analog_in_rw:
        //copy value from remote states
        pStates->analog[pConfig->axis_config[out]] = aRemote[in-analog_in_rx];
        break;

      case analog_in_none:
      default:
        //copy original value
        pStates->analog[pConfig->axis_config[out]] = aRemote[out];
        break;
    }
  }
}


/*******************************************************************************
 * Handles the buddybutton transitions, adds the buddybutton states to the
 * I/O-states-struct and finally sets the LEDs for the buddybuttons.
 *
 * @param pConfig A pointer to the currently loaded config.
 * @param pIOStates A pointer to the current states of the I/Os.
 * @param pBuddyStates A pointer to the current states of the buddyButtons.
 * @return nothing
 *******************************************************************************/
static inline void remUnit_getBuddyButtons( RemUnit_Config_t *pConfig,
    RemUnit_IOStates_t *pIOStates, BuddyButton_State_t *pBuddyStates ) {
  BuddyButtonMessage_t msg;
  Config_BuddyButton_t buddyId;

  //Handle buddy buttons message queue
  while(osMessageWaiting(buddyButtonsMsgBox) > 0) {
    msg.bits = osMessageGet(buddyButtonsMsgBox, osWaitForever).value.v;
    buddyId = msg.fields.buddyId;

    switch(pConfig->bb_config[buddyId]) {
      case sysconf_switch_2pos:
        if(msg.fields.event == buddybutton_pressed ) {
          pBuddyStates[buddyId]++;
          if(pBuddyStates[buddyId] > bbState_on_1) {
            pBuddyStates[buddyId] = bbState_off;
          }
        }
        break;

      case sysconf_switch_3pos:
        if(msg.fields.event == buddybutton_pressed ) {
          pBuddyStates[buddyId]++;
          if(pBuddyStates[buddyId] > bbState_on_2) {
            pBuddyStates[buddyId] = bbState_off;
          }
        }
        break;

      case sysconf_momentary_2pos:
        if(msg.fields.event == buddybutton_pressed ) {
          pBuddyStates[buddyId] = bbState_on_1;
        } else if (msg.fields.event == buddybutton_released ) {
          pBuddyStates[buddyId] = bbState_off;
        }
        break;

      case sysconf_switch_none:
      default:
        break;
    }
  }

  //Add data to output and update LEDs
  for(buddyId = 0; buddyId < 4; buddyId++) {
    switch(pBuddyStates[buddyId]) {
      case bbState_on_1:
        if(pConfig->bb_config[buddyId] == sysconf_switch_3pos) {
          if(pConfig->bb_ch1[buddyId] != DIGITAL_PORT_NOT_USED) {
            pIOStates->digital[pConfig->bb_ch1[buddyId]] = GPIO_PIN_SET;
          }
          if(pConfig->bb_ch2[buddyId] != DIGITAL_PORT_NOT_USED) {
            pIOStates->digital[pConfig->bb_ch2[buddyId]] = GPIO_PIN_SET;
          }
        } else {
          if(pConfig->bb_ch1[buddyId] != DIGITAL_PORT_NOT_USED) {
            pIOStates->digital[pConfig->bb_ch1[buddyId]] = GPIO_PIN_RESET;
          }
        }
        HAL_GPIO_WritePin(buddyButtons_ledPorts[buddyId][0], buddyButtons_ledPins[buddyId][0], GPIO_PIN_SET);
        HAL_GPIO_WritePin(buddyButtons_ledPorts[buddyId][1], buddyButtons_ledPins[buddyId][1], GPIO_PIN_RESET);
        break;

      case bbState_on_2:
        if(pConfig->bb_ch1[buddyId] != DIGITAL_PORT_NOT_USED) {
          pIOStates->digital[pConfig->bb_ch1[buddyId]] = GPIO_PIN_SET;
        }
        if(pConfig->bb_ch2[buddyId] != DIGITAL_PORT_NOT_USED) {
          pIOStates->digital[pConfig->bb_ch2[buddyId]] = GPIO_PIN_RESET;
        }
        HAL_GPIO_WritePin(buddyButtons_ledPorts[buddyId][0], buddyButtons_ledPins[buddyId][0], GPIO_PIN_RESET);
        HAL_GPIO_WritePin(buddyButtons_ledPorts[buddyId][1], buddyButtons_ledPins[buddyId][1], GPIO_PIN_SET);
        break;

      case bbState_off:
      default:
        if(pConfig->bb_config[buddyId] == sysconf_switch_3pos) {
          if(pConfig->bb_ch1[buddyId] != DIGITAL_PORT_NOT_USED) {
            pIOStates->digital[pConfig->bb_ch1[buddyId]] = GPIO_PIN_RESET;
          }
          if(pConfig->bb_ch2[buddyId] != DIGITAL_PORT_NOT_USED) {
            pIOStates->digital[pConfig->bb_ch2[buddyId]] = GPIO_PIN_SET;
          }
        } else {
          if(pConfig->bb_ch1[buddyId] != DIGITAL_PORT_NOT_USED) {
            pIOStates->digital[pConfig->bb_ch1[buddyId]] = GPIO_PIN_SET;
          }
        }

        HAL_GPIO_WritePin(buddyButtons_ledPorts[buddyId][0], buddyButtons_ledPins[buddyId][0], GPIO_PIN_RESET);
        HAL_GPIO_WritePin(buddyButtons_ledPorts[buddyId][1], buddyButtons_ledPins[buddyId][1], GPIO_PIN_RESET);
        break;
    }
  }
}


/*******************************************************************************
 * Resets the buddybutton-states to their default value and disables all LEDs.
 * Emptys the message queue.
 *
 * @param pStates A pointer to the current states of the buddyButtons.
 * @return nothing
 *******************************************************************************/
static void remUnit_resetBuddyButtons( BuddyButton_State_t* pStates ) {
  BuddyButtonMessage_t msg;

  //Reset stats
  pStates[buddyButton1] = bbState_off;
  pStates[buddyButton2] = bbState_off;
  pStates[buddyButton3] = bbState_off;
  pStates[buddyButton4] = bbState_off;

  //Reset LEDs
  HAL_GPIO_WritePin(GPIOB, LED2_G_Pin | LED2_R_Pin | LED3_G_Pin | LED3_R_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOC, LED4_G_Pin | LED1_G_Pin | LED1_R_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOA, LED4_R_Pin, GPIO_PIN_RESET);

  //Empty message queue
  while(osMessageWaiting(buddyButtonsMsgBox) > 0) {
    msg.bits = osMessageGet(buddyButtonsMsgBox, osWaitForever).value.v;
    (void)msg;
  }
}


/*******************************************************************************
 * Packs an IO-struct into a format, which can be sent to the remote-unit.
 *
 * @param pData The data (IO-struct) which will be used to create the package.
 * @param pPackage The package which will be filled with data (uint8_t x[10])
 * @return nothing
 *******************************************************************************/
static inline void remUnit_packData( RemUnit_IOStates_t* data, uint8_t* package ) {
  package[0]  = (data->analog[0] >> 0) & 0xFF;
  package[1]  = (data->analog[0] >> 8) & 0x0F;
  package[1] |= ADD_GPIO_BIT(data->digital[ 0], 4);
  package[1] |= ADD_GPIO_BIT(data->digital[ 1], 5);
  package[1] |= ADD_GPIO_BIT(data->digital[ 2], 6);
  package[1] |= ADD_GPIO_BIT(data->digital[ 3], 7);

  package[2]  = (data->analog[1] >> 0) & 0xFF;
  package[3]  = (data->analog[1] >> 8) & 0x0F;
  package[3] |= ADD_GPIO_BIT(data->digital[ 4], 4);
  package[3] |= ADD_GPIO_BIT(data->digital[ 5], 5);
  package[3] |= ADD_GPIO_BIT(data->digital[ 6], 6);
  package[3] |= ADD_GPIO_BIT(data->digital[ 7], 7);

  package[4]  = (data->analog[2] >> 0) & 0xFF;
  package[5]  = (data->analog[2] >> 8) & 0x0F;
  package[5] |= ADD_GPIO_BIT(data->digital[ 8], 4);
  package[5] |= ADD_GPIO_BIT(data->digital[ 9], 5);
  package[5] |= ADD_GPIO_BIT(data->digital[10], 6);
  package[5] |= ADD_GPIO_BIT(data->digital[11], 7);

  package[6]  = (data->analog[3] >> 0) & 0xFF;
  package[7]  = (data->analog[3] >> 8) & 0x0F;
  package[7] |= ADD_GPIO_BIT(data->digital[12], 4);
  package[7] |= ADD_GPIO_BIT(data->digital[13], 5);
  package[7] |= ADD_GPIO_BIT(data->digital[14], 6);
  package[7] |= ADD_GPIO_BIT(data->digital[15], 7);

  package[8]  = ADD_GPIO_BIT(data->digital[16], 0);
  package[8] |= ADD_GPIO_BIT(data->digital[17], 1);
  package[8] |= ADD_GPIO_BIT(data->digital[18], 2);
  package[8] |= ADD_GPIO_BIT(data->digital[19], 3);
  package[8] |= ADD_GPIO_BIT(data->digital[20], 4);
  package[8] |= ADD_GPIO_BIT(data->digital[21], 5);
  package[8] |= ADD_GPIO_BIT(data->digital[22], 6);
  package[8] |= ADD_GPIO_BIT(data->digital[23], 7);

  package[9] = remUnit_calcCRC(package, 9);
}


/*******************************************************************************
 * Unpacks a package received from the remote-unit and stores its contents to
 * the IO-struct.
 *
 * @param pData A pointer to the IO-struct which will be filled.
 * @param pPackage The package received from the remote-unit (uint8_t x[10])
 * @return nothing
 *******************************************************************************/
static inline void remUnit_unpackData( RemUnit_IOStates_t* pData, uint8_t* pPackage ) {
  //Convert analog
  pData->analog[0] = pPackage[0] | ((pPackage[1] & 0x0F) << 8);
  pData->analog[1] = pPackage[2] | ((pPackage[3] & 0x0F) << 8);
  pData->analog[2] = pPackage[4] | ((pPackage[5] & 0x0F) << 8);
  pData->analog[3] = pPackage[6] | ((pPackage[7] & 0x0F) << 8);

  //Convert digital
  pData->digital[ 0] = EXTRACT_GPIO_VAL(pPackage[1], 4);
  pData->digital[ 1] = EXTRACT_GPIO_VAL(pPackage[1], 5);
  pData->digital[ 2] = EXTRACT_GPIO_VAL(pPackage[1], 6);
  pData->digital[ 3] = EXTRACT_GPIO_VAL(pPackage[1], 7);

  pData->digital[ 4] = EXTRACT_GPIO_VAL(pPackage[3], 4);
  pData->digital[ 5] = EXTRACT_GPIO_VAL(pPackage[3], 5);
  pData->digital[ 6] = EXTRACT_GPIO_VAL(pPackage[3], 6);
  pData->digital[ 7] = EXTRACT_GPIO_VAL(pPackage[3], 7);

  pData->digital[ 8] = EXTRACT_GPIO_VAL(pPackage[5], 4);
  pData->digital[ 9] = EXTRACT_GPIO_VAL(pPackage[5], 5);
  pData->digital[10] = EXTRACT_GPIO_VAL(pPackage[5], 6);
  pData->digital[11] = EXTRACT_GPIO_VAL(pPackage[5], 7);

  pData->digital[12] = EXTRACT_GPIO_VAL(pPackage[7], 4);
  pData->digital[13] = EXTRACT_GPIO_VAL(pPackage[7], 5);
  pData->digital[14] = EXTRACT_GPIO_VAL(pPackage[7], 6);
  pData->digital[15] = EXTRACT_GPIO_VAL(pPackage[7], 7);

  pData->digital[16] = EXTRACT_GPIO_VAL(pPackage[8], 0);
  pData->digital[17] = EXTRACT_GPIO_VAL(pPackage[8], 1);
  pData->digital[18] = EXTRACT_GPIO_VAL(pPackage[8], 2);
  pData->digital[19] = EXTRACT_GPIO_VAL(pPackage[8], 3);
  pData->digital[20] = EXTRACT_GPIO_VAL(pPackage[8], 4);
  pData->digital[21] = EXTRACT_GPIO_VAL(pPackage[8], 5);
  pData->digital[22] = EXTRACT_GPIO_VAL(pPackage[8], 6);
  pData->digital[23] = EXTRACT_GPIO_VAL(pPackage[8], 7);
}


/*******************************************************************************
 * Calculates the checksum from a given uint8_t-array 'pData' with length 'len'.
 *
 * @param pData A pointer to the data.
 * @param pPackage The size of the data-array
 * @return The CRC-Checksum
 *******************************************************************************/
static uint8_t remUnit_calcCRC( uint8_t* pData, uint32_t len ) {
  uint8_t crc = CRC_START_VALUE;

  while (len--) {
    crc = crc8_table[crc ^ *pData++];
  }

  return crc;
}


/*******************************************************************************
 * Checks if the checksum of a package is correct. The checksum must be the
 * last byte of the package.
 *
 * @param pData A pointer to the data.
 * @param pPackage The size of the data-array.
 * @return true if checksum was correct.
 *******************************************************************************/
static inline bool remUnit_checkCRC( uint8_t* data, uint32_t len ) {
  uint8_t calculatedCRC = remUnit_calcCRC(data, len);
  return (calculatedCRC == data[len]);
}
