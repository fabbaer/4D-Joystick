/*******************************************************************************
* @file         : board.h
* @project      : 4D-Joystick, Joystick-Unit
* @author       : Fabian Baer
* @brief        : Pin definitions
*******************************************************************************/

#ifndef __DRIVERS_INC_BOARD_H_
#define __DRIVERS_INC_BOARD_H_

#include "stm32l4xx_hal.h"

/* ########################### Configuration ############################### */
#define USE_DEBUG_UART
#define ENABLE_WATCHDOG


/* ########################### Joystick ##################################### */
#define JOYSTICK_X_Port     GPIOA
#define JOYSTICK_X_Pin      GPIO_PIN_1
#define JOYSTICK_X_Channel  ADC_CHANNEL_6
#define JOYSTICK_Y_Port     GPIOA
#define JOYSTICK_Y_Pin      GPIO_PIN_2
#define JOYSTICK_Y_Channel  ADC_CHANNEL_7
#define JOYSTICK_Z_Port     GPIOA
#define JOYSTICK_Z_Pin      GPIO_PIN_0
#define JOYSTICK_Z_Channel  ADC_CHANNEL_5
#define JOYSTICK_W_Port     GPIOC
#define JOYSTICK_W_Pin      GPIO_PIN_4
#define JOYSTICK_W_Channel  ADC_CHANNEL_13


/* ########################### Buddy buttons ################################ */
#define BB_1_Port         GPIOC
#define BB_1_Pin          GPIO_PIN_3
#define BB_2_Port         GPIOC
#define BB_2_Pin          GPIO_PIN_2
#define BB_3_Port         GPIOC
#define BB_3_Pin          GPIO_PIN_1
#define BB_4_Port         GPIOC
#define BB_4_Pin          GPIO_PIN_0


/* ########################### Display ###################################### */
#define DISP_CLK_Port     GPIOB
#define DISP_CLK_Pin      GPIO_PIN_12
#define DISP_MOSI_Port    GPIOB
#define DISP_MOSI_Pin     GPIO_PIN_14
#define DISP_DC_Port      GPIOC
#define DISP_DC_Pin       GPIO_PIN_12
#define DISP_CS_Port      GPIOB
#define DISP_CS_Pin       GPIO_PIN_13


/* ########################### Menu buttons ################################# */
#define BTN1_Port         GPIOC
#define BTN1_Pin          GPIO_PIN_9
#define BTN2_Port         GPIOC
#define BTN2_Pin          GPIO_PIN_8
#define BTN3_Port         GPIOC
#define BTN3_Pin          GPIO_PIN_7


/* ########################### Frontpanel-LEDs ############################## */
#define LED1_G_Port       GPIOC
#define LED1_G_Pin        GPIO_PIN_10
#define LED1_R_Port       GPIOC
#define LED1_R_Pin        GPIO_PIN_11
#define LED2_G_Port       GPIOB
#define LED2_G_Pin        GPIO_PIN_4
#define LED2_R_Port       GPIOB
#define LED2_R_Pin        GPIO_PIN_5
#define LED3_G_Port       GPIOB
#define LED3_G_Pin        GPIO_PIN_6
#define LED3_R_Port       GPIOB
#define LED3_R_Pin        GPIO_PIN_7
#define LED4_G_Port       GPIOC
#define LED4_G_Pin        GPIO_PIN_6
#define LED4_R_Port       GPIOA
#define LED4_R_Pin        GPIO_PIN_8


/* ########################### RJ12 ######################################### */
#define RJ12_CS_Port      GPIOA
#define RJ12_CS_Pin       GPIO_PIN_4
#define RJ12_SCK_Port     GPIOA
#define RJ12_SCK_Pin      GPIO_PIN_5
#define RJ12_MISO_Port    GPIOA
#define RJ12_MISO_Pin     GPIO_PIN_6
#define RJ12_MOSI_Port    GPIOA
#define RJ12_MOSI_Pin     GPIO_PIN_7


/* ########################### USB ########################################## */
#define USB_DM_Port       GPIOA
#define USB_DM_Pin        GPIO_PIN_11
#define USB_DP_Port       GPIOA
#define USB_DP_Pin        GPIO_PIN_12


/* ########################### I2C ########################################## */
#define I2C_SCL_Port      GPIOB
#define I2C_SCL_Pin       GPIO_PIN_8
#define I2C_SDA_Port      GPIOB
#define I2C_SDA_Pin       GPIO_PIN_9


/* ########################### System ####################################### */
#define DEBUG_LED_Port    GPIOB
#define DEBUG_LED_Pin     GPIO_PIN_15
#define DEBUG_TX_Port     GPIOA
#define DEBUG_TX_Pin      GPIO_PIN_9
#define DEBUG_RX_Port     GPIOA
#define DEBUG_RX_Pin      GPIO_PIN_10


/* ########################### Supply ####################################### */
#define SUPPLY_P5V_Port     GPIOC
#define SUPPLY_P5V_Pin      GPIO_PIN_5
#define SUPPLY_P5V_Channel  ADC_CHANNEL_14
#define USB_P5V_Port        GPIOB
#define USB_P5V_Pin         GPIO_PIN_0
#define USB_P5V_Channel     ADC_CHANNEL_15
#define RJ12_P5V_Port       GPIOB
#define RJ12_P5V_Pin        GPIO_PIN_1
#define RJ12_P5V_Channel    ADC_CHANNEL_16


/* ########################### Spare pins ################################### */
#define SPARE1_Port       GPIOC
#define SPARE1_Pin        GPIO_PIN_14
#define SPARE2_Port       GPIOH
#define SPARE2_Pin        GPIO_PIN_0
#define SPARE3_Port       GPIOA
#define SPARE3_Pin        GPIO_PIN_3
#define SPARE4_Port       GPIOH
#define SPARE4_Pin        GPIO_PIN_1
#define SPARE5_Port       GPIOC
#define SPARE5_Pin        GPIO_PIN_15
#define SPARE6_Port       GPIOC
#define SPARE6_Pin        GPIO_PIN_13


/* ########################### Not connected ################################ */
#define NC1_Port          GPIOB
#define NC1_Pin           GPIO_PIN_2
#define NC2_Port          GPIOB
#define NC2_Pin           GPIO_PIN_10
#define NC3_Port          GPIOB
#define NC3_Pin           GPIO_PIN_11
#define NC4_Port          GPIOD
#define NC4_Pin           GPIO_PIN_2

#endif /* __DRIVERS_INC_BOARD_H_ */
