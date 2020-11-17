/*******************************************************************************
 * @file         : remoteunit.h
 * @project      : 4D-Joystick, Joystick-Unit
 * @author       : Fabian Baer
 * @brief        : Handels all interactions with remote-unit
 ******************************************************************************/

#ifndef __CORE_INC_REMOTEUNIT_H_
#define __CORE_INC_REMOTEUNIT_H_

#include <cmsis_os.h>

typedef struct {
  uint32_t rem_x;
  uint32_t rem_y;
  uint32_t rem_z;
  uint32_t rem_w;
  uint32_t joy_x;
  uint32_t joy_y;
  uint32_t joy_z;
  uint32_t joy_w;
} RemoteUnit_adcStates_t;

void remoteunit_init( void );
void remoteunit_setBuddyButtonsMQ( osMessageQId msgQueue );
void remoteunit_setupTask( osPriority priority );
void remoteunit_terminateTask( void );
void remoteunit_reloadConfig( void );
RemoteUnit_adcStates_t remoteunit_getADC( void );
bool remoteunit_isTeachermodeActive( void );

#endif /* __CORE_INC_REMOTEUNIT_H_ */
