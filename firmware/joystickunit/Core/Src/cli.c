/*******************************************************************************
 * @file         : cli.c
 * @project      : 4D-Joystick, Joystick-Unit
 * @author       : Fabian Baer
 * @brief        : Command-Line-Interface
 ******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <cmsis_os.h>
#include <stm32l4xx_hal.h>
#include <board.h>
#include <system.h>
#include "usb_device.h"
#include "cli.h"
#include "cli_commands.h"


/* Defines -------------------------------------------------------------------*/
#define CHAR_CMD            '>'
#define STR_CLEAR_LINE      "\33[2K"


/* Prototypes ----------------------------------------------------------------*/
void cli_task(void const *argument);
static inline void cli_newCmd(CLI_Handle_t* hcli);
static inline void cli_trimm(CLI_Handle_t* hcli);


/* Variables -----------------------------------------------------------------*/
static osThreadId htask_cliUSB;
CLI_Handle_t hcli_usb = {0};

#ifdef USE_UART1_CLI
static UART_HandleTypeDef huart1;
static osThreadId htask_cliUART1;
static CLI_Handle_t hcli_uart1 = {0};
#endif


/* Code ----------------------------------------------------------------------*/
void cli_init( void ) {
  //Init USB handle
  hcli_usb.connection = cli_connection_usb;
  hcli_usb.type = cli_type_user;
  hcli_usb.rxLength = 0;
  hcli_usb.rxCursor = 0;
  hcli_usb.flags = 0;

  //Init peripherals
  usbDevice_init();

#ifdef USE_UART1_CLI
  GPIO_InitTypeDef GPIO_InitStruct = { 0 };

  //Init UART handle
  hcli_uart1.connection = cli_connection_uart1;
  hcli_uart1.type = cli_type_user;
  hcli_uart1.rxLength = 0;
  hcli_uart1.rxCursor = 0;
  hcli_uart1.flags = 0;

  //Init UART1
  __HAL_RCC_USART1_CLK_ENABLE();

  GPIO_InitStruct.Pin = DEBUG_TX_Pin | DEBUG_RX_Pin;
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
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK) {
    system_errorHandler();
  }
#endif

}

void cli_setupTask( osPriority priority ) {
  osThreadDef(cliTaskUSB, cli_task, priority, 0, 512);
  htask_cliUSB = osThreadCreate(osThread(cliTaskUSB), &hcli_usb);

#ifdef USE_UART1_CLI
  osThreadDef(cliTaskUART1, cli_task, priority, 0, 512);
  htask_cliUART1 = osThreadCreate(osThread(cliTaskUART1), &hcli_uart1);
#endif
}

void cli_task(void const *argument) {
  CLI_Handle_t *hcli = (CLI_Handle_t*)argument;

  while (1) {
    if (hcli->flags & CLI_FLAG_RX_ABORT) {
      cli_newLine(hcli);
      cli_newCmd(hcli);
    }

    if (hcli->flags & CLI_FLAG_RX_RETURN) {
      cli_newLine(hcli);
      cli_trimm(hcli);
      cli_commands_execute(hcli);
      cli_newCmd(hcli);
    }

    if (hcli->flags & CLI_FLAG_REPRINT) {
      cli_putStr(hcli, STR_CLEAR_LINE);
      cli_putChar(hcli, '\r');
      cli_putChar(hcli, CHAR_CMD);
      cli_putChar(hcli, ' ');
      cli_print(hcli, hcli->rxBuffer, hcli->rxLength);
      for(uint32_t i = hcli->rxLength; i > hcli->rxCursor; i--) {
        cli_putStr(hcli, "\e[D");
      }
      hcli->flags &= ~CLI_FLAG_REPRINT;
    }

    osDelay(10);
  }
}

static inline void cli_newCmd(CLI_Handle_t *hcli) {
  hcli->flags = 0;
  hcli->rxLength = 0;
  hcli->rxCursor = 0;
  cli_putChar(hcli, '\r');
  cli_putChar(hcli, CHAR_CMD);
  cli_putChar(hcli, ' ');
}

void cli_putChar(CLI_Handle_t *hcli, uint8_t ch) {
#ifdef USE_UART1_CLI
  if(hcli->connection == cli_connection_uart1) {
    HAL_UART_Transmit(&huart1, &ch, 1, 10);
  } else
#else
    (void)hcli;
#endif
  {
    CDC_Transmit_FS(&ch, 1);
  }
}

void cli_print(CLI_Handle_t *hcli, uint8_t *str, uint32_t length) {
#ifdef USE_UART1_CLI
  if(hcli->connection == cli_connection_uart1) {
    HAL_UART_Transmit(&huart1, str, length, 10);
  } else
#else
    (void)hcli;
#endif
  {
    CDC_Transmit_FS(str, length);
  }
}

