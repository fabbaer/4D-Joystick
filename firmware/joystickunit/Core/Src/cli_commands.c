/*******************************************************************************
 * @file         : cli_commands.c
 * @project      : 4D-Joystick, Joystick-Unit
 * @author       : Fabian Baer
 * @brief        : Command definition of the CLI
 ******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include <stdbool.h>
#include <system.h>
#include <cli_commands.h>
#include <configHandler.h>
#include <remoteunit.h>


/* Macros --------------------------------------------------------------------*/
#define CLI_COMMAND(cmd, func, desc) {.name=cmd, .function=func, .description=desc, .nameLen=sizeof(cmd)-1, .descLen=sizeof(desc)-1}


/* Typedefs ------------------------------------------------------------------*/
typedef struct {
    uint8_t name[15];
    uint32_t nameLen;
    uint8_t description[60];
    uint32_t descLen;
    void (*function)(CLI_Handle_t* hcli);
} CLI_Function_t;


/* Prototypes ----------------------------------------------------------------*/
static void cli_commands_notImplemented(CLI_Handle_t* hcli);
static void cli_commands_clear(CLI_Handle_t* hcli);
static void cli_commands_help(CLI_Handle_t* hcli);
static void cli_commands_new(CLI_Handle_t *hcli);
static void cli_commands_delete(CLI_Handle_t *hcli);
static void cli_commands_load(CLI_Handle_t *hcli);
static void cli_commands_move(CLI_Handle_t *hcli);
static void cli_commands_rename(CLI_Handle_t *hcli);
static void cli_commands_list(CLI_Handle_t *hcli);
static void cli_commands_copy(CLI_Handle_t *hcli);
static void cli_commands_calibrate(CLI_Handle_t *hcli);
static void cli_commands_calcCalib(uint16_t lower, uint16_t upper, uint16_t* mid, bool* inv, uint16_t* marg);
static void cli_commands_info(CLI_Handle_t *hcli);
static void cli_commands_deadzone(CLI_Handle_t *hcli);
static void cli_commands_map(CLI_Handle_t *hcli);
static void cli_commands_unmap(CLI_Handle_t *hcli);
static inline void cli_commands_mapAnalog(CLI_Handle_t *hcli);
static inline void cli_commands_mapDigital(CLI_Handle_t *hcli);
static inline void cli_commands_mapTeacher(CLI_Handle_t *hcli);
static void cli_commands_show(CLI_Handle_t *hcli);
static void cli_commands_remShow(CLI_Handle_t *hcli);
static void cli_commands_remMap(CLI_Handle_t *hcli);
static void cli_commands_backup(CLI_Handle_t *hcli);
static void cli_commands_restore(CLI_Handle_t *hcli);


/* Variables -----------------------------------------------------------------*/
CLI_Function_t commands[] = {
    CLI_COMMAND("new", cli_commands_new, "Create a new configuration an load it"),
    CLI_COMMAND("load", cli_commands_load, "Load another configuration"),
    CLI_COMMAND("delete", cli_commands_delete, "Delete a configuration"),
    CLI_COMMAND("move", cli_commands_move, "Move a configuration to another slot"),
    CLI_COMMAND("rename", cli_commands_rename, "Rename the current configuration"),
    CLI_COMMAND("list", cli_commands_list, "List all configurations"),
    CLI_COMMAND("copy", cli_commands_copy, "Copy a configuration"),
    CLI_COMMAND("show", cli_commands_show, "Display the current configuration"),
    CLI_COMMAND("calibrate", cli_commands_calibrate, "Calibration of analogue channels"),
    CLI_COMMAND("deadzone", cli_commands_deadzone, "Configure dead-zones of analogue channels"),
    CLI_COMMAND("map", cli_commands_map, "Maps two channels (analogue/digital)"),
    CLI_COMMAND("unmap", cli_commands_unmap, "Unmaps two channels (analogue/digital)"),
    CLI_COMMAND("rem_show", cli_commands_remShow, "Show the mapping of the remote-unit"),
    CLI_COMMAND("rem_map", cli_commands_remMap, "Maps a channel on the remote-unit"),
    CLI_COMMAND("backup", cli_commands_backup, "Creates a backup of all configurations"),
    CLI_COMMAND("restore", cli_commands_restore, "Restores a backup"),
    CLI_COMMAND("info", cli_commands_info, "Show the system version"),
    CLI_COMMAND("clear", cli_commands_clear, "Clears the CLI"),
    CLI_COMMAND("help", cli_commands_help, "Display all available commands"),
    //End of function list identifier, don't remove the next line.
    CLI_COMMAND("", NULL, ""),
    CLI_COMMAND("", cli_commands_notImplemented, "")
};
static uint8_t eepromBackupStorage[33*256];


/* Code ----------------------------------------------------------------------*/
void cli_commands_execute(CLI_Handle_t *hcli) {
  uint32_t cnt = 0;

  //Check if there is any command
  if (hcli->rxLength == 0) {
    return;
  }

  //Add terminating zero to buffer
  hcli->rxBuffer[hcli->rxLength] = '\0';

  //Search for command and execute it
  while (commands[cnt].function != NULL) {
    if (strcmp((char*) (hcli->rxBuffer), (char*) (commands[cnt].name)) == 0) {
      commands[cnt].function(hcli);
      return;
    }
    cnt++;
  }

  cli_putStr(hcli,
      "Unknown command! Enter \"help\" to display possible commands.");
  cli_newLine(hcli);
}

