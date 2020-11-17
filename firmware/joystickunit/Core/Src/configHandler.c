/*******************************************************************************
* @file         : configController.c
* @project      : 4D-Joystick, Joystick-Unit
* @author       : Fabian Baer
* @brief        : Handles the different configurations and related storage
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include <stddef.h>
#include <stdbool.h>
#include <cmsis_os.h>
#include <stm32l4xx_hal.h>
#include <board.h>
#include <system.h>
#include <configHandler.h>
#include <remoteunit.h>
#include <eeprom.h>
#include <userinterface.h>


/* Defines -------------------------------------------------------------------*/
#define STORAGE_INIT_SEQUENCE         (uint32_t)0xAF06CAFA
#define ADDR_SYSCONFIG                (uint32_t)0x0000
#define NC                            DIGITAL_PORT_NOT_USED


/* Macros -------------------------------------------------------------------*/
#define ADDR_CONFIG(slot)                     (uint32_t)((slot+1) << 8)
#define ADDR_SYSCONFIG_ITEM(item)             (ADDR_SYSCONFIG | offsetof(SystemConfiguration_t, item))
#define ADDR_CONFIG_ITEM(slot, item)          (ADDR_CONFIG(slot) | offsetof(Configuration_t, item))
#define IS_SLOT_ACTIVE(slot)                  (slot < 32 && ((1 << slot) & sysConfig.activeSlots))
#define IS_CURRENT_CONFIG_OF_TYPE(confType)   (configurations[sysConfig.currentSlot].type == confType)
#define MEMBER_SIZE(type, member)             sizeof(((type *)0)->member)
#define STORE_SYSCONFIG_ITEM(item)            eeprom_write(ADDR_SYSCONFIG_ITEM(item), (uint8_t*)&sysConfig.item, MEMBER_SIZE(SystemConfiguration_t, item))
#define STORE_CONFIG_ITEM(slot, item)         eeprom_write(ADDR_CONFIG_ITEM(slot, item), (uint8_t*)&configurations[slot].item, MEMBER_SIZE(Configuration_t, item))


/* Prototypes ----------------------------------------------------------------*/
static inline bool configHandler_isStorageInitalized( void );
static inline void configHandler_writeConfigToStorage( uint32_t slot, Configuration_t* pConfig );
static inline void configHandler_loadSysConfigFromStorage( void );
static inline void configHandler_loadConfigFromStorage( uint32_t slot );
static void configHandler_forceConfigTaskToReloadConfig( void );
static ConfigHandler_Status_t configHandler_loadConfig_withoutSemaphore( uint32_t slot );


/* Variables -----------------------------------------------------------------*/
static osSemaphoreDef(hsem_config);
static osSemaphoreId(hsem_config);
static Configuration_t configurations[32];
static SystemConfiguration_t sysConfig = {
    .initSequence = STORAGE_INIT_SEQUENCE,
    .activeSlots = 0x00000001,
    .currentSlot = 0x00,
    .axis_channels = {2, 3, 1, 0},
    .switch_types = {sysconf_switch_3pos, sysconf_switch_3pos, sysconf_switch_3pos,
        sysconf_switch_3pos, sysconf_switch_3pos, sysconf_switch_2pos, sysconf_switch_3pos,
        sysconf_momentary_2pos, sysconf_momentary_2pos, sysconf_momentary_2pos,
        sysconf_momentary_2pos, sysconf_momentary_2pos, sysconf_momentary_2pos,
        sysconf_momentary_2pos, sysconf_momentary_2pos, sysconf_momentary_2pos,
        sysconf_momentary_2pos, sysconf_switch_none, sysconf_switch_none,
        sysconf_switch_none, sysconf_switch_none, sysconf_switch_none,
        sysconf_switch_none, sysconf_switch_none, sysconf_switch_none,
        sysconf_switch_none},
    .switch_ch1 = {14, 11, 10,  7, 16, 23,  5, 22, 13, 21, 20, 19, 18,  0,  1,  2,  3,
        NC, NC, NC, NC, NC, NC, NC, NC, NC},
    .switch_ch2 = {15, 12,  9,  6, 17, NC,  4, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC,
        NC, NC, NC, NC, NC, NC, NC, NC, NC},
    .aIn_midpoint = {2047, 2047, 2047, 2047},
    .aIn_margin = {2000, 2000, 2000, 2000},
    .aIn_inverted = {false, false, false, false},
    .aOut_midpoint = {2047, 2047, 2047, 2047},
    .aOut_margin = {2000, 2000, 2000, 2000},
    .aOut_inverted = {false, false, false, false}
};

