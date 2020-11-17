/*******************************************************************************
* @file         : joystickunit.c
* @project      : 4D-Joystick, Remote-Unit
* @author       : Fabian Baer
* @brief        : Handles communication with joystick-unit.
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include <stm32f4xx_hal.h>
#include <stdbool.h>
#include <board.h>
#include <system.h>
#include <joystickunit.h>
#include <remoteIO.h>


/* Defines -------------------------------------------------------------------*/
#define DEBUG_PREFIX        "Joyunit - "
#define CRC_START_VALUE     0xA5


/* Makros --------------------------------------------------------------------*/
#define ADD_GPIO_BIT(gpioval, bitnum)   (((gpioval == GPIO_PIN_SET) ? 1 : 0) << bitnum)
#define EXTRACT_GPIO_VAL(data, bitnum)  ((data & (1<<bitnum)) ? GPIO_PIN_SET : GPIO_PIN_RESET)


/* Prototypes ----------------------------------------------------------------*/
static inline HAL_StatusTypeDef joystickunit_spiRxTx(uint8_t* rx, uint8_t* tx, uint8_t len, uint8_t timeout);
static inline void joystickunit_packData(RemoteIO_States_t* data, uint8_t* package);
static inline void joystickunit_unpackData(RemoteIO_States_t* data, uint8_t* package);
static uint8_t joystickunit_calcCRC(uint8_t* data, uint32_t len);
static inline bool joystickunit_checkCRC(uint8_t* data, uint32_t len);


/* Variables -----------------------------------------------------------------*/
SPI_HandleTypeDef hspi1;
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
 * Initializes CS-Pin of joystickunit
 *
 * @return nothing
 *******************************************************************************/
void joystickunit_init(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  //Init SPI-Pins
  GPIO_InitStruct.Pin = RJ12_SCK_Pin | RJ12_MOSI_Pin | RJ12_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = RJ12_MISO_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}


/*******************************************************************************
 * Communciates with Joystickunit.
 *
 * @return nothing
 *******************************************************************************/
Joystickunit_State_t joystickunit_communicate( RemoteIO_States_t* in, RemoteIO_States_t* out ) {
  uint8_t rxData[10], txData[10];
  HAL_StatusTypeDef comState;

  //Prepare the data to send
  joystickunit_packData(in, txData);

  //Transmit the data
  comState = joystickunit_spiRxTx(rxData, txData, 10, 10);
  if(comState == HAL_TIMEOUT) {
    return JOY_STATE_NOT_AVAILABLE;
  } else if (comState != HAL_OK) {
    return JOY_STATE_ERROR;
  }

  //Check received data
  if(joystickunit_checkCRC(rxData, 9)) {
    joystickunit_unpackData(out, rxData);
    return JOY_STATE_OK;
  } else {
    return JOY_STATE_ERROR;
  }
}



/*******************************************************************************
 * Handles communication with joystickunit via bitbanging-SPI.
 *
 * @param rx A pointer to the receiving data array
 * @param tx A pointer to the transmitting data array
 * @param len The amount of data
 * @param timeout The timeout in Millisecons
 * @return The status of the communcation
 *******************************************************************************/
static inline HAL_StatusTypeDef joystickunit_spiRxTx(uint8_t* rx, uint8_t* tx, uint8_t len, uint8_t timeout) {
  uint32_t time = HAL_GetTick() + timeout;
  uint16_t curByte, curBit;

  //Init RX-data
  for(curByte = 0; curByte < len; curByte++) {
    rx[curByte] = 0;
  }

  //Wait for CS pin to get low
  while(GPIOA->IDR & RJ12_CS_Pin) {
    if(HAL_GetTick() > time)  return HAL_TIMEOUT;
  }

  //Main transmission
  for(curByte = 0; curByte < len; curByte++) {
    for(curBit = 1; curBit <= 0x80; curBit = (curBit << 1)) {
      //Wait for falling edge on SCK
      while(GPIOA->IDR & RJ12_SCK_Pin) {
        if(HAL_GetTick() > time)      {
          return HAL_TIMEOUT;
        }
        if(GPIOA->IDR & RJ12_CS_Pin)  {
          return HAL_ERROR;
        }
      }

      //Set output data
      GPIOA->BSRR = (tx[curByte] & curBit) ? (RJ12_MISO_Pin) : (RJ12_MISO_Pin << 16);

      //Wait for rising edge on SCK
      while(!(GPIOA->IDR & RJ12_SCK_Pin)) {
        if(HAL_GetTick() > time)      {
          return HAL_TIMEOUT;
        }
        if(GPIOA->IDR & RJ12_CS_Pin)  {
          return HAL_ERROR;
        }
      }

      //Read input
      rx[curByte] |= (GPIOA->IDR & RJ12_MOSI_Pin) ? curBit : 0;
    }
  }

  return HAL_OK;
}