static void cli_commands_notImplemented(CLI_Handle_t *hcli) {
  cli_putStr(hcli, "This function is currently not implemented!");
  cli_newLine(hcli);
}

static void cli_commands_list(CLI_Handle_t* hcli) {
  SystemConfiguration_t* sysConfig = configHandler_getSystemConfig();
  Configuration_t* configs = configHandler_getConfigs();
  uint32_t len = 0;

  cli_putStrLn(hcli, "########################");
  for(uint32_t i = 0; i < 32; i++) {
    if(sysConfig->activeSlots & (1 << i)) {
      //Print configuration
      if(i<10) {
        cli_putStr(hcli, "#  (");
      } else {
        cli_putStr(hcli, "# (");
      }
      cli_putNum(hcli, i);
      cli_putStr(hcli, ") ");
      len = strlen((char*)configs[i].name);
      cli_print(hcli, configs[i].name, len);
      for(;len<16;len++) {
        cli_putChar(hcli, ' ');
      }
      cli_putChar(hcli, '#');
      cli_newLine(hcli);
    }
  }
  cli_putStrLn(hcli, "########################");
}

static void cli_commands_clear(CLI_Handle_t *hcli) {
  //Clear the screen with VT100 escape sequences
  cli_putStr(hcli, "\33[2J");
  cli_putStr(hcli, "\33[;H");
}

static void cli_commands_help(CLI_Handle_t *hcli) {
  uint32_t cnt = 0;
  uint32_t maxLen = 0;
  while (commands[cnt].function != NULL) {
    maxLen = (commands[cnt].nameLen > maxLen) ? commands[cnt].nameLen : maxLen;
    cnt++;
  }

  cnt = 0;
  while (commands[cnt].function != NULL) {
    cli_print(hcli, commands[cnt].name, commands[cnt].nameLen);
    for(uint32_t fill = commands[cnt].nameLen; fill<=maxLen; fill++) {
      cli_putChar(hcli, ' ');
    }
    cli_putStr(hcli, " - ");
    cli_print(hcli, commands[cnt].description, commands[cnt].descLen);
    cli_newLine(hcli);
    cnt++;
  }
}

static void cli_commands_new(CLI_Handle_t *hcli) {
  CLI_InputState_t retval;
  uint8_t name[15];
  uint32_t nameLen = 15;
  uint32_t slot;

  //Ask for name and check
  cli_putStrLn(hcli, "Please enter modelname and press enter:");
  retval = cli_getInput(hcli, name, &nameLen);
  switch(retval) {
    case cli_input_OK:
      break;
    case cli_input_empty:
      cli_putStrLn(hcli, "Error: Nothing entered!");
      cli_printAbort(hcli);
      return;
    default:
      return;
  }

  //Ask for slot
  cli_putStr(hcli, "Please enter slot number (enter for slot ");
  cli_putNum(hcli, configHandler_getFirstFreeSlot());
  cli_putStrLn(hcli, "):");
  retval = cli_getNum(hcli, &slot);
  switch(retval) {
    case cli_input_OK:
      break;
    case cli_input_empty:
      slot = configHandler_getFirstFreeSlot();
      break;
    default:
      return;
  }

  //Add config
  if(slot < 32) {
    if(configHandler_newConfig(configType_remoteunit, slot, name, nameLen) == ConfigHandler_OK) {
      cli_printSucess(hcli);
      return;
    }
  }

  cli_putStrLn(hcli, "Error: Slot is not available!");
  cli_printAbort(hcli);
  return;
}

static void cli_commands_delete(CLI_Handle_t *hcli) {
  CLI_InputState_t retval;
  uint32_t slot;

  //Print active slots
  cli_commands_list(hcli);

  //Ask for slot
  cli_putStrLn(hcli, "Please enter slot number (enter for current slot):");
  retval = cli_getNum(hcli, &slot);
  switch(retval) {
    case cli_input_OK:
      break;
    case cli_input_empty:
      slot = configHandler_getSystemConfig()->currentSlot;
      break;
    default:
      return;
  }

  //Add config
  if(slot < 32) {
    if(configHandler_deleteConfig(slot) == ConfigHandler_OK) {
      cli_printSucess(hcli);
      return;
    }
  }

  cli_putStrLn(hcli, "Error: Slot is not available!");
  cli_printAbort(hcli);
  return;
}

static void cli_commands_load(CLI_Handle_t *hcli) {
  CLI_InputState_t retval;
  uint32_t slot;

  //Print active slots
  cli_commands_list(hcli);

  //Ask for slot
  cli_putStrLn(hcli, "Please enter slot number:");
  retval = cli_getNum(hcli, &slot);
  switch(retval) {
    case cli_input_OK:
      break;
    case cli_input_empty:
      cli_putStrLn(hcli, "Error: Nothing entered!");
      cli_printAbort(hcli);
      return;
    default:
      return;
  }

  //Add config
  if(slot < 32) {
    if(configHandler_loadConfig(slot) == ConfigHandler_OK) {
      cli_printSucess(hcli);
      return;
    }
  }

  cli_putStrLn(hcli, "Error: Slot is not available!");
  cli_printAbort(hcli);
  return;
}

