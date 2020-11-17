/*******************************************************************************
* @file         : freertos.c
* @project      : 4D-Joystick, Joystick-Unit
* @author       : Fabian Baer
* @brief        : Functions needed by RTOS
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include <FreeRTOS.h>
#include <task.h>


/* Variables -----------------------------------------------------------------*/
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];


/* Code ----------------------------------------------------------------------*/
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
    StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize) {
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

void vApplicationStackOverflowHook( TaskHandle_t xTask, signed char *pcTaskName ) {
  volatile uint32_t x = 0;
  x++;
}
