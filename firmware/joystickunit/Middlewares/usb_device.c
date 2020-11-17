/*******************************************************************************
* @file         : usb_device.c
* @project      : 4D-Joystick, Joystick-Unit
* @author       : Fabian Baer
* @brief        : Defines the USB-CDC-device
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include <cmsis_os.h>
#include <usb_device.h>
#include <usbd_core.h>
#include <usbd_desc.h>
#include <usbd_cdc.h>
#include <system.h>
#include <cli.h>


/* Defines -------------------------------------------------------------------*/
#define RX_BUFFER_SIZE  2048
#define TX_BUFFER_SIZE  2048


/* Prototypes ----------------------------------------------------------------*/
static int8_t CDC_Init_FS(void);
static int8_t CDC_DeInit_FS(void);
static int8_t CDC_Control_FS(uint8_t cmd, uint8_t* pbuf, uint16_t length);
static int8_t CDC_Receive_FS(uint8_t* pbuf, uint32_t *Len);


/* Variables -----------------------------------------------------------------*/
USBD_HandleTypeDef hUsbDeviceFS;
extern USBD_DescriptorsTypeDef FS_Desc;
extern PCD_HandleTypeDef hpcd_USB_OTG_FS;
extern CLI_Handle_t hcli_usb;
USBD_CDC_ItfTypeDef USBD_Interface_fops_FS = {
    CDC_Init_FS,
    CDC_DeInit_FS,
    CDC_Control_FS,
    CDC_Receive_FS
};
uint8_t UserRxBufferFS[RX_BUFFER_SIZE];
uint8_t UserTxBufferFS[TX_BUFFER_SIZE];
uint32_t bufCnt = 0;


/* Code ----------------------------------------------------------------------*/
void usbDevice_init(void) {
  if (USBD_Init(&hUsbDeviceFS, &FS_Desc, DEVICE_FS) != USBD_OK) {
    system_errorHandler();
  }
  if (USBD_RegisterClass(&hUsbDeviceFS, &USBD_CDC) != USBD_OK) {
    system_errorHandler();
  }
  if (USBD_CDC_RegisterInterface(&hUsbDeviceFS, &USBD_Interface_fops_FS)
      != USBD_OK) {
    system_errorHandler();
  }
  if (USBD_Start(&hUsbDeviceFS) != USBD_OK) {
    system_errorHandler();
  }

}

void OTG_FS_IRQHandler(void) {
  HAL_PCD_IRQHandler(&hpcd_USB_OTG_FS);
}

static int8_t CDC_Init_FS(void) {
  USBD_CDC_SetTxBuffer(&hUsbDeviceFS, UserTxBufferFS, 0);
  USBD_CDC_SetRxBuffer(&hUsbDeviceFS, UserRxBufferFS);
  return USBD_OK;
}

static int8_t CDC_DeInit_FS(void) {
  return USBD_OK;
}

static int8_t CDC_Control_FS(uint8_t cmd, uint8_t* pbuf, uint16_t length) {
  switch(cmd) {
    case CDC_SEND_ENCAPSULATED_COMMAND:
    break;

    case CDC_GET_ENCAPSULATED_RESPONSE:
    break;

    case CDC_SET_COMM_FEATURE:
    break;

    case CDC_GET_COMM_FEATURE:
    break;

    case CDC_CLEAR_COMM_FEATURE:
    break;

  /*******************************************************************************/
  /* Line Coding Structure                                                       */
  /*-----------------------------------------------------------------------------*/
  /* Offset | Field       | Size | Value  | Description                          */
  /* 0      | dwDTERate   |   4  | Number |Data terminal rate, in bits per second*/
  /* 4      | bCharFormat |   1  | Number | Stop bits                            */
  /*                                        0 - 1 Stop bit                       */
  /*                                        1 - 1.5 Stop bits                    */
  /*                                        2 - 2 Stop bits                      */
  /* 5      | bParityType |  1   | Number | Parity                               */
  /*                                        0 - None                             */
  /*                                        1 - Odd                              */
  /*                                        2 - Even                             */
  /*                                        3 - Mark                             */
  /*                                        4 - Space                            */
  /* 6      | bDataBits  |   1   | Number Data bits (5, 6, 7, 8 or 16).          */
  /*******************************************************************************/
    case CDC_SET_LINE_CODING:
    break;

    case CDC_GET_LINE_CODING:
    break;

    case CDC_SET_CONTROL_LINE_STATE:
    break;

    case CDC_SEND_BREAK:
    break;

  default:
    break;
  }

  return USBD_OK;
}