static void cli_commands_copy(CLI_Handle_t *hcli) {
  CLI_InputState_t retval;
  uint32_t source, destination;

  //Print active slots
  cli_commands_list(hcli);

  //Ask for slot
  cli_putStrLn(hcli, "Please enter source slot number:");
  retval = cli_getNum(hcli, &source);
  switch(retval) {
    case cli_input_OK:
      break;
    case cli_input_empty:
      cli_putStrLn(hcli, "Error: Nothing entered!");
      cli_printAbort(hcli);
      return;
    default:
      return;
  }

  cli_putStrLn(hcli, "Please enter destination slot number:");
  retval = cli_getNum(hcli, &destination);
  switch(retval) {
    case cli_input_OK:
      break;
    case cli_input_empty:
      cli_putStrLn(hcli, "Error: Nothing entered!");
      cli_printAbort(hcli);
      return;
    default:
      return;
  }

  //Add config
  if(source < 3 && destination < 32) {
    if(configHandler_copyConfig(source, destination) == ConfigHandler_OK) {
      cli_printSucess(hcli);
      return;
    }
  }

  cli_putStrLn(hcli, "Error: Slot is not available!");
  cli_printAbort(hcli);
  return;
}

static void cli_commands_move(CLI_Handle_t *hcli) {
  CLI_InputState_t retval;
  uint32_t source, destination;

  //Print active slots
  cli_commands_list(hcli);

  //Ask for slot
  cli_putStrLn(hcli, "Please enter source slot number:");
  retval = cli_getNum(hcli, &source);
  switch(retval) {
    case cli_input_OK:
      break;
    case cli_input_empty:
      cli_putStrLn(hcli, "Error: Nothing entered!");
      cli_printAbort(hcli);
      return;
    default:
      return;
  }

  cli_putStrLn(hcli, "Please enter destination slot number:");
  retval = cli_getNum(hcli, &destination);
  switch(retval) {
    case cli_input_OK:
      break;
    case cli_input_empty:
      cli_putStrLn(hcli, "Error: Nothing entered!");
      cli_printAbort(hcli);
      return;
    default:
      return;
  }

  //Add config
  if(source < 32 && destination < 32) {
    if(configHandler_moveConfig(source, destination) == ConfigHandler_OK) {
      cli_printSucess(hcli);
      return;
    }
  }

  cli_putStrLn(hcli, "Error: Slot is not available!");
  cli_printAbort(hcli);
  return;
}

static void cli_commands_rename(CLI_Handle_t *hcli) {
  CLI_InputState_t retval;
  uint8_t name[15];
  uint32_t nameLen = 15;

  //Ask for name and check
  cli_putStrLn(hcli, "Please enter modelname and press enter:");
  retval = cli_getInput(hcli, name, &nameLen);
  switch(retval) {
    case cli_input_OK:
      break;
    case cli_input_empty:
      cli_putStrLn(hcli, "Error: Nothing entered!");
      cli_printAbort(hcli);
      return;
    default:
      return;
  }

  if(configHandler_renameConfig(name, nameLen) == ConfigHandler_OK) {
    cli_printSucess(hcli);
    return;
  }

}

