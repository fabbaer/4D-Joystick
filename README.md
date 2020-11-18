# 4D-Joystick

## Introduction
The 4D-Joystick allows people with physical disabilities to control complex RC-toys, like plains or multicopper. The RC-toys are controlled via a mouthpiece, a pressure sensor and movements of the head.

The system is divided into two units. The joystickunit is the user interface. It records the user inputs, processes them and forwards them to the remoteunit. The remoteunit is responsible for the communication with the RC-toy.

### Joystickunit
The joystickunit is the main part of the system and handles all user interactions. The heart of the system is a self-developed hardware with a Cortex-M4-Controller. The firmware offers a command-line-interface (CLI) via a USB connection to configure the system. Up to 32 different configurations can be stored on the joystickunit, each offering a flexible mapping of all inputs and outputs and configurations like deadzones. 

The joystickunit can either be powered via USB or via the remoteunit. All peripherals are placed inside a 3D-printed enclosure, which can be mounted on a stand. On the outside of the housing a display and a rotary button is placed, which can be used to load different configurations and display system information. Additionally, there are four headphone jacks to connect external buttons, which can be used to control the digital channels of the remote control. The buttons can be chosen to fit the user's needs and can for example be pressed with a foot or finger. 

The analogue channels are controlled via a mouthpiece. Three of the four degrees of freedom are controlled via movements of the head in up, down, left, right, forward and backward direction. The fourth degree of freedom is controlled via a pressure sensor, which is connected to the mouthpiece. 

The firmware of the joystickunit is designed to be future-proof. In future applications, the joystickunit can for example be used as an input device for a computer or a game console. However, none of these applications are currently implemented, but many parts of the firmware are designed that this can be easily done in future. 


### Remoteunit
The remoteunit handles the communication with the RC-toy. A conventional RC-toy-remote (FrSky Taranis X9D Plus v2019) with OpenTX as main firmware is used as a basis for this unit.

The joystickunit needs to gain control over the input-devices of the remote-control (joysticks and buttons). Therefore, an additional PCB is added to the remote, which is placed right between the input devices and the main controller board. It is able to read the input-devices and control the corresponding inputs of the main controller board. Additionally, it is able to communicate with the joystickunit via SPI.

If the joystick unit is connected to this hardware, the board samples the input devices and sends this data to the joystick unit, where it can be used as input data. Parallel to this, the joystick unit sends the data for the output channels, which are then set by the board accordingly. Therefore, the joystick unit gains full access over the inputs of the remote unit.

If the joystick unit is not connected to the remoteunit, the additional board acts transparent and copies the inputs directly to the outputs. Therefore, the remote control can be normally used without the joystick unit.



## Content
### Folder "firmware"
Contains the firmware of the joystickunit and the remoteunit. Both firmwares were originally developed in STM32CubeIDE. The corresponding project files for this IDE were also included in this directory. 

To flash the firmware to the units a JTAG-programmer (e.g. ST-Link) and an adapter (e.g. Olimex ARM-JTAG-20-10) is needed.
 
### Folder "hardware"
Contains the hardware for both units. The PCBs were developed in Autodesk EAGLE. However, the folder contains also a PDF of the schematics and GERBER-data for manufacturing.

**NOTE:** The bill of material can be found in the documentation-folder.

### Folder "mechanics"

Contains the 3D-models of the enclosure for the joystickunit, which can be printed with a 3D-printer. They were developed in Autodesk Fusion 360.

### Folder "documentation"
Contains a bill of material and some additional documentation for the system.