static const Configuration_t defaultRemoteunitConfig = {
    .type = configType_remoteunit,
    .name = "Default",
    .remoteunit = {
      .teacher_Port = DIGITAL_PORT_NOT_USED,
      .aOut[analog_out_x] = analog_in_none,
      .aOut[analog_out_y] = analog_in_none,
      .aOut[analog_out_z] = analog_in_none,
      .aOut[analog_out_w] = analog_in_none,
      .aIn_deadzone[analog_in_x] = 0,
      .aIn_deadzone[analog_in_y] = 0,
      .aIn_deadzone[analog_in_z] = 0,
      .aIn_deadzone[analog_in_w] = 0,
      .dOut[buddyButton1] = DIGITAL_PORT_NOT_USED,
      .dOut[buddyButton2] = DIGITAL_PORT_NOT_USED,
      .dOut[buddyButton3] = DIGITAL_PORT_NOT_USED,
      .dOut[buddyButton4] = DIGITAL_PORT_NOT_USED
    }
};


/* Code ----------------------------------------------------------------------*/

/*******************************************************************************
 * Initializes configuration related hardware (EEPROM), semaphore and structs.
 *
 * @return nothing
 *******************************************************************************/
void configHandler_init( void ) {
  //Init hardware
  eeprom_init();

  //Check if storage is initialized
  if(!configHandler_isStorageInitalized()) {
    eeprom_write(ADDR_SYSCONFIG, (uint8_t*)&sysConfig, sizeof(SystemConfiguration_t));
    configHandler_writeConfigToStorage(0, (Configuration_t*)&defaultRemoteunitConfig);
  }

  //Load configurations
  configHandler_loadSysConfigFromStorage();
  for(uint32_t slot = 0; slot<32; slot++) {
    if(IS_SLOT_ACTIVE(slot)) {
      configHandler_loadConfigFromStorage(slot);
    }
  }

  //Init semaphore
  hsem_config = osSemaphoreCreate(osSemaphore(hsem_config), 1);
}


/*******************************************************************************
 * Initializes the task which is related to the currently loaded configuration.
 *
 * @return nothing
 *******************************************************************************/
void configHandler_setupTaskOfCurrentConfigType( void ) {
  switch(configurations[sysConfig.currentSlot].type) {
  case configType_remoteunit:
    remoteunit_setupTask( osPriorityHigh );
    break;
  case configType_undefined:
  default:
    //do not start a task
    break;
  }
}


/*******************************************************************************
 * Returns a pointer to the currently loaded configuration.
 *
 * @return pointer to the configuration
 *******************************************************************************/
Configuration_t* configHandler_getCurrentConfig( void ) {
  return &(configurations[sysConfig.currentSlot]);
}


/*******************************************************************************
 * Returns a pointer to the configuration-array.
 *
 * @return pointer to the configurations-array
 *******************************************************************************/
Configuration_t* configHandler_getConfigs( void ) {
  return configurations;
}


/*******************************************************************************
 * Returns a pointer to the system configuration.
 *
 * @return pointer to the system configuration
 *******************************************************************************/
SystemConfiguration_t* configHandler_getSystemConfig( void ) {
  return &sysConfig;
}


/*******************************************************************************
 * Writes a configuration to the EEPROM at a specific slot.
 *
 * @param slot The slot number to store the config.
 * @param config Pointer to the configuration
 * @return nothing
 *******************************************************************************/
static inline void configHandler_writeConfigToStorage( uint32_t slot, Configuration_t* pConfig ) {
  eeprom_write(ADDR_CONFIG(slot), (uint8_t*)pConfig, sizeof(Configuration_t));
}


/*******************************************************************************
 * Checks if storage is initialized. The storage is initialized if the first
 * four bytes are equal to 'STORAGE_INIT_SEQUENCE'
 *
 * @return true if storage is initialized
 *******************************************************************************/