static void cli_commands_calibrate(CLI_Handle_t *hcli) {
  RemoteUnit_adcStates_t adcStates;
  uint16_t lower1, mid1, upper1, lower2, mid2, upper2, marg;
  bool inverted;

  //Remote-Unit, left joystick
  cli_putStrLn(hcli, "Move left joystick on remote-unit to lower-left corner and press enter");
  if(cli_waitForEnter(hcli) != cli_input_OK) {
    cli_printAbort(hcli);
    return;
  }
  adcStates = remoteunit_getADC();
  lower1 = adcStates.rem_x;
  lower2 = adcStates.rem_y;

  cli_putStrLn(hcli, "Move left joystick on remote-unit to midpoint and press enter");
  if(cli_waitForEnter(hcli) != cli_input_OK) {
    cli_printAbort(hcli);
    return;
  }
  adcStates = remoteunit_getADC();
  mid1 = adcStates.rem_x;
  mid2 = adcStates.rem_y;

  cli_putStrLn(hcli, "Move left joystick on remote-unit to upper-right corner and press enter");
  if(cli_waitForEnter(hcli) != cli_input_OK) {
    cli_printAbort(hcli);
    return;
  }
  adcStates = remoteunit_getADC();
  upper1 = adcStates.rem_x;
  upper2 = adcStates.rem_y;

  cli_commands_calcCalib(lower1, upper1, &mid1, &inverted, &marg);
  if(configHandler_setAnalogOutCalibration(analog_out_x, mid1, marg, inverted) != ConfigHandler_OK) {
    cli_putStrLn(hcli, "Error. Abort!");
    return;
  }

  cli_commands_calcCalib(lower2, upper2, &mid2, &inverted, &marg);
  if(configHandler_setAnalogOutCalibration(analog_out_y, mid2, marg, inverted) != ConfigHandler_OK) {
    cli_putStrLn(hcli, "Error. Abort!");
    return;
  }



  //Remote-Unit, right joystick
  cli_putStrLn(hcli, "Move right joystick on remote-unit to lower-left corner and press enter");
  if(cli_waitForEnter(hcli) != cli_input_OK) {
    cli_printAbort(hcli);
    return;
  }
  adcStates = remoteunit_getADC();
  lower1 = adcStates.rem_z;
  lower2 = adcStates.rem_w;

  cli_putStrLn(hcli, "Move right joystick on remote-unit to midpoint and press enter");
  if(cli_waitForEnter(hcli) != cli_input_OK) {
    cli_printAbort(hcli);
    return;
  }
  adcStates = remoteunit_getADC();
  mid1 = adcStates.rem_z;
  mid2 = adcStates.rem_w;

  cli_putStrLn(hcli, "Move right joystick on remote-unit to upper-right corner and press enter");
  if(cli_waitForEnter(hcli) != cli_input_OK) {
    cli_printAbort(hcli);
    return;
  }
  adcStates = remoteunit_getADC();
  upper1 = adcStates.rem_z;
  upper2 = adcStates.rem_w;

  cli_commands_calcCalib(lower1, upper1, &mid1, &inverted, &marg);
  if(configHandler_setAnalogOutCalibration(analog_out_z, mid1, marg, inverted) != ConfigHandler_OK) {
    cli_putStrLn(hcli, "Error. Abort!");
    return;
  }

  cli_commands_calcCalib(lower2, upper2, &mid2, &inverted, &marg);
  if(configHandler_setAnalogOutCalibration(analog_out_w, mid2, marg, inverted) != ConfigHandler_OK) {
    cli_putStrLn(hcli, "Error. Abort!");
    return;
  }



  //Joystick-Unit, main joystick
  cli_putStrLn(hcli, "Move joystick on joystick-unit to lower-left corner and press enter");
  if(cli_waitForEnter(hcli) != cli_input_OK) {
    cli_printAbort(hcli);
    return;
  }
  adcStates = remoteunit_getADC();
  lower1 = adcStates.joy_x;
  lower2 = adcStates.joy_y;

  cli_putStrLn(hcli, "Move joystick on joystick-unit to midpoint and press enter");
  if(cli_waitForEnter(hcli) != cli_input_OK) {
    cli_printAbort(hcli);
    return;
  }
  adcStates = remoteunit_getADC();
  mid1 = adcStates.joy_x;
  mid2 = adcStates.joy_y;

  cli_putStrLn(hcli, "Move joystick on joystick-unit to upper-right corner and press enter");
  if(cli_waitForEnter(hcli) != cli_input_OK) {
    cli_printAbort(hcli);
    return;
  }
  adcStates = remoteunit_getADC();
  upper1 = adcStates.joy_x;
  upper2 = adcStates.joy_y;

  cli_commands_calcCalib(lower1, upper1, &mid1, &inverted, &marg);
  if(configHandler_setAnalogInCalibration(analog_in_x, mid1, marg, inverted) != ConfigHandler_OK) {
    cli_putStrLn(hcli, "Error. Abort!");
    return;
  }

  cli_commands_calcCalib(lower2, upper2, &mid2, &inverted, &marg);
  if(configHandler_setAnalogInCalibration(analog_in_y, mid2, marg, inverted) != ConfigHandler_OK) {
    cli_putStrLn(hcli, "Error. Abort!");
    return;
  }


  //Joystick-Unit, z-axis
  cli_putStrLn(hcli, "Bring 4d-joystick to back position and press enter");
  if(cli_waitForEnter(hcli) != cli_input_OK) {
    cli_printAbort(hcli);
    return;
  }
  adcStates = remoteunit_getADC();
  upper1 = adcStates.joy_z;

  cli_putStrLn(hcli, "Bring 4d-joystick to front position and press enter");
  if(cli_waitForEnter(hcli) != cli_input_OK) {
    cli_printAbort(hcli);
    return;
  }
  adcStates = remoteunit_getADC();
  lower1 = adcStates.joy_z;

  if(upper1 > lower1) {
    mid1 = (upper1 - lower1) / 2;
  } else {
    mid1 = (lower1 - upper1) / 2;
  }

  cli_commands_calcCalib(lower1, upper1, &mid1, &inverted, &marg);
  if(configHandler_setAnalogInCalibration(analog_in_z, mid1, marg, inverted) != ConfigHandler_OK) {
    cli_putStrLn(hcli, "Error. Abort!");
    return;
  }


  //Joystick-Unit, pressur sensor
  cli_putStrLn(hcli, "Don't apply any pressure to the mouthpiece and press enter");
  if(cli_waitForEnter(hcli) != cli_input_OK) {
    cli_printAbort(hcli);
    return;
  }
  adcStates = remoteunit_getADC();
  mid1 = adcStates.joy_w;

  cli_putStrLn(hcli, "Apply maximum pressure to the mouthpiece and press enter");
  if(cli_waitForEnter(hcli) != cli_input_OK) {
    cli_printAbort(hcli);
    return;
  }
  adcStates = remoteunit_getADC();
  upper1 = adcStates.joy_w;

  cli_putStrLn(hcli, "Apply minimum pressure to the mouthpiece and press enter");
  if(cli_waitForEnter(hcli) != cli_input_OK) {
    cli_printAbort(hcli);
    return;
  }
  adcStates = remoteunit_getADC();
  lower1 = adcStates.joy_w;

  cli_commands_calcCalib(lower1, upper1, &mid1, &inverted, &marg);
  if(configHandler_setAnalogInCalibration(analog_in_w, mid1, marg, inverted) != ConfigHandler_OK) {
    cli_putStrLn(hcli, "Error. Abort!");
    return;
  }
}

static void cli_commands_calcCalib(uint16_t lower, uint16_t upper, uint16_t* mid, bool* inv, uint16_t* marg) {
  uint16_t marg1, marg2, dMid;

  *inv = (lower > upper) ? true : false;
  if(*inv) {
    dMid = *mid * 2;
    upper = dMid - upper;
    lower = dMid - lower;
  }

  marg1 = upper - *mid;
  marg2 = *mid - lower;

  *marg = (marg1 < marg2) ? marg2 : marg1;
}


static void cli_commands_info(CLI_Handle_t *hcli) {
  cli_putStrLn(hcli, "4D-Joystick, Joystick-Unit");
  cli_putStrLn(hcli, "Firmware "FW_VERSION_STRING);
}


