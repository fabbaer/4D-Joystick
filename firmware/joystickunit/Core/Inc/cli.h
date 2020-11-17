/*******************************************************************************
 * @file         : cli.c
 * @project      : 4D-Joystick, Joystick-Unit
 * @author       : Fabian Baer
 * @brief        : Command-Line-Interface
 ******************************************************************************/

#ifndef INC_CLI_H_
#define INC_CLI_H_

#include <cmsis_os.h>
#include <stm32l4xx_hal.h>

//#define USE_UART1_CLI

#define CLI_BUFFERSIZE  256

#define CLI_FLAG_RX_ABORT      0x02
#define CLI_FLAG_RX_RETURN     0x04
#define CLI_FLAG_RX_ESCAPE     0x08
#define CLI_FLAG_REPRINT       0x10
#define CLI_FLAG_EXECUTING     0x20
#define CLI_FLAG_RX_RAW        0x40


typedef enum {
    cli_connection_usb,
    cli_connection_uart1
} CLI_Connection_t;

typedef enum {
    cli_type_user,
    cli_type_software
} CLI_Type_t;

typedef struct {
    CLI_Connection_t connection;
    CLI_Type_t type;
    uint8_t flags;
    uint8_t rxBuffer[CLI_BUFFERSIZE+1];
    uint32_t rxLength;
    uint32_t rxCursor;
} CLI_Handle_t;

typedef enum {
    cli_input_OK,
    cli_input_empty,
    cli_input_overflow,
    cli_input_cancle,
    cli_input_noNum
} CLI_InputState_t;

void cli_init( void );
void cli_setupTask( osPriority priority );
void cli_putChar(CLI_Handle_t* hcli, uint8_t ch);
void cli_print(CLI_Handle_t* hcli, uint8_t* str, uint32_t length);
void cli_newLine(CLI_Handle_t* hcli);
void cli_putNum(CLI_Handle_t* hcli, uint32_t num);
CLI_InputState_t cli_getInput(CLI_Handle_t* hcli, uint8_t* buf, uint32_t* len);
CLI_InputState_t cli_getNum(CLI_Handle_t* hcli, uint32_t* num);
CLI_InputState_t cli_waitForEnter( CLI_Handle_t* hcli );
void cli_printAbort(CLI_Handle_t* hcli);
void cli_printSucess(CLI_Handle_t* hcli);

#define cli_putStr(hcli, str) cli_print(hcli, (uint8_t*)str, sizeof(str)-1)
#define cli_putStrLn(hcli, str) {cli_print(hcli, (uint8_t*)str, sizeof(str)-1); cli_newLine(hcli);}


#endif /* INC_CLI_H_ */