static inline bool configHandler_isStorageInitalized( void ) {
  uint32_t data;

  eeprom_read(ADDR_SYSCONFIG_ITEM(initSequence), (uint8_t*)&data, sizeof(uint32_t));

  return (data == STORAGE_INIT_SEQUENCE) ? true : false;
}


/*******************************************************************************
 * Loads the system configuration form the EEPROM.
 *
 * @return nothing
 *******************************************************************************/
static inline void configHandler_loadSysConfigFromStorage( void ) {
  eeprom_read(ADDR_SYSCONFIG, (uint8_t*)&sysConfig, sizeof(SystemConfiguration_t));
}


/*******************************************************************************
 * Loads a configuration form the EEPROM.
 *
 * @param slot The slot to load.
 * @return nothing
 *******************************************************************************/
static inline void configHandler_loadConfigFromStorage( uint32_t slot ) {
  eeprom_read(ADDR_CONFIG(slot), (uint8_t*)&configurations[slot], sizeof(Configuration_t));
}


/*******************************************************************************
 * Forces the task of the current configuration to reload the configuration.
 *
 * @return nothing
 *******************************************************************************/
static void configHandler_forceConfigTaskToReloadConfig( void ) {
  if(configurations[sysConfig.currentSlot].type == configType_remoteunit) {
    remoteunit_reloadConfig();
  }
}


/*******************************************************************************
 * Forces the task of a configuration to terminate.
 *
 * @param type The type of the configuration.
 * @return nothing
 *******************************************************************************/
static void configHandler_forceSlaveToTerminate(Config_Type_t type) {
  switch(type) {
  case configType_remoteunit:
    remoteunit_terminateTask();
    break;
  case configType_undefined:
  default:
    //do not start a task
    break;
  }
}


/*******************************************************************************
 * Loads another configuration without blocking the semaphore (for internal use
 * only)
 *
 * @param slot The slot of the configuration to load.
 * @return 'ConfigHandler_OK' in case of success
 *******************************************************************************/
static ConfigHandler_Status_t configHandler_loadConfig_withoutSemaphore( uint32_t slot ) {
  ConfigHandler_Status_t retVal = ConfigHandler_Error;
  Config_Type_t oldType = configType_undefined;

  if(IS_SLOT_ACTIVE(slot)) {
    oldType = configurations[sysConfig.currentSlot].type;

    //Switch config
    sysConfig.currentSlot = slot;
    STORE_SYSCONFIG_ITEM(currentSlot);

    //Switch Task if another config-type was loaded
    if(oldType != configurations[sysConfig.currentSlot].type) {
      configHandler_forceSlaveToTerminate(oldType);
      configHandler_setupTaskOfCurrentConfigType();
    } else {
      configHandler_forceConfigTaskToReloadConfig();
    }

    retVal = ConfigHandler_OK;
  }

  return retVal;
}


/*******************************************************************************
 * Loads another configuration (for external use)
 *
 * @param slot The slot of the configuration to load.
 * @return 'ConfigHandler_OK' in case of success
 *******************************************************************************/
ConfigHandler_Status_t configHandler_loadConfig( uint8_t slot ) {
  ConfigHandler_Status_t retVal = ConfigHandler_Error;
  osSemaphoreWait(hsem_config, osWaitForever);

  retVal = configHandler_loadConfig_withoutSemaphore(slot);

  osSemaphoreRelease(hsem_config);
  return retVal;
}


/*******************************************************************************
 * Creates a new configuration and set it as active configuration.
 *
 * @param type The type of the new configuration
 * @param slot The slot to store the configuration.
 * @param name The name of the config (max. 15 chars)
 * @param lengthName The length of the name.
 * @return 'ConfigHandler_OK' in case of success
 *******************************************************************************/
