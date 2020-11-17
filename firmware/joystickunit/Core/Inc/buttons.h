/*******************************************************************************
* @file         : buttons.h
* @project      : 4D-Joystick, Joystick-Unit
* @author       : Fabian Baer
* @brief        : Handles debouncing of the buddybuttons and rotary encoder
*******************************************************************************/

#ifndef __CORE_INC_BUTTONS_H_
#define __CORE_INC_BUTTONS_H_

#include <cmsis_os.h>
#include <configHandler.h>

typedef union {
  struct {
    Config_BuddyButton_t buddyId;
    enum {
      buddybutton_none,
      buddybutton_pressed,
      buddybutton_released
    } event;
  } fields;
  uint32_t bits;
} BuddyButtonMessage_t;

typedef union {
  enum {
    rotaryEncoder_none,
    rotaryEncoder_pressed,
    rotaryEncoder_released,
    rotaryEncoder_clockwise,
    rotaryEncoder_counterclockwise
  } event;
  uint32_t bits;
} RotaryEncoderMessage_t;

void buttons_init( void );
void buttons_setupTask( osPriority priority );
osMessageQId buttons_getBuddyButtonsMQ( void );
osMessageQId buttons_getRotaryEncoderMQ( void );

#endif /* __CORE_INC_BUTTONS_H_ */