static int8_t CDC_Receive_FS(uint8_t* Buf, uint32_t *Len) {
  USBD_CDC_SetRxBuffer(&hUsbDeviceFS, &Buf[0]);
  USBD_CDC_ReceivePacket(&hUsbDeviceFS);

  for (uint32_t i = 0; i < *Len; i++) {
    uint8_t x = Buf[i];

    //Receive raw data if requested
    if(hcli_usb.flags & CLI_FLAG_RX_RAW) {
      if(x == '\r') {
        hcli_usb.flags  |= CLI_FLAG_RX_RETURN;
      } else {
        if(hcli_usb.rxLength < RX_BUFFER_SIZE) {
          hcli_usb.rxBuffer[hcli_usb.rxLength++] = x;
        }
      }
      continue;
    }

    //Received normal data
    if(hcli_usb.flags & CLI_FLAG_RX_ESCAPE) {
      //Last character was escape
      hcli_usb.flags  &= ~CLI_FLAG_RX_ESCAPE;

      if(*Len == 3 && Buf[1] == '[') {
        if(Buf[2] == 'C') {
          //Right key
          if(hcli_usb.rxCursor < hcli_usb.rxLength) {
            hcli_usb.rxCursor++;
            CDC_Transmit_FS((uint8_t*) "\e[C", 3);
          }
        } else if(Buf[2] == 'D') {
          //Left key
          if(hcli_usb.rxCursor > 0) {
            hcli_usb.rxCursor--;
            CDC_Transmit_FS((uint8_t*) "\e[D", 3);
          }
        }
        break;
      }
    } else {
      switch(x) {
        //Return received
        case '\r':
          hcli_usb.flags |= CLI_FLAG_RX_RETURN;
          break;

        //Backspace received
        case '\b':
        case 127:
          if (hcli_usb.rxLength > 0 && hcli_usb.rxCursor != 0) {
            if (hcli_usb.rxLength == hcli_usb.rxCursor) {
              hcli_usb.rxCursor--;
              hcli_usb.rxLength--;
            } else {
              for(uint32_t i = hcli_usb.rxCursor-1; i<hcli_usb.rxLength-1; i++) {
                hcli_usb.rxBuffer[i] = hcli_usb.rxBuffer[i+1];
              }
              hcli_usb.rxCursor--;
              hcli_usb.rxLength--;
            }
          }
          hcli_usb.flags |= CLI_FLAG_REPRINT;
          break;

        //Abort received (Ctrl+C)
        case '\x03':
          hcli_usb.flags |= CLI_FLAG_RX_ABORT;
          break;

        //Escape sequence
        case '\e':
          hcli_usb.flags |= CLI_FLAG_RX_ESCAPE;
          break;

        //Normal characters
        default:
          if (x >= ' ' && x <= '~' && hcli_usb.rxLength < RX_BUFFER_SIZE) {
            if (hcli_usb.rxLength == hcli_usb.rxCursor) {
              hcli_usb.rxCursor++;
              hcli_usb.rxBuffer[hcli_usb.rxLength++] = x;
              CDC_Transmit_FS(&x, 1);
            } else {
              for (uint32_t i = hcli_usb.rxLength; i > hcli_usb.rxCursor; i--) {
                hcli_usb.rxBuffer[i] = hcli_usb.rxBuffer[i-1];
              }
              hcli_usb.rxBuffer[hcli_usb.rxCursor] = x;
              hcli_usb.rxCursor++;
              hcli_usb.rxLength++;
              hcli_usb.flags |= CLI_FLAG_REPRINT;
            }
          }
          break;
      }
    }
  }

  return USBD_OK;
}

uint8_t CDC_Transmit_FS(uint8_t* Buf, uint16_t Len) {
  uint8_t result = USBD_OK;
  USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef*)hUsbDeviceFS.pClassData;
  while (hcdc->TxState != 0){
    osDelay(1);
  }
  USBD_CDC_SetTxBuffer(&hUsbDeviceFS, Buf, Len);
  result = USBD_CDC_TransmitPacket(&hUsbDeviceFS);
  return result;
}