ConfigHandler_Status_t configHandler_newConfig( Config_Type_t type, uint8_t slot, uint8_t* name, uint32_t lengthName ) {
  ConfigHandler_Status_t retVal = ConfigHandler_Error;
  osSemaphoreWait(hsem_config, osWaitForever);

  if(!IS_SLOT_ACTIVE(slot) && lengthName < 16 && type < configType_undefined) {
    //Load default configuration
    switch(type) {
      case configType_remoteunit:
      case configType_undefined:
      default:
        configurations[slot] = defaultRemoteunitConfig;
        break;
    }

    //Add new name
    for(uint32_t i = 0; i<lengthName; i++) {
      configurations[slot].name[i] = name[i];
    }
    configurations[slot].name[lengthName] = '\0';

    //Store configuration
    configHandler_writeConfigToStorage(slot, &configurations[slot]);

    //Set slot to active
    sysConfig.activeSlots |= 1 << slot;
    STORE_SYSCONFIG_ITEM(activeSlots);

    //Set slot to active
    configHandler_loadConfig_withoutSemaphore(slot);

    retVal = ConfigHandler_OK;
  }

  osSemaphoreRelease(hsem_config);
  return retVal;
}

/*******************************************************************************
 * Returns the first available empty slot
 *
 * @return The first empty slot, 32 if no one is empty
 *******************************************************************************/
uint32_t configHandler_getFirstFreeSlot(void) {
  uint32_t i;

  for(i = 0; i<32; i++) {
    if(!IS_SLOT_ACTIVE(i)) {
      return i;
    }
  }

  return i;
}


/*******************************************************************************
 * Deletes a configuration. Loads the next available if it was the active one.
 * If there is no configuration left, the default config is stored to slot 0.
 *
 * @param slot The slot to delete.
 * @return 'ConfigHandler_OK' in case of success
 *******************************************************************************/
ConfigHandler_Status_t configHandler_deleteConfig( uint8_t slot ) {
  ConfigHandler_Status_t retVal = ConfigHandler_Error;
  osSemaphoreWait(hsem_config, osWaitForever);

  if(IS_SLOT_ACTIVE(slot)) {
    //Deactivate slot
    sysConfig.activeSlots &= ~(1 << slot);
    STORE_SYSCONFIG_ITEM(activeSlots);

    //Store default config, if this was the last one
    if(sysConfig.activeSlots == 0x00) {
      configurations[0] = defaultRemoteunitConfig;
      configHandler_writeConfigToStorage(0, &configurations[0]);
      sysConfig.activeSlots = 0x01;
      STORE_SYSCONFIG_ITEM(activeSlots);
    }

    //Load first available config, if this was the active one
    if(sysConfig.currentSlot == slot) {
      for(uint32_t i=0; i<32; i++) {
        if(IS_SLOT_ACTIVE(i)) {
          configHandler_loadConfig_withoutSemaphore(i);
          break;
        }
      }
    }

    retVal = ConfigHandler_OK;
  }

  osSemaphoreRelease(hsem_config);
  return retVal;
}


/*******************************************************************************
 * Copies an configuration from source-slot to the destination port. Loads the
 * new configuration (destination-slot).
 *
 * @param source The source slot.
 * @param destination The destination slot.
 * @return 'ConfigHandler_OK' in case of success
 *******************************************************************************/
ConfigHandler_Status_t configHandler_copyConfig( uint8_t source, uint8_t destination ) {
  ConfigHandler_Status_t retVal = ConfigHandler_Error;
  osSemaphoreWait(hsem_config, osWaitForever);

  if(IS_SLOT_ACTIVE(source) && !IS_SLOT_ACTIVE(destination)) {
    //Copy configuration
    configurations[destination] = configurations[source];
    configHandler_writeConfigToStorage(destination, &configurations[destination]);

    //Set slot to active
    sysConfig.activeSlots |= (1 << destination);
    STORE_SYSCONFIG_ITEM(activeSlots);

    //Load the new slot
    configHandler_loadConfig_withoutSemaphore(destination);

    retVal = ConfigHandler_OK;
  }

  osSemaphoreRelease(hsem_config);
  return retVal;
}


/*******************************************************************************
 * Moves a config to another slot (destination-slot).
 *
 * @param source The source slot.
 * @param destination The destination slot.
 * @return 'ConfigHandler_OK' in case of success
 *******************************************************************************/
ConfigHandler_Status_t configHandler_moveConfig( uint8_t source, uint8_t destination ) {
  ConfigHandler_Status_t retVal = ConfigHandler_Error;
  osSemaphoreWait(hsem_config, osWaitForever);

  if(IS_SLOT_ACTIVE(source) && !IS_SLOT_ACTIVE(destination)) {
    //Copy configuration
    configurations[destination] = configurations[source];
    configHandler_writeConfigToStorage(destination, &configurations[destination]);

    //Set slot to active
    sysConfig.activeSlots |= (1 << destination);
    sysConfig.activeSlots &= ~(1 << source);
    STORE_SYSCONFIG_ITEM(activeSlots);

    if(sysConfig.currentSlot == source) {
      configHandler_loadConfig_withoutSemaphore(destination);
    }

    retVal = ConfigHandler_OK;
  }

  osSemaphoreRelease(hsem_config);
  return retVal;
}


