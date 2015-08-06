/****************************************************************************
**
** Copyright (C) 2015 Catrinus Feddema
** All rights reserved.
** This file is part of "VController" teensy software.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License along
** with this program; if not, write to the Free Software Foundation, Inc.,
** 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
**
****************************************************************************/

// Basic settings of the VController - here you can quickly program the pedal

// ****************** SECTION 1: MIDI SETUP ******************
// ***** Common midi settings:
uint8_t GP10_MIDI_channel = 1;
uint8_t GR55_MIDI_channel = 7;
uint8_t VG99_MIDI_channel = 8;

// ****************** SECTION 2: SWITCH SETUP ******************
// ***** Set functionality of the top three mode switches - make sure it matches the LED definitions below
#define SWITCH10_FUNC toggle_mode(MODE_VG99_PATCH, MODE_STOMP_3)
#define VG99_LED 10
#define SWITCH11_FUNC toggle_mode(MODE_GR55_PATCH, MODE_STOMP_2)
#define GR55_LED 11
#define SWITCH12_FUNC toggle_mode(MODE_GP10_PATCH, MODE_STOMP_1)
#define GP10_LED 12

#define SWITCH10_LONG_FUNC select_mode(MODE_VG99_DIRECTSELECT1)
#define SWITCH11_LONG_FUNC select_mode(MODE_GR55_DIRECTSELECT1)
#define SWITCH12_LONG_FUNC select_mode(MODE_GP10_DIRECTSELECT1)

// ***** Set functionality of the external switches. Use names as defined in the Switch_FUNCtions_common section
#define EXT1_FUNC bank_up()
#define EXT2_FUNC bank_down()
#define EXT3_FUNC start_global_tuner()
#define EXT4_FUNC start_global_tuner()

//***** Set functionality of the 9 buttons in stompbox mode 1 - make sure you connect both the stomp and the LED!
#define STOMP_SWITCH1_1_PRESS GP10_stomp(0)
#define STOMP_SWITCH1_1_RELEASE nothing() 
#define STOMP_SWITCH1_1_LED GP10_stomps[0].LED

#define STOMP_SWITCH1_2_PRESS GP10_stomp(1)
#define STOMP_SWITCH1_2_RELEASE nothing() 
#define STOMP_SWITCH1_2_LED GP10_stomps[1].LED

#define STOMP_SWITCH1_3_PRESS GP10_stomp(2)
#define STOMP_SWITCH1_3_RELEASE nothing() 
#define STOMP_SWITCH1_3_LED GP10_stomps[2].LED

#define STOMP_SWITCH1_4_PRESS GP10_stomp(3)
#define STOMP_SWITCH1_4_RELEASE nothing() 
#define STOMP_SWITCH1_4_LED GP10_stomps[3].LED

#define STOMP_SWITCH1_5_PRESS GP10_stomp(4)
#define STOMP_SWITCH1_5_RELEASE nothing() 
#define STOMP_SWITCH1_5_LED GP10_stomps[4].LED

#define STOMP_SWITCH1_6_PRESS GP10_stomp(5)
#define STOMP_SWITCH1_6_RELEASE nothing() 
#define STOMP_SWITCH1_6_LED GP10_stomps[5].LED

#define STOMP_SWITCH1_7_PRESS nothing()
#define STOMP_SWITCH1_7_RELEASE nothing() 
#define STOMP_SWITCH1_7_LED LEDoff

#define STOMP_SWITCH1_8_PRESS start_global_tuner()
#define STOMP_SWITCH1_8_RELEASE nothing() 
#define STOMP_SWITCH1_8_LED global_tuner_LED

#define STOMP_SWITCH1_9_PRESS nothing()
#define STOMP_SWITCH1_9_RELEASE nothing() 
#define STOMP_SWITCH1_9_LED LEDoff

//Set functionality of the 9 buttons in stompbox mode 2 - make sure you connect both the stomp and the LED!
#define STOMP_SWITCH2_1_PRESS nothing()
#define STOMP_SWITCH2_1_RELEASE nothing() 
#define STOMP_SWITCH2_1_LED LEDoff

#define STOMP_SWITCH2_2_PRESS nothing()
#define STOMP_SWITCH2_2_RELEASE nothing() 
#define STOMP_SWITCH2_2_LED LEDoff

#define STOMP_SWITCH2_3_PRESS nothing()
#define STOMP_SWITCH2_3_RELEASE nothing() 
#define STOMP_SWITCH2_3_LED LEDoff

#define STOMP_SWITCH2_4_PRESS nothing()
#define STOMP_SWITCH2_4_RELEASE nothing() 
#define STOMP_SWITCH2_4_LED LEDoff

#define STOMP_SWITCH2_5_PRESS nothing()
#define STOMP_SWITCH2_5_RELEASE nothing() 
#define STOMP_SWITCH2_5_LED LEDoff

#define STOMP_SWITCH2_6_PRESS nothing()
#define STOMP_SWITCH2_6_RELEASE nothing() 
#define STOMP_SWITCH2_6_LED LEDoff

#define STOMP_SWITCH2_7_PRESS nothing()
#define STOMP_SWITCH2_7_RELEASE nothing() 
#define STOMP_SWITCH2_7_LED LEDoff

#define STOMP_SWITCH2_8_PRESS start_global_tuner()
#define STOMP_SWITCH2_8_RELEASE nothing() 
#define STOMP_SWITCH2_8_LED global_tuner_LED

#define STOMP_SWITCH2_9_PRESS nothing()
#define STOMP_SWITCH2_9_RELEASE nothing() 
#define STOMP_SWITCH2_9_LED LEDoff