static void cli_commands_deadzone(CLI_Handle_t *hcli) {
  CLI_InputState_t retval;
  uint8_t buf;
  uint32_t len = 1;
  uint32_t deadzone;
  Config_Analog_In_t aIn;

  //Ask for analog channel
  cli_putStrLn(hcli, "Please select one of the following input channel:");
  cli_putStrLn(hcli, "x, y, z, w");
  retval = cli_getInput(hcli, &buf, &len);

  switch(retval) {
    case cli_input_OK:
      switch(buf) {
        case 'x':
          aIn = analog_in_x;
          break;
        case 'y':
          aIn = analog_in_y;
          break;
        case 'z':
          aIn = analog_in_z;
          break;
        case 'w':
          aIn = analog_in_w;
          break;
        default:
          cli_putStrLn(hcli, "Error: Invalid channel!");
          cli_printAbort(hcli);
          return;
      }
      break;
    case cli_input_empty:
      cli_putStrLn(hcli, "Error: Nothing entered!");
      cli_printAbort(hcli);
      return;
    default:
      return;
  }

  //Ask for deadzone
  cli_putStrLn(hcli, "Enter deadzone with (0-255):");
  retval = cli_getNum(hcli, &deadzone);
  switch(retval) {
    case cli_input_OK:
      if(deadzone > 255) {
        cli_putStrLn(hcli, "Error: Invalid value!");
        cli_printAbort(hcli);
        return;
      }
      break;
    case cli_input_empty:
      cli_putStrLn(hcli, "Error: Nothing entered!");
      cli_printAbort(hcli);
      return;
    default:
      return;
  }

  //Change config
  if(configHandler_setDeadzone(aIn, deadzone) == ConfigHandler_OK) {
    cli_printSucess(hcli);
    return;
  }

  cli_putStrLn(hcli, "Error!");
  cli_printAbort(hcli);
  return;
}

static void cli_commands_map(CLI_Handle_t *hcli) {
  CLI_InputState_t retval;
  uint8_t buf;
  uint32_t len = 1;

  cli_putStrLn(hcli, "Map analog/digital channel or teacher port? (a/d/t)");
  retval = cli_getInput(hcli, &buf, &len);

  switch(retval) {
    case cli_input_OK:
      switch(buf) {
        case 'a':
          cli_commands_mapAnalog(hcli);
          break;
        case 'd':
          cli_commands_mapDigital(hcli);
          break;
        case 't':
          cli_commands_mapTeacher(hcli);
          break;
        default:
          cli_putStrLn(hcli, "Error: Invalid input!");
          cli_printAbort(hcli);
          return;
      }
      break;
    case cli_input_empty:
      cli_putStrLn(hcli, "Error: Nothing entered!");
      cli_printAbort(hcli);
      return;
    default:
      return;
  }
}

static inline void cli_commands_mapAnalog(CLI_Handle_t *hcli) {
  CLI_InputState_t retval;
  Config_Analog_In_t aIn;
  Config_Analog_Out_t aOut;
  uint8_t buf[2];
  uint32_t len = 2;

  //Ask for input channel
  cli_putStrLn(hcli, "Select one of the following input channels:");
  cli_putStrLn(hcli, "x, y, z, w, rx, ry, rz, rw");
  retval = cli_getInput(hcli, buf, &len);

  //Check input
  switch(retval) {
    case cli_input_OK:
      break;
    case cli_input_empty:
      cli_putStrLn(hcli, "Error: Nothing entered!");
      cli_printAbort(hcli);
      return;
    default:
      return;
  }

  if(len == 2 && buf[0] == 'r') {
    switch(buf[1]) {
      case 'x':
        aIn = analog_in_rx;
        break;
      case 'y':
        aIn = analog_in_ry;
        break;
      case 'z':
        aIn = analog_in_rz;
        break;
      case 'w':
        aIn = analog_in_rw;
        break;
      default:
        cli_putStrLn(hcli, "Error: Invalid input!");
        cli_printAbort(hcli);
        return;
    }
  } else if(len == 1) {
    switch(buf[0]) {
      case 'x':
        aIn = analog_in_x;
        break;
      case 'y':
        aIn = analog_in_y;
        break;
      case 'z':
        aIn = analog_in_z;
        break;
      case 'w':
        aIn = analog_in_w;
        break;
      default:
        cli_putStrLn(hcli, "Error: Invalid input!");
        cli_printAbort(hcli);
        return;
    }
  }

  //Ask for output channel
  cli_putStrLn(hcli, "Select one of the following output channels:");
  cli_putStrLn(hcli, "x, y, z, w");
  len = 1;
  retval = cli_getInput(hcli, buf, &len);

  //Check input
  switch(retval) {
    case cli_input_OK:
      break;
    case cli_input_empty:
      cli_putStrLn(hcli, "Error: Nothing entered!");
      cli_printAbort(hcli);
      return;
    default:
      return;
  }

  switch(buf[0]) {
    case 'x':
      aOut = analog_out_x;
      break;
    case 'y':
      aOut = analog_out_y;
      break;
    case 'z':
      aOut = analog_out_z;
      break;
    case 'w':
      aOut = analog_out_w;
      break;
    default:
      cli_putStrLn(hcli, "Error: Invalid input!");
      cli_printAbort(hcli);
      return;
  }

  //Change config
  if(configHandler_setAxis(aIn, aOut) == ConfigHandler_OK) {
    cli_printSucess(hcli);
    return;
  }

  cli_putStrLn(hcli, "Error!");
  cli_printAbort(hcli);
  return;
}