/*******************************************************************************
 * Renames the currently loaded slot
 *
 * @param name The new name (max. 15 chars)
 * @param lengthName The length of the name.
 * @return 'ConfigHandler_OK' in case of success
 *******************************************************************************/
ConfigHandler_Status_t configHandler_renameConfig( uint8_t* name, uint32_t lengthName ) {
  ConfigHandler_Status_t retVal = ConfigHandler_Error;
  osSemaphoreWait(hsem_config, osWaitForever);

  if(lengthName < 16) {
    //Add new name
    for(uint32_t i = 0; i<lengthName; i++) {
      configurations[sysConfig.currentSlot].name[i] = name[i];
    }
    configurations[sysConfig.currentSlot].name[lengthName] = '\0';
    eeprom_write(ADDR_CONFIG_ITEM(sysConfig.currentSlot, name), configurations[sysConfig.currentSlot].name, MEMBER_SIZE(Configuration_t, name));

    //Update display to show new name
    ui_updateDisplay();

    retVal = ConfigHandler_OK;
  }

  osSemaphoreRelease(hsem_config);
  return retVal;
}


/*******************************************************************************
 * Set the axis of the current configuration
 *
 * @param in Analog input channel
 * @param out Analog output channel
 * @return 'ConfigHandler_OK' in case of success
 *******************************************************************************/
ConfigHandler_Status_t configHandler_setAxis( Config_Analog_In_t in, Config_Analog_Out_t out ) {
  ConfigHandler_Status_t retVal = ConfigHandler_Error;
  osSemaphoreWait(hsem_config, osWaitForever);

  if(configurations[sysConfig.currentSlot].type == configType_remoteunit) {
    configurations[sysConfig.currentSlot].remoteunit.aOut[out] = in;
    STORE_CONFIG_ITEM(sysConfig.currentSlot, remoteunit.aOut[out]);

    configHandler_forceConfigTaskToReloadConfig();
    retVal = ConfigHandler_OK;
  }

  osSemaphoreRelease(hsem_config);
  return retVal;
}


/*******************************************************************************
 * Set the deadzone of the current configuration
 *
 * @param aIn The related analog input
 * @param deadzone The deadzone to store
 * @return 'ConfigHandler_OK' in case of success
 *******************************************************************************/
ConfigHandler_Status_t configHandler_setDeadzone( Config_Analog_In_t aIn, uint8_t deadzone ) {
  ConfigHandler_Status_t retVal = ConfigHandler_Error;
  osSemaphoreWait(hsem_config, osWaitForever);

  if(IS_CURRENT_CONFIG_OF_TYPE(configType_remoteunit)) {
    configurations[sysConfig.currentSlot].remoteunit.aIn_deadzone[aIn] = deadzone;
    STORE_CONFIG_ITEM(sysConfig.currentSlot, remoteunit.aIn_deadzone[aIn]);

    configHandler_forceConfigTaskToReloadConfig();
    retVal = ConfigHandler_OK;
  }
  osSemaphoreRelease(hsem_config);
  return retVal;
}


/*******************************************************************************
 * Set the buddy button mapping of the current configuration
 *
 * @param in Digital input channel
 * @param out Digital output channel
 * @return 'ConfigHandler_OK' in case of success
 *******************************************************************************/