//Set functionality of the 9 buttons in stompbox mode 3 - make sure you connect both the stomp and the LED!
#define STOMP_SWITCH3_1_PRESS FC300_stomp_press(0)
#define STOMP_SWITCH3_1_RELEASE FC300_stomp_release(0)
#define STOMP_SWITCH3_1_LED FC300_ctls[0].LED

#define STOMP_SWITCH3_2_PRESS FC300_stomp_press(1)
#define STOMP_SWITCH3_2_RELEASE FC300_stomp_release(1)
#define STOMP_SWITCH3_2_LED FC300_ctls[1].LED

#define STOMP_SWITCH3_3_PRESS FC300_stomp_press(2)
#define STOMP_SWITCH3_3_RELEASE FC300_stomp_release(2)
#define STOMP_SWITCH3_3_LED FC300_ctls[2].LED

#define STOMP_SWITCH3_4_PRESS FC300_stomp_press(3)
#define STOMP_SWITCH3_4_RELEASE FC300_stomp_release(3)
#define STOMP_SWITCH3_4_LED FC300_ctls[3].LED

#define STOMP_SWITCH3_5_PRESS FC300_stomp_press(4)
#define STOMP_SWITCH3_5_RELEASE FC300_stomp_release(4)
#define STOMP_SWITCH3_5_LED FC300_ctls[4].LED

#define STOMP_SWITCH3_6_PRESS FC300_stomp_press(5)
#define STOMP_SWITCH3_6_RELEASE FC300_stomp_release(5)
#define STOMP_SWITCH3_6_LED FC300_ctls[5].LED

#define STOMP_SWITCH3_7_PRESS FC300_stomp_press(6)
#define STOMP_SWITCH3_7_RELEASE FC300_stomp_release(6)
#define STOMP_SWITCH3_7_LED FC300_ctls[6].LED

#define STOMP_SWITCH3_8_PRESS FC300_stomp_press(7)
#define STOMP_SWITCH3_8_RELEASE FC300_stomp_release(7)
#define STOMP_SWITCH3_8_LED FC300_ctls[7].LED

#define STOMP_SWITCH3_9_PRESS start_global_tuner()
#define STOMP_SWITCH3_9_RELEASE nothing()
#define STOMP_SWITCH3_9_LED global_tuner_LED

// ****************** SECTION 3: PATCH UP/DOWN PATCH EXTEND SETUP ******************
// GP-10 bank extend (when using bank up/down) - set values between 0 and 9.
#define GP10_BANK_MIN 0
#define GP10_BANK_MAX 9

// GR-55 bank extend (when using bank up/down) - set values between 0 and 99 for just the user patches and 1 and 219 for user and preset banks
// To keep the banks equal, make sure the numbers can be divided by three, because we always have three banks on display
#define GR55_BANK_MIN 0
#define GR55_BANK_MAX 219

// VG-99 bank extend (when using bank up/down) - set values between 0 and 19 for just the user patches and 0 and 39 for user and preset banks
#define VG99_BANK_MIN 0
#define VG99_BANK_MAX 39


// ************************ Settings you probably won't have to change ***********************
uint8_t GP10_device_id = 0x10; // Device ID's are read automatically
uint8_t GR55_device_id = 0x10;
uint8_t VG99_device_id = 0x10;
uint8_t FC300_device_id = 0x00;

// Pedal can be in different modes. In each mode the buttons have different functions
uint8_t mode = 0; // Variable mode is declared. Last state is remembered in EEPROM
uint8_t previous_mode = 0;

// To make the code more readable, all the states are given a desciptive name
#define MODE_TUNER 0
#define MODE_GP10_PATCH 1
#define MODE_GP10_DIRECTSELECT1 2
#define MODE_GP10_DIRECTSELECT2 3
#define MODE_GR55_PATCH 4
#define MODE_GR55_DIRECTSELECT1 5
#define MODE_GR55_DIRECTSELECT2 6
#define MODE_GR55_DIRECTSELECT3 7
#define MODE_VG99_PATCH 8
#define MODE_VG99_DIRECTSELECT1 9
#define MODE_VG99_DIRECTSELECT2 10
#define MODE_GENERIC1_PATCH 11           // Used for other devices  
#define MODE_GENERIC1_DIRECTSELECT1 12
#define MODE_GENERIC1_DIRECTSELECT2 13
#define MODE_GENERIC2_PATCH 14           // Used for other devices  
#define MODE_GENERIC2_DIRECTSELECT1 15
#define MODE_GENERIC2_DIRECTSELECT2 16
// All modes with numbers 20 and up are freely assignable stomp banks
#define MODE_STOMP_1 20 //Used for GP-10 stomp
#define MODE_STOMP_2 21 //Used for GR-55 stomp
#define MODE_STOMP_3 22 //Used for VG-99 stomp
#define NUMBER_OF_STOMP_BANKS 3

String GP10_patch_name = "                ";
uint8_t GP10_patch_number = 0;
uint8_t GP10_bank_number = 0;
boolean GP10_detected = false;
boolean GP10_bank_selection_active = false;

String GR55_patch_name = "                ";
uint16_t GR55_patch_number = 0;
uint16_t GR55_previous_patch_number = 0;
uint16_t GR55_bank_number = 1;
uint8_t GR55_CC01 = 0;    // the MIDI CC #01 sent by the GR-55
boolean GR55_detected = false;
boolean GR55_bank_selection_active = false;

String VG99_patch_name = "                ";
uint16_t VG99_patch_number = 0;
uint16_t VG99_previous_patch_number = 0;
uint8_t VG99_bank_number = 0;
uint8_t VG99_CC01 = 0;    // the MIDI CC #01 sent by the GR-55
boolean VG99_detected = false;
boolean VG99_bank_selection_active = false;

boolean FC300_detected = false;






