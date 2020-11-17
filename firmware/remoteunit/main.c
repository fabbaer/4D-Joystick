/*******************************************************************************
* @file         : main.c
* @project      : 4D-Joystick, Remote-Unit
* @author       : Fabian Baer
* @brief        : Main program body.
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include <stm32f4xx_hal.h>
#include <board.h>
#include <system.h>
#include <remoteIO.h>
#include <joystickunit.h>


/* Defines -------------------------------------------------------------------*/
#define DEBUG_PREFIX        "Main - "
#define MAX_CRITICAL_LOOPS  25


/* Code ----------------------------------------------------------------------*/
int main(void) {
  RemoteIO_States_t inputs = {0};
  RemoteIO_States_t outputs = {0};
  Joystickunit_State_t joyState = JOY_STATE_NOT_AVAILABLE;
  uint32_t errorCnt = 0;

  //Init system
  HAL_Init();
  system_setupClock();

  //Init peripherals
  system_init();
  remoteIO_init();
  joystickunit_init();
  system_watchdogHandler();

  system_debugMessage(DEBUG_PREFIX "Init finished");

  while (1) {
    //Standalone-Mode (without joystick-unit)
    do {
      remoteIO_getStates(&inputs);
      remoteIO_setStates(&inputs);
      system_handler(JOY_STATE_NOT_AVAILABLE);
    } while(HAL_GPIO_ReadPin(RJ12_CS_Port, RJ12_CS_Pin) == GPIO_PIN_RESET);

    //Switch to Joystickunit-Mode

    //Wait for the first communication to finish
    joyState = joystickunit_communicate(&inputs, &outputs);

    while(1) {
      remoteIO_getStates(&inputs);

      joyState = joystickunit_communicate(&inputs, &outputs);

      if(errorCnt > MAX_CRITICAL_LOOPS) {
        joyState = JOY_STATE_NOT_AVAILABLE;
      }

      switch(joyState) {
        case JOY_STATE_OK:
          remoteIO_setStates(&outputs);
          errorCnt=0;
          break;
        case JOY_STATE_ERROR:
          //Keep old states in case of error
          errorCnt++;
          system_debugMessage(DEBUG_PREFIX "Communication error");
          break;
        case JOY_STATE_NOT_AVAILABLE:
        default:
          system_debugMessage(DEBUG_PREFIX "Joystick not available");
          NVIC_SystemReset();
          break;
      }

      system_handler(joyState);
    }
  }
}