ConfigHandler_Status_t configHandler_setBuddyButton( Config_BuddyButton_t in, uint8_t out ) {
  ConfigHandler_Status_t retVal = ConfigHandler_Error;
  osSemaphoreWait(hsem_config, osWaitForever);

  //Check if output is not already in use
  if(IS_CURRENT_CONFIG_OF_TYPE(configType_remoteunit) &&
      (out == DIGITAL_PORT_NOT_USED ||
        (configurations[sysConfig.currentSlot].remoteunit.dOut[buddyButton1] != out &&
        configurations[sysConfig.currentSlot].remoteunit.dOut[buddyButton2] != out &&
        configurations[sysConfig.currentSlot].remoteunit.dOut[buddyButton3] != out &&
        configurations[sysConfig.currentSlot].remoteunit.dOut[buddyButton4] != out )) &&
      (out < 26 || out == DIGITAL_PORT_NOT_USED)) {

    configurations[sysConfig.currentSlot].remoteunit.dOut[in] = out;
    STORE_CONFIG_ITEM(sysConfig.currentSlot, remoteunit.dOut[in]);

    configHandler_forceConfigTaskToReloadConfig();
    retVal = ConfigHandler_OK;
  }

  osSemaphoreRelease(hsem_config);
  return retVal;
}


/*******************************************************************************
 * Set the teacher-port source
 *
 * @param dIn Digital input channel
 * @return 'ConfigHandler_OK' in case of success
 *******************************************************************************/
ConfigHandler_Status_t configHandler_setTeacherPort( uint8_t dIn ) {
  ConfigHandler_Status_t retVal = ConfigHandler_Error;
  osSemaphoreWait(hsem_config, osWaitForever);

  //Check if output is not already in use
  if(IS_CURRENT_CONFIG_OF_TYPE(configType_remoteunit) && (dIn <  26|| dIn == DIGITAL_PORT_NOT_USED)) {
    configurations[sysConfig.currentSlot].remoteunit.teacher_Port = dIn;
    STORE_CONFIG_ITEM(sysConfig.currentSlot, remoteunit.teacher_Port);

    configHandler_forceConfigTaskToReloadConfig();
    retVal = ConfigHandler_OK;
  }

  osSemaphoreRelease(hsem_config);
  return retVal;
}


/*******************************************************************************
 * Set the calibration of a analog input channel
 *
 * @param aIn The analog channel
 * @param midpoint The middle value of the channel.
 * @param margin The margin above/below the midpoint of the analog channel
 * @return 'ConfigHandler_OK' in case of success
 *******************************************************************************/
ConfigHandler_Status_t configHandler_setAnalogInCalibration( Config_Analog_In_t aIn, uint32_t midpoint, uint32_t margin, bool inverted ) {
  ConfigHandler_Status_t retVal = ConfigHandler_Error;
  osSemaphoreWait(hsem_config, osWaitForever);

  //Check if output is not already in use
  if(midpoint > 500 && midpoint < 3594 && margin > 100 && margin < 2047 && aIn <= analog_in_w) {
    sysConfig.aIn_midpoint[aIn] = midpoint;
    STORE_SYSCONFIG_ITEM(aIn_midpoint[aIn]);

    sysConfig.aIn_margin[aIn] = margin;
    STORE_SYSCONFIG_ITEM(aIn_margin[aIn]);

    sysConfig.aIn_inverted[aIn] = inverted;
    STORE_SYSCONFIG_ITEM(aIn_inverted[aIn]);

    configHandler_forceConfigTaskToReloadConfig();
    retVal = ConfigHandler_OK;
  }

  osSemaphoreRelease(hsem_config);
  return retVal;
}


/*******************************************************************************
 * Set the calibration of a analog output channel
 *
 * @param aOut The analog channel
 * @param midpoint The middle value of the channel.
 * @param margin The margin above/below the midpoint of the analog channel
 * @return 'ConfigHandler_OK' in case of success
 *******************************************************************************/
ConfigHandler_Status_t configHandler_setAnalogOutCalibration( Config_Analog_Out_t aOut, uint32_t midpoint, uint32_t margin, bool inverted ) {
  ConfigHandler_Status_t retVal = ConfigHandler_Error;
  osSemaphoreWait(hsem_config, osWaitForever);

  //Check if output is not already in use
  if(midpoint > 500 && midpoint < 3594 && margin > 100 && margin < 2047 && aOut <= analog_out_w) {
    sysConfig.aOut_midpoint[aOut] = midpoint;
    STORE_SYSCONFIG_ITEM(aOut_midpoint[aOut]);

    sysConfig.aOut_margin[aOut] = margin;
    STORE_SYSCONFIG_ITEM(aOut_margin[aOut]);

    sysConfig.aOut_inverted[aOut] = inverted;
    STORE_SYSCONFIG_ITEM(aOut_inverted[aOut]);

    configHandler_forceConfigTaskToReloadConfig();
    retVal = ConfigHandler_OK;
  }

  osSemaphoreRelease(hsem_config);
  return retVal;
}