static inline void cli_commands_mapDigital(CLI_Handle_t *hcli) {
  CLI_InputState_t retval;
  uint8_t buf[2];
  uint32_t len = 2;
  Config_BuddyButton_t in;
  uint32_t out;

  //Ask for input channel
  cli_putStrLn(hcli, "Select one of the following input channels:");
  cli_putStrLn(hcli, "b1, b2, b3, b4");
  retval = cli_getInput(hcli, buf, &len);

  //Check input
  switch(retval) {
    case cli_input_OK:
      break;
    case cli_input_empty:
      cli_putStrLn(hcli, "Error: Nothing entered!");
      cli_printAbort(hcli);
      return;
    default:
      return;
  }

  if(len == 2 && buf[0] == 'b' && buf[1] >= '1' && buf[1] <= '4') {
    switch(buf[1]) {
      case '1':
        in = buddyButton1;
        break;
      case '2':
        in = buddyButton2;
        break;
      case '3':
        in = buddyButton3;
        break;
      case '4':
        in = buddyButton4;
        break;
      default:
        break;
    }
  } else {
    cli_putStrLn(hcli, "Error: Invalid input!");
    cli_printAbort(hcli);
    return;
  }

  //Ask for output channel
  cli_putStrLn(hcli, "Select one of the following output channels:");
  cli_putStrLn(hcli, "sa, sb, sc, ..., sy, sz");
  len = 2;
  retval = cli_getInput(hcli, buf, &len);

  //Check input
  switch(retval) {
    case cli_input_OK:
      break;
    case cli_input_empty:
      cli_putStrLn(hcli, "Error: Nothing entered!");
      cli_printAbort(hcli);
      return;
    default:
      return;
  }

  if(len == 2 && buf[0] == 's' && buf[1] >= 'a' && buf[1] <= 'z') {
    out = buf[1] - 'a';
  } else {
    cli_putStrLn(hcli, "Error: Invalid input!");
    cli_printAbort(hcli);
    return;
  }

  //Change config
  if(configHandler_setBuddyButton(in, out) == ConfigHandler_OK) {
    cli_printSucess(hcli);
    return;
  }

  cli_putStrLn(hcli, "Error!");
  cli_printAbort(hcli);
  return;
}

static inline void cli_commands_mapTeacher(CLI_Handle_t *hcli) {
  CLI_InputState_t retval;
  uint8_t buf[2];
  uint32_t len = 2;
  uint32_t in;

  //Ask for input channel
  cli_putStrLn(hcli, "Select one of the following input channels:");
  cli_putStrLn(hcli, "sa, sb, sc, ..., sy, sz");
  retval = cli_getInput(hcli, buf, &len);

  //Check input
  switch(retval) {
    case cli_input_OK:
      break;
    case cli_input_empty:
      cli_putStrLn(hcli, "Error: Nothing entered!");
      cli_printAbort(hcli);
      return;
    default:
      return;
  }

  if(len == 2 && buf[0] == 's' && buf[1] >= 'a' && buf[1] <= 'z') {
    in = buf[1] - 'a';
  } else {
    cli_putStrLn(hcli, "Error: Invalid input!");
    cli_printAbort(hcli);
    return;
  }

  //Change config
  if(configHandler_setTeacherPort(in) == ConfigHandler_OK) {
    cli_printSucess(hcli);
    return;
  }

  cli_putStrLn(hcli, "Error!");
  cli_printAbort(hcli);
  return;
}


static void cli_commands_show(CLI_Handle_t *hcli) {
  Configuration_t* config = configHandler_getCurrentConfig();

  //Analog channels
  for(Config_Analog_Out_t out = analog_out_x; out <= analog_out_w; out++) {
    cli_putChar(hcli, '(');
    cli_putNum(hcli, out+1);
    cli_putStr(hcli, ") ");
    switch(config->remoteunit.aOut[out]) {
      case analog_in_rx:
        cli_putStr(hcli, "rx");
        break;
      case analog_in_x:
        cli_putStr(hcli, "x ");
        break;
      case analog_in_ry:
        cli_putStr(hcli, "ry");
        break;
      case analog_in_y:
        cli_putStr(hcli, "y ");
        break;
      case analog_in_rz:
        cli_putStr(hcli, "rz");
        break;
      case analog_in_z:
        cli_putStr(hcli, "z ");
        break;
      case analog_in_rw:
        cli_putStr(hcli, "rw");
        break;
      case analog_in_w:
        cli_putStr(hcli, "w ");
        break;
      case analog_in_none:
      default:
        switch(out) {
          case analog_out_x:
            cli_putStr(hcli, "rx");
            break;
          case analog_out_y:
            cli_putStr(hcli, "ry");
            break;
          case analog_out_z:
            cli_putStr(hcli, "rz");
            break;
          case analog_out_w:
            cli_putStr(hcli, "rw");
            break;
          default:
            break;
        }
        break;
    }
    cli_putStr(hcli, " --> ");
    switch(out) {
      case analog_out_x:
        cli_putStr(hcli, "x");
        break;
      case analog_out_y:
        cli_putStr(hcli, "y");
        break;
      case analog_out_z:
        cli_putStr(hcli, "z");
        break;
      case analog_out_w:
        cli_putStr(hcli, "w");
        break;
      default:
        break;
    }

    if(config->remoteunit.aOut[out] >= analog_in_x && config->remoteunit.aOut[out] <= analog_in_w) {
      cli_putStr(hcli, " [deadzone: ");
      cli_putNum(hcli, config->remoteunit.aIn_deadzone[config->remoteunit.aOut[out]]);
      cli_putChar(hcli, ']');
    }

    cli_newLine(hcli);
  }

  //Buddybuttons
  for(Config_BuddyButton_t btn = buddyButton1; btn <= buddyButton4; btn++) {
    cli_putChar(hcli, '(');
    cli_putNum(hcli, btn+5);
    cli_putStr(hcli, ") b");
    cli_putNum(hcli, btn+1);
    cli_putStr(hcli, " --> ");
    if(config->remoteunit.dOut[btn] != DIGITAL_PORT_NOT_USED) {
      cli_putChar(hcli, 's');
      cli_putChar(hcli, config->remoteunit.dOut[btn] + 'a');
      cli_newLine(hcli);
    } else {
      cli_putStrLn(hcli, "not used");
    }
  }

  //Teacher
  cli_putStr(hcli, "(9) teacher --> ");
  if(config->remoteunit.teacher_Port == DIGITAL_PORT_NOT_USED) {
    cli_putStrLn(hcli, "not used");
  } else {
    cli_putChar(hcli, 's');
    cli_putChar(hcli, config->remoteunit.teacher_Port+'a');
    cli_newLine(hcli);
  }
}

