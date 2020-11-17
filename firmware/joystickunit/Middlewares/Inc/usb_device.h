/*******************************************************************************
* @file         : usb_device.c
* @project      : 4D-Joystick, Joystick-Unit
* @author       : Fabian Baer
* @brief        : Defines the USB-CDC-device
*******************************************************************************/

#ifndef __MIDDLEWARE_INC_USB_DEVICE_H
#define __MIDDLEWARE_INC_USB_DEVICE_H

#include <stm32l4xx.h>
#include <stm32l4xx_hal.h>
#include <usbd_def.h>

#define USBD_VID                      1155
#define USBD_LANGID_STRING            1033
#define USBD_MANUFACTURER_STRING      "FHTW"
#define USBD_PID_FS                   22336
#define USBD_PRODUCT_STRING_FS        "4D-Joystick"
#define USBD_CONFIGURATION_STRING_FS  "CDC Config"
#define USBD_INTERFACE_STRING_FS      "CDC Interface"

void usbDevice_init(void);
uint8_t CDC_Transmit_FS(uint8_t* Buf, uint16_t Len);

#endif /* __MIDDLEWARE_INC_USB_DEVICE_H */
