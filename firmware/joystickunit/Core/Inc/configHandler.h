/*******************************************************************************
* @file         : configController.h
* @project      : 4D-Joystick, Joystick-Unit
* @author       : Fabian Baer
* @brief        : Handles the different configurations and related storage
*******************************************************************************/

#ifndef __CORE_INC_CONFIGHANDLER_H_
#define __CORE_INC_CONFIGHANDLER_H_

#include <stdbool.h>
#include <cmsis_os.h>

#define DIGITAL_PORT_NOT_USED   0xFF

typedef enum {
  ConfigHandler_OK,
  ConfigHandler_Error
} ConfigHandler_Status_t;

typedef enum {
  sysconf_switch_none = 0,
  sysconf_switch_2pos = 1,
  sysconf_switch_3pos = 2,
  sysconf_momentary_2pos = 3
} SysConf_Switch_t;

typedef enum {
  configType_remoteunit,
  configType_undefined    /* Must be the last element! */
} Config_Type_t;

typedef enum {
  analog_in_x = 0,
  analog_in_y = 1,
  analog_in_z = 2,
  analog_in_w = 3,
  analog_in_rx = 4,
  analog_in_ry = 5,
  analog_in_rz = 6,
  analog_in_rw = 7,
  analog_in_none
} Config_Analog_In_t;

typedef enum {
  analog_out_x = 0,
  analog_out_y = 1,
  analog_out_z = 2,
  analog_out_w = 3
} Config_Analog_Out_t;

typedef enum {
  buddyButton1 = 0,
  buddyButton2 = 1,
  buddyButton3 = 2,
  buddyButton4 = 3
} Config_BuddyButton_t;

typedef struct {
  uint32_t initSequence;

  //Configuration slot information
  uint32_t activeSlots;
  uint8_t currentSlot;

  //Configuration of digital switches
  uint8_t axis_channels[4];
  SysConf_Switch_t switch_types[26];
  uint8_t switch_ch1[26];
  uint8_t switch_ch2[26];

  //Calibration data for analog channels
  uint16_t aIn_midpoint[4];
  uint16_t aIn_margin[4];
  bool aIn_inverted[4];
  uint16_t aOut_midpoint[4];
  uint16_t aOut_margin[4];
  bool aOut_inverted[4];
} SystemConfiguration_t;

typedef struct {
  Config_Type_t type;
  uint8_t name[16];

  //Configuration-type specific items
  union {
    struct {
      uint8_t dOut[4];
      Config_Analog_In_t aOut[4];
      uint8_t aIn_deadzone[4];
      uint8_t teacher_Port;
    } remoteunit;
  };
} Configuration_t;


void configHandler_init( void );
void configHandler_setupTaskOfCurrentConfigType( void );
Configuration_t* configHandler_getCurrentConfig( void );
Configuration_t* configHandler_getConfigs( void );
SystemConfiguration_t* configHandler_getSystemConfig( void );
uint32_t configHandler_getFirstFreeSlot( void );
ConfigHandler_Status_t configHandler_loadConfig( uint8_t slot );
ConfigHandler_Status_t configHandler_newConfig( Config_Type_t type, uint8_t slot, uint8_t* name, uint32_t lengthName );
ConfigHandler_Status_t configHandler_deleteConfig( uint8_t slot );
ConfigHandler_Status_t configHandler_copyConfig( uint8_t source, uint8_t destination );
ConfigHandler_Status_t configHandler_moveConfig( uint8_t source, uint8_t destination );
ConfigHandler_Status_t configHandler_renameConfig( uint8_t* name, uint32_t lengthName );
ConfigHandler_Status_t configHandler_setAxis( Config_Analog_In_t aIn, Config_Analog_Out_t aOut );
ConfigHandler_Status_t configHandler_setDeadzone( Config_Analog_In_t aIn, uint8_t deadzone );
ConfigHandler_Status_t configHandler_setBuddyButton( Config_BuddyButton_t dIn, uint8_t dOut );
ConfigHandler_Status_t configHandler_setTeacherPort( uint8_t dIn );
ConfigHandler_Status_t configHandler_setAnalogInCalibration( Config_Analog_In_t aIn, uint32_t midpoint, uint32_t margin, bool inverted );
ConfigHandler_Status_t configHandler_setAnalogOutCalibration( Config_Analog_Out_t aOut, uint32_t midpoint, uint32_t margin, bool inverted );
ConfigHandler_Status_t configHandler_setGlobalSwitches( uint8_t id, SysConf_Switch_t type, uint8_t ch1, uint8_t ch2 );
ConfigHandler_Status_t configHandler_setGlobalAxis( Config_Analog_Out_t out, uint8_t ch );
ConfigHandler_Status_t configHandler_generateBackup( uint8_t* buffer, uint32_t length );
ConfigHandler_Status_t configHandler_restoreBackup( uint8_t* buffer, uint32_t length );

#endif /* __CORE_INC_CONFIGHANDLER_H_ */
