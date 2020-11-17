/*******************************************************************************
* @file         : board.h
* @project      : 4D-Joystick, Remote-Unit
* @author       : Fabian Baer
* @brief        : Pin definitions
*******************************************************************************/

#ifndef _DRIVER_INC_BOARD_H
#define _DRIVER_INC_BOARD_H

/* ########################### Configuration ############################### */
//#define USE_DEBUG_UART
#define ENABLE_WATCHDOG

/* ########################### Output GPIO ################################# */
#define DO1_Port         GPIOA
#define DO1_Pin          GPIO_PIN_1
#define DO2_Port         GPIOA
#define DO2_Pin          GPIO_PIN_0
#define DO3_Port         GPIOC
#define DO3_Pin          GPIO_PIN_2
#define DO4_Port         GPIOC
#define DO4_Pin          GPIO_PIN_3
#define DO5_Port         GPIOC
#define DO5_Pin          GPIO_PIN_1
#define DO6_Port         GPIOC
#define DO6_Pin          GPIO_PIN_0
#define DO7_Port         GPIOH
#define DO7_Pin          GPIO_PIN_1
#define DO8_Port         GPIOH
#define DO8_Pin          GPIO_PIN_0
#define DO9_Port         GPIOC
#define DO9_Pin          GPIO_PIN_15
#define DO10_Port        GPIOC
#define DO10_Pin         GPIO_PIN_14
#define DO11_Port        GPIOC
#define DO11_Pin         GPIO_PIN_13
#define DO12_Pin         GPIO_PIN_6
#define DO13_Pin         GPIO_PIN_5
#define DO14_Pin         GPIO_PIN_4
#define DO15_Pin         GPIO_PIN_3
#define DO16_Pin         GPIO_PIN_2
#define DO17_Pin         GPIO_PIN_1
#define DO18_Pin         GPIO_PIN_0
#define DO19_Port        GPIOB
#define DO19_Pin         GPIO_PIN_5
#define DO20_Port        GPIOB
#define DO20_Pin         GPIO_PIN_4
#define DO21_Port        GPIOB
#define DO21_Pin         GPIO_PIN_11
#define DO22_Port        GPIOC
#define DO22_Pin         GPIO_PIN_12
#define DO23_Port        GPIOA
#define DO23_Pin         GPIO_PIN_10
#define DO24_Pin         GPIO_PIN_7


/* ########################### Input GPIO ################################## */
#define DI1_Port         GPIOB
#define DI1_Pin          GPIO_PIN_9
#define DI2_Port         GPIOB
#define DI2_Pin          GPIO_PIN_10
#define DI3_Port         GPIOB
#define DI3_Pin          GPIO_PIN_13
#define DI4_Port         GPIOB
#define DI4_Pin          GPIO_PIN_12
#define DI5_Port         GPIOA
#define DI5_Pin          GPIO_PIN_12
#define DI6_Port         GPIOA
#define DI6_Pin          GPIO_PIN_11
#define DI7_Port         GPIOA
#define DI7_Pin          GPIO_PIN_8
#define DI8_Port         GPIOC
#define DI8_Pin          GPIO_PIN_9
#define DI9_Port         GPIOC
#define DI9_Pin          GPIO_PIN_8
#define DI10_Port        GPIOC
#define DI10_Pin         GPIO_PIN_7
#define DI11_Port        GPIOC
#define DI11_Pin         GPIO_PIN_6
#define DI12_Pin         GPIO_PIN_6
#define DI13_Pin         GPIO_PIN_5
#define DI14_Pin         GPIO_PIN_4
#define DI15_Pin         GPIO_PIN_3
#define DI16_Pin         GPIO_PIN_2
#define DI17_Pin         GPIO_PIN_1
#define DI18_Pin         GPIO_PIN_0
#define DI19_Port        GPIOC
#define DI19_Pin         GPIO_PIN_10
#define DI20_Port        GPIOC
#define DI20_Pin         GPIO_PIN_11
#define DI21_Port        GPIOB
#define DI21_Pin         GPIO_PIN_15
#define DI22_Port        GPIOB
#define DI22_Pin         GPIO_PIN_14
#define DI23_Port        GPIOA
#define DI23_Pin         GPIO_PIN_9
#define DI24_Pin         GPIO_PIN_7


/* ########################### Analog ###################################### */
#define AI1_Port         GPIOC
#define AI1_Pin          GPIO_PIN_4
#define AI1_Channel      ADC_CHANNEL_14
#define AI2_Port         GPIOC
#define AI2_Pin          GPIO_PIN_5
#define AI2_Channel      ADC_CHANNEL_15
#define AI3_Port         GPIOA
#define AI3_Pin          GPIO_PIN_2
#define AI3_Channel      ADC_CHANNEL_2
#define AI4_Port         GPIOA
#define AI4_Pin          GPIO_PIN_3
#define AI4_Channel      ADC_CHANNEL_3


/* ########################### SPI ######################################### */
#define RJ12_CS_Port     GPIOA
#define RJ12_CS_Pin      GPIO_PIN_4
#define RJ12_SCK_Port    GPIOA
#define RJ12_SCK_Pin     GPIO_PIN_5
#define RJ12_MISO_Port   GPIOA
#define RJ12_MISO_Pin    GPIO_PIN_6
#define RJ12_MOSI_Port   GPIOA
#define RJ12_MOSI_Pin    GPIO_PIN_7
#define DAC_SCLK_Port    GPIOB
#define DAC_SCLK_Pin     GPIO_PIN_0
#define DAC_SYNC_Port    GPIOB
#define DAC_SYNC_Pin     GPIO_PIN_1
#define DAC_DIN_Port     GPIOB
#define DAC_DIN_Pin      GPIO_PIN_8


/* ########################### Debug ####################################### */
#define DEBUG_LED_Port   GPIOB
#define DEBUG_LED_Pin    GPIO_PIN_2


#endif /* _DRIVER_INC_BOARD_H */