/*******************************************************************************
 * Packs an IO-struct into a format, which can be sent to the remote-unit.
 *
 * @param pData The data (IO-struct) which will be used to create the package.
 * @param pPackage The package which will be filled with data (uint8_t x[10])
 * @return nothing
 *******************************************************************************/
static inline void joystickunit_packData(RemoteIO_States_t* data, uint8_t* package) {
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

  package[9] = joystickunit_calcCRC(package, 9);
}


/*******************************************************************************
 * Unpacks a package received from the remote-unit and stores its contents to
 * the IO-struct.
 *
 * @param pData A pointer to the IO-struct which will be filled.
 * @param pPackage The package received from the remote-unit (uint8_t x[10])
 * @return nothing
 *******************************************************************************/
static inline void joystickunit_unpackData(RemoteIO_States_t* data, uint8_t* package) {
  //Convert analog
  data->analog[0] = package[0] | ((package[1] & 0x0F) << 8);
  data->analog[1] = package[2] | ((package[3] & 0x0F) << 8);
  data->analog[2] = package[4] | ((package[5] & 0x0F) << 8);
  data->analog[3] = package[6] | ((package[7] & 0x0F) << 8);

  //Convert digital
  data->digital[ 0] = EXTRACT_GPIO_VAL(package[1], 4);
  data->digital[ 1] = EXTRACT_GPIO_VAL(package[1], 5);
  data->digital[ 2] = EXTRACT_GPIO_VAL(package[1], 6);
  data->digital[ 3] = EXTRACT_GPIO_VAL(package[1], 7);

  data->digital[ 4] = EXTRACT_GPIO_VAL(package[3], 4);
  data->digital[ 5] = EXTRACT_GPIO_VAL(package[3], 5);
  data->digital[ 6] = EXTRACT_GPIO_VAL(package[3], 6);
  data->digital[ 7] = EXTRACT_GPIO_VAL(package[3], 7);

  data->digital[ 8] = EXTRACT_GPIO_VAL(package[5], 4);
  data->digital[ 9] = EXTRACT_GPIO_VAL(package[5], 5);
  data->digital[10] = EXTRACT_GPIO_VAL(package[5], 6);
  data->digital[11] = EXTRACT_GPIO_VAL(package[5], 7);

  data->digital[12] = EXTRACT_GPIO_VAL(package[7], 4);
  data->digital[13] = EXTRACT_GPIO_VAL(package[7], 5);
  data->digital[14] = EXTRACT_GPIO_VAL(package[7], 6);
  data->digital[15] = EXTRACT_GPIO_VAL(package[7], 7);

  data->digital[16] = EXTRACT_GPIO_VAL(package[8], 0);
  data->digital[17] = EXTRACT_GPIO_VAL(package[8], 1);
  data->digital[18] = EXTRACT_GPIO_VAL(package[8], 2);
  data->digital[19] = EXTRACT_GPIO_VAL(package[8], 3);
  data->digital[20] = EXTRACT_GPIO_VAL(package[8], 4);
  data->digital[21] = EXTRACT_GPIO_VAL(package[8], 5);
  data->digital[22] = EXTRACT_GPIO_VAL(package[8], 6);
  data->digital[23] = EXTRACT_GPIO_VAL(package[8], 7);
}


/*******************************************************************************
 * Calculates the checksum from a given uint8_t-array 'pData' with length 'len'.
 *
 * @param pData A pointer to the data.
 * @param pPackage The size of the data-array
 * @return The CRC-Checksum
 *******************************************************************************/
uint8_t joystickunit_calcCRC(uint8_t* data, uint32_t len) {
  uint8_t crc = CRC_START_VALUE;

  while (len--) {
    crc = crc8_table[crc ^ *data++];
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
static inline bool joystickunit_checkCRC(uint8_t* data, uint32_t len) {
  uint8_t calculatedCRC = joystickunit_calcCRC(data, len);

  if(calculatedCRC == data[len]) {
    return true;
  } else {
    return false;
  }
}