static void cli_commands_unmap(CLI_Handle_t *hcli) {
  CLI_InputState_t retval;
  uint32_t channel;

  //Show list of connections
  cli_commands_show(hcli);
  cli_newLine(hcli);

  //Ask the user for the channel to unmap
  cli_putStrLn(hcli, "Select one of the channels to unmap (1-9):");
  retval = cli_getNum(hcli, &channel);
  switch(retval) {
    case cli_input_OK:
      break;
    case cli_input_empty:
      cli_putStrLn(hcli, "Error: Nothing entered!");
      cli_printAbort(hcli);
      return;
    default:
      return;
  }

  //Check input and unmap
  if(channel > 0 && channel < 5) {
    if(configHandler_setAxis(analog_in_none, channel-1+analog_out_x) == ConfigHandler_OK) {
      cli_printSucess(hcli);
      return;
    }
  } else if(channel > 4 && channel < 9) {
    if(configHandler_setBuddyButton(buddyButton1+channel-5, DIGITAL_PORT_NOT_USED) == ConfigHandler_OK) {
      cli_printSucess(hcli);
      return;
    }
  } else if(channel == 9) {
    if(configHandler_setTeacherPort(DIGITAL_PORT_NOT_USED) == ConfigHandler_OK) {
      cli_printSucess(hcli);
      return;
    }
  } else {
    cli_putStrLn(hcli, "Error: Invalid channel!");
    cli_printAbort(hcli);
    return;
  }

  cli_putStrLn(hcli, "Error!");
  cli_printAbort(hcli);
  return;
}

static inline void cli_commands_remShow(CLI_Handle_t *hcli) {
  SystemConfiguration_t* sysConfig = configHandler_getSystemConfig();

  //Show analog channels
  for(Config_Analog_Out_t i = analog_out_x; i<=analog_out_w; i++) {
    switch(i) {
      case analog_out_x:
        cli_putChar(hcli, 'x');
        break;
      case analog_out_y:
        cli_putChar(hcli, 'y');
        break;
      case analog_out_z:
        cli_putChar(hcli, 'z');
        break;
      case analog_out_w:
        cli_putChar(hcli, 'w');
        break;
    }
    cli_putStr(hcli, " -> A");
    cli_putNum(hcli, sysConfig->axis_channels[i]);
    cli_newLine(hcli);
  }

  //Show digital channels
  for(uint32_t i = 0; i<26; i++) {
    cli_putChar(hcli, 's');
    cli_putChar(hcli, i+'a');
    cli_putStr(hcli, " -> ");

    switch(sysConfig->switch_types[i]) {
      case sysconf_switch_none:
        cli_putStrLn(hcli, "no switch");
        break;
      case sysconf_switch_2pos:
        cli_putStrLn(hcli, "switch, 2 positions");
        break;
      case sysconf_switch_3pos:
        cli_putStrLn(hcli, "switch, 3 positions");
        break;
      case sysconf_momentary_2pos:
        cli_putStrLn(hcli, "momentary switch, 2 positions");
        break;
      default:
        cli_putStrLn(hcli, "error");
        break;
    }

    switch(sysConfig->switch_types[i]) {
      case sysconf_momentary_2pos:
      case sysconf_switch_2pos:
        cli_putStr(hcli, "      ch1: ");
        cli_putNum(hcli, sysConfig->switch_ch1[i]);
        cli_newLine(hcli);
        break;
      case sysconf_switch_3pos:
        cli_putStr(hcli, "      ch1: ");
        cli_putNum(hcli, sysConfig->switch_ch1[i]);
        cli_putStr(hcli, ", ch2: ");
        cli_putNum(hcli, sysConfig->switch_ch2[i]);
        cli_newLine(hcli);
        break;
      case sysconf_switch_none:
      default:
        break;
    }
  }
}