void cli_newLine(CLI_Handle_t *hcli) {
  cli_putStr(hcli, "\r\n  ");
}

void cli_putNum(CLI_Handle_t* hcli, uint32_t num) {
  uint8_t buf[10];

  buf[9] = ((num % 10) / 1) + '0';
  buf[8] = ((num % 100) / 10) + '0';
  buf[7] = ((num % 1000) / 100) + '0';
  buf[6] = ((num % 10000) / 1000) + '0';
  buf[5] = ((num % 100000) / 10000) + '0';
  buf[4] = ((num % 1000000) / 100000) + '0';
  buf[3] = ((num % 10000000) / 1000000) + '0';
  buf[2] = ((num % 100000000) / 10000000) + '0';
  buf[1] = ((num % 1000000000) / 100000000) + '0';
  buf[0] = ((num % 10000000000) / 1000000000) + '0';

  for(uint32_t i = 0; i<9; i++) {
    if(buf[i] != '0') {
      cli_putChar(hcli, buf[i]);
    }
  }

  cli_putChar(hcli, buf[9]);
}

CLI_InputState_t cli_waitForEnter( CLI_Handle_t* hcli ) {
  CLI_InputState_t state = cli_input_empty;

  //Clear buffer
  hcli->flags = 0;
  hcli->rxLength = 0;
  hcli->rxCursor = 0;

  while (1) {
    if (hcli->flags & CLI_FLAG_RX_ABORT) {
      state = cli_input_cancle;
      break;
    }

    if (hcli->flags & CLI_FLAG_RX_RETURN) {
      state = cli_input_OK;
      break;
    }

    osDelay(10);
  }

  return state;
}

CLI_InputState_t cli_getInput(CLI_Handle_t* hcli, uint8_t* buf, uint32_t* len) {
  CLI_InputState_t state = cli_input_empty;

  //Clear buffer
  hcli->flags = 0;
  hcli->rxLength = 0;
  hcli->rxCursor = 0;

  while (1) {
    if (hcli->flags & CLI_FLAG_RX_ABORT) {
      state = cli_input_cancle;
      break;
    }

    if (hcli->flags & CLI_FLAG_RX_RETURN) {
      cli_newLine(hcli);
      cli_trimm(hcli);
      if(hcli->rxLength == 0) {
        state = cli_input_empty;
        break;
      } else if(hcli->rxLength > *len) {
        state = cli_input_overflow;
        break;
      } else {
        for(uint32_t i = 0; i<hcli->rxLength; i++) {
          buf[i] = hcli->rxBuffer[i];
        }
        *len = hcli->rxLength;
        state = cli_input_OK;
        break;
      }
    }

    if (hcli->flags & CLI_FLAG_REPRINT) {
      hcli->flags &= ~CLI_FLAG_REPRINT;
      cli_putStr(hcli, STR_CLEAR_LINE"\r  ");
      cli_print(hcli, hcli->rxBuffer, hcli->rxLength);
      for(uint32_t i = hcli->rxLength; i > hcli->rxCursor; i--) {
        cli_putStr(hcli, "\e[D");
      }
    }

    osDelay(10);
  }

  switch(state) {
    case cli_input_OK:
    case cli_input_empty:
      break;
    case cli_input_overflow:
      cli_putStrLn(hcli, "Error: To many characters!");
    default:
    case cli_input_cancle:
      cli_printAbort(hcli);
  }

  return state;
}

CLI_InputState_t cli_getNum(CLI_Handle_t* hcli, uint32_t* num) {
  CLI_InputState_t state = cli_input_empty;
  uint8_t buf[10];
  uint32_t len = 10;
  uint32_t dec = 1000000000;
  uint32_t i;

  state = cli_getInput(hcli, buf, &len);

  if(state == cli_input_OK) {
    //Check if it is a numeric value
    for(i=0; i<len; i++) {
      if(buf[i] < '0' || buf[i] > '9') {
        cli_putStrLn(hcli, "Error: Input is not numeric!");
        cli_printAbort(hcli);
        return cli_input_noNum;
      }
    }

    //Get start-decade
    for(i=10; i>len; i--) {
      dec /= 10;
    }

    //Calculate value
    *num = 0;
    for(i=0; i<len; i++) {
      *num += (buf[i] - '0') * dec;
      dec /= 10;
    }
  }
  return state;
}

void cli_printAbort(CLI_Handle_t* hcli) {
  cli_putStrLn(hcli, "Abort!");
}

void cli_printSucess(CLI_Handle_t* hcli) {
  cli_putStrLn(hcli, "Success!");
}

static inline void cli_trimm(CLI_Handle_t* hcli) {
  while(hcli->rxBuffer[hcli->rxLength-1] == ' ' && hcli->rxLength > 0) {
    hcli->rxLength--;
  }
}
