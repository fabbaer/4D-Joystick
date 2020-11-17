/*******************************************************************************
* @file         : main.c
* @project      : 4D-Joystick, Joystick-Unit
* @author       : Fabian Baer
* @brief        : Main program body
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include <cmsis_os.h>
#include <stm32l4xx_hal.h>
#include <board.h>
#include <system.h>
#include <remoteunit.h>
#include <configHandler.h>
#include <buttons.h>
#include <userinterface.h>
#include <cli.h>


/* Code ----------------------------------------------------------------------*/
int main(void) {
  //Init HAL
  HAL_Init();

  //Init peripherals
  system_init();
  configHandler_init();
  remoteunit_init();
  buttons_init();
  ui_init();
  cli_init();

  //Init message queues
  remoteunit_setBuddyButtonsMQ(buttons_getBuddyButtonsMQ());
  ui_setRotaryEncoderMQ(buttons_getRotaryEncoderMQ());

  //Init tasks
  system_setupTask( osPriorityAboveNormal );
  buttons_setupTask( osPriorityHigh );
  ui_setupTask( osPriorityNormal );
  configHandler_setupTaskOfCurrentConfigType();
  cli_setupTask( osPriorityNormal );

  //Start scheduler
  osKernelStart();

  //Reset if system gets to this point
  NVIC_SystemReset();
}