static void cli_commands_remMap(CLI_Handle_t *hcli) {
  CLI_InputState_t retval;
  uint8_t buf[2];
  uint32_t len = 2;
  uint32_t type;
  uint32_t ch1 = 0;
  uint32_t ch2 = 0;
  Config_Analog_Out_t aOut;

  //Ask for channel to map
  cli_putStrLn(hcli, "Select one of the following channels to map:");
  cli_putStrLn(hcli, "x, y, z, w, sa, sb, sc, ..., sy, sz");
  retval = cli_getInput(hcli, buf, &len);

  switch(retval) {
    case cli_input_OK:
      break;
    case cli_input_empty:
      cli_putStrLn(hcli, "Error: Nothing entered!");
      cli_printAbort(hcli);
      return;
    default:
      return;
  }

  //Check channel
  if(len == 1) {
    //Analog channel
    switch(buf[0]) {
      case 'x':
        aOut = analog_out_x;
        break;
      case 'y':
        aOut = analog_out_y;
        break;
      case 'z':
        aOut = analog_out_z;
        break;
      case 'w':
        aOut = analog_out_w;
        break;
      default:
        cli_putStrLn(hcli, "Error: Invalid channel!");
        cli_printAbort(hcli);
        return;
    }

    //Ask for channel
    cli_putStrLn(hcli, "Select channel (0-3):");

    retval = cli_getNum(hcli, &ch1);
    switch(retval) {
      case cli_input_OK:
        if(ch1 > 3) {
          cli_putStrLn(hcli, "Error: Invalid channel!");
          cli_printAbort(hcli);
          return;
        }
        break;
      case cli_input_empty:
        cli_putStrLn(hcli, "Error: Nothing entered!");
        cli_printAbort(hcli);
        return;
      default:
        return;
    }

    if(configHandler_setGlobalAxis(aOut, ch1) == ConfigHandler_OK) {
      cli_printSucess(hcli);
      return;
    }

    cli_putStrLn(hcli, "Error!");
    cli_printAbort(hcli);

  } else if (len == 2 && buf[0] == 's' && buf[1]>='a' && buf[1]<='z') {
    //Digital channel

    //Ask for switch-type
    cli_putStrLn(hcli, "Select type of switch (1-5):");
    cli_putStrLn(hcli, "(1) no switch");
    cli_putStrLn(hcli, "(2) switch, 2 positions");
    cli_putStrLn(hcli, "(3) switch, 3 positions");
    cli_putStrLn(hcli, "(4) momentary switch, 2 positions");

    retval = cli_getNum(hcli, &type);
    switch(retval) {
      case cli_input_OK:
        if(type < 1 || type > 4) {
          cli_putStrLn(hcli, "Error: Invalid channel!");
          cli_printAbort(hcli);
          return;
        }
        break;
      case cli_input_empty:
        cli_putStrLn(hcli, "Error: Nothing entered!");
        cli_printAbort(hcli);
        return;
      default:
        return;
    }

    if(type > 1) {
      //Ask for ch1
      cli_putStrLn(hcli, "Select ch1 (0-23):");

      retval = cli_getNum(hcli, &ch1);
      switch(retval) {
        case cli_input_OK:
          if(ch1 > 23) {
            cli_putStrLn(hcli, "Error: Invalid channel!");
            cli_printAbort(hcli);
            return;
          }
          break;
        case cli_input_empty:
          cli_putStrLn(hcli, "Error: Nothing entered!");
          cli_printAbort(hcli);
          return;
        default:
          return;
      }
    }

    if(type == 3) {
      //Ask for ch2
      cli_putStrLn(hcli, "Select ch2 (0-23):");

      retval = cli_getNum(hcli, &ch2);
      switch(retval) {
        case cli_input_OK:
          if(ch2 > 23) {
            cli_putStrLn(hcli, "Error: Invalid channel!");
            cli_printAbort(hcli);
            return;
          }
          break;
        case cli_input_empty:
          cli_putStrLn(hcli, "Error: Nothing entered!");
          cli_printAbort(hcli);
          return;
        default:
          return;
      }
    }

    if(configHandler_setGlobalSwitches(buf[1]-'a', type-1, ch1, ch2) == ConfigHandler_OK) {
      cli_printSucess(hcli);
      return;
    }

    cli_putStrLn(hcli, "Error!");
    cli_printAbort(hcli);

  } else {
    cli_putStrLn(hcli, "Error: Invalid channel!");
    cli_printAbort(hcli);
  }
}

static void cli_commands_backup(CLI_Handle_t *hcli) {
  if(configHandler_generateBackup(eepromBackupStorage, 33*256) == ConfigHandler_OK) {
    for(uint32_t i = 0; i<33*256; i += 64) {
      cli_print(hcli, &(eepromBackupStorage[i]), 64);
      osDelay(4);
      cli_putChar(hcli, '\r');
    }

    cli_putChar(hcli, '\x06');  //ACK
    return;
  }

  cli_putStrLn(hcli, "Error!");
  cli_printAbort(hcli);
}

static void cli_commands_restore(CLI_Handle_t *hcli) {
  cli_putChar(hcli, '\x06');  //ACK

  //Receive data
  for(uint32_t j = 0; j<33*256; j += 64) {
    //Setup transmission
    hcli->flags = CLI_FLAG_RX_RAW;
    hcli->rxLength = 0;
    hcli->rxCursor = 0;

    while (1) {
      if (hcli->flags & CLI_FLAG_RX_ABORT) {
        hcli->flags = 0;
        cli_newLine(hcli);
        cli_printAbort(hcli);
        return;
      }

      if (hcli->flags & CLI_FLAG_RX_RETURN) {
        if(hcli->rxLength == 64) {
          for(uint32_t i = 0; i<64; i++) {
            eepromBackupStorage[j+i] = hcli->rxBuffer[i];
          }
          cli_putChar(hcli, '\x06');  //ACK
          break;
        } else {
          cli_putChar(hcli, '\x15');  //NACK
          return;
        }
      }

      osDelay(1);
    }
  }
  hcli->flags = 0;

  //Store data
  configHandler_restoreBackup(eepromBackupStorage, 256*33);

  NVIC_SystemReset();
}