/*******************************************************************************
 * Sets the type of the switches and its related channel
 *
 * @param id The switch id
 * @param type The type of the switch
 * @param ch1 The id of the digital output for channel 1
 * @param ch2 The id of the digital output for channel 2
 * @return 'ConfigHandler_OK' in case of success
 *******************************************************************************/
ConfigHandler_Status_t configHandler_setGlobalSwitches( uint8_t id, SysConf_Switch_t type, uint8_t ch1, uint8_t ch2 ) {
  ConfigHandler_Status_t retVal = ConfigHandler_Error;
  osSemaphoreWait(hsem_config, osWaitForever);

  //Check if output is not already in use
  if(id < 26 && (ch1 < 26 || ch1 == DIGITAL_PORT_NOT_USED) && (ch2 < 26 || ch2 == DIGITAL_PORT_NOT_USED)) {
    sysConfig.switch_types[id] = type;
    STORE_SYSCONFIG_ITEM(switch_types[id]);

    sysConfig.switch_ch1[id] = ch1;
    STORE_SYSCONFIG_ITEM(switch_types[ch1]);

    if(type == sysconf_switch_3pos) {
      sysConfig.switch_ch2[id] = ch2;
    } else {
      sysConfig.switch_ch2[id] = DIGITAL_PORT_NOT_USED;
    }
    STORE_SYSCONFIG_ITEM(switch_types[ch2]);

    configHandler_forceConfigTaskToReloadConfig();
    retVal = ConfigHandler_OK;
  }

  osSemaphoreRelease(hsem_config);
  return retVal;
}


/*******************************************************************************
 * Sets the channel of an analog axis
 *
 * @param out Analog output channel
 * @param ch The id of the analog channel
 * @return 'ConfigHandler_OK' in case of success
 *******************************************************************************/
ConfigHandler_Status_t configHandler_setGlobalAxis( Config_Analog_Out_t out, uint8_t ch ) {
  ConfigHandler_Status_t retVal = ConfigHandler_Error;
  osSemaphoreWait(hsem_config, osWaitForever);

  //Check if output is not already in use
  if(ch < 4 && ch >= 1 && out >= analog_out_x && out <= analog_out_w) {
    sysConfig.axis_channels[out] = ch;
    STORE_SYSCONFIG_ITEM(axis_channels[out]);

    configHandler_forceConfigTaskToReloadConfig();
    retVal = ConfigHandler_OK;
  }

  osSemaphoreRelease(hsem_config);
  return retVal;
}


/*******************************************************************************
 * Generates a backup of the EEPROM contents
 *
 * @param buffer The buffer where the data is stored
 * @param length The length of the buffer
 * @return 'ConfigHandler_OK' in case of success
 *******************************************************************************/
ConfigHandler_Status_t configHandler_generateBackup( uint8_t* buffer, uint32_t length ) {
  ConfigHandler_Status_t retVal = ConfigHandler_Error;
  osSemaphoreWait(hsem_config, osWaitForever);

  if(length == 33*256) {
    for(uint32_t i=0; i<33*256; i+=256) {
      eeprom_read(i, &(buffer[i]), 256);
    }
    retVal = ConfigHandler_OK;
  }

  osSemaphoreRelease(hsem_config);
  return retVal;
}


/*******************************************************************************
 * Restores a backup to the EEPROM
 *
 * @param buffer The buffer where the data is stored
 * @param length The length of the buffer
 * @return 'ConfigHandler_OK' in case of success
 *******************************************************************************/
ConfigHandler_Status_t configHandler_restoreBackup( uint8_t* buffer, uint32_t length ) {
  ConfigHandler_Status_t retVal = ConfigHandler_Error;
  osSemaphoreWait(hsem_config, osWaitForever);

  if(length == 33*256) {
    for(uint32_t i=0; i<33*256; i+=64) {
      eeprom_write(i, &(buffer[i]), 64);
      osDelay(5);
    }
    retVal = ConfigHandler_OK;
  }

  osSemaphoreRelease(hsem_config);
  return retVal;
}

