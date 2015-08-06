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

// Here are the actual logic of the pedal - what to do after a switch is pressed



// ************************ Settings you probably won't have to change ************************

boolean switch10_used = false; //For Directselect modes switch 10 is used as a 0. Then its SWITCH10_FUNC should be ignored

void setup_switch_control()
{
  //initialize LED pointers
  init_LED_pointers();
}

// Do something with buttons being pressed
void main_switch_control()  // Checks if a button has been pressed and calls the subroutine that belongs to the mode
{
  // Call functions for switches being pressed
  if (switch_pressed > 0) {
    //show_status_message("you pressed " + String(switch_pressed));
    update_LEDS = true;
    update_lcd = true;

    if (mode == MODE_TUNER) { // mode tuner has to be separate, so it also includes the external switches as well
      switchcheck_MODE_TUNER(); // Pressing any key will exit tuner mode!
    }
    else {

      switch10_used = false;

      switch (mode) {
        case MODE_STOMP_1:
          switchcheck_MODE_STOMPbox(1);
          break;
        case MODE_STOMP_2:
          switchcheck_MODE_STOMPbox(2);
          break;
        case MODE_STOMP_3:
          switchcheck_MODE_STOMPbox(3);
          break;
        case MODE_GP10_PATCH:
          switchcheck_MODE_GP10_PATCH();
          break;
        case MODE_GP10_DIRECTSELECT1:
          switchcheck_MODE_GP10_DIRECTSELECT1();
          break;
        case MODE_GP10_DIRECTSELECT2:
          switchcheck_MODE_GP10_DIRECTSELECT2();
          break;
        case MODE_GR55_PATCH:
          switchcheck_MODE_GR55_PATCH();
          break;
        case MODE_GR55_DIRECTSELECT1:
          switchcheck_MODE_GR55_DIRECTSELECT1();
          break;
        case MODE_GR55_DIRECTSELECT2:
          switchcheck_MODE_GR55_DIRECTSELECT2();
          break;
        case MODE_GR55_DIRECTSELECT3:
          switchcheck_MODE_GR55_DIRECTSELECT3();
          break;
        case MODE_VG99_PATCH:
          switchcheck_MODE_VG99_PATCH();
          break;
        case MODE_VG99_DIRECTSELECT1:
          switchcheck_MODE_VG99_DIRECTSELECT1();
          break;
        case MODE_VG99_DIRECTSELECT2:
          switchcheck_MODE_VG99_DIRECTSELECT2();
          break;
        default:
          select_mode(MODE_STOMP_2);
      }

      // Check top switches (10, 11 and 12)
      if ((switch_pressed == 10) && (switch10_used == false)) SWITCH10_FUNC;
      if (switch_pressed == 11) SWITCH11_FUNC;
      if (switch_pressed == 12) SWITCH12_FUNC;

      // Check external switches
      if (switch_pressed == 13) EXT1_FUNC;
      if (switch_pressed == 14) EXT2_FUNC;
      if (switch_pressed == 15) EXT3_FUNC;
      if (switch_pressed == 16) EXT4_FUNC;
    }
  }

  // Call functions for switches being released
  if (switch_released > 0) {
    //show_status_message("you released " + String(switch_released));
    update_LEDS = true;
    update_lcd = true;
    
    if (mode == MODE_STOMP_1) switchcheck_MODE_STOMPbox_release(1);
    if (mode == MODE_STOMP_2) switchcheck_MODE_STOMPbox_release(2);
    if (mode == MODE_STOMP_3) switchcheck_MODE_STOMPbox_release(3);
  }

  // Call functions for switches being long pressed
  if (switch_long_pressed > 0) {
    update_LEDS = true;
    update_lcd = true;

    switch (mode) {
      case MODE_VG99_DIRECTSELECT2:
        switchcheck_MODE_VG99_DIRECTSELECT1_long_pressed();
        break;
    }

    if (switch_long_pressed == 10) SWITCH10_LONG_FUNC;
    if (switch_long_pressed == 11) SWITCH11_LONG_FUNC;
    if (switch_long_pressed == 12) SWITCH12_LONG_FUNC;
  }
}

// Mode 0: Tuner_mode
void switchcheck_MODE_TUNER()
{
  // Leave global_tuner_mode
  stop_global_tuner();
}

// Mode 1-3: Stompbox modes
void switchcheck_MODE_STOMPbox(uint8_t page)
{
  // OK, very crude, but because I use defines, there is just no other way to do this.
  if ((page == 1) && (switch_pressed == 1)) STOMP_SWITCH1_1_PRESS;
  if ((page == 1) && (switch_pressed == 2)) STOMP_SWITCH1_2_PRESS;
  if ((page == 1) && (switch_pressed == 3)) STOMP_SWITCH1_3_PRESS;
  if ((page == 1) && (switch_pressed == 4)) STOMP_SWITCH1_4_PRESS;
  if ((page == 1) && (switch_pressed == 5)) STOMP_SWITCH1_5_PRESS;
  if ((page == 1) && (switch_pressed == 6)) STOMP_SWITCH1_6_PRESS;
  if ((page == 1) && (switch_pressed == 7)) STOMP_SWITCH1_7_PRESS;
  if ((page == 1) && (switch_pressed == 8)) STOMP_SWITCH1_8_PRESS;
  if ((page == 1) && (switch_pressed == 9)) STOMP_SWITCH1_9_PRESS;

  if ((page == 2) && (switch_pressed == 1)) STOMP_SWITCH2_1_PRESS;
  if ((page == 2) && (switch_pressed == 2)) STOMP_SWITCH2_2_PRESS;
  if ((page == 2) && (switch_pressed == 3)) STOMP_SWITCH2_3_PRESS;
  if ((page == 2) && (switch_pressed == 4)) STOMP_SWITCH2_4_PRESS;
  if ((page == 2) && (switch_pressed == 5)) STOMP_SWITCH2_5_PRESS;
  if ((page == 2) && (switch_pressed == 6)) STOMP_SWITCH2_6_PRESS;
  if ((page == 2) && (switch_pressed == 7)) STOMP_SWITCH2_7_PRESS;
  if ((page == 2) && (switch_pressed == 8)) STOMP_SWITCH2_8_PRESS;
  if ((page == 2) && (switch_pressed == 9)) STOMP_SWITCH2_9_PRESS;
  
  if ((page == 3) && (switch_pressed == 1)) STOMP_SWITCH3_1_PRESS;
  if ((page == 3) && (switch_pressed == 2)) STOMP_SWITCH3_2_PRESS;
  if ((page == 3) && (switch_pressed == 3)) STOMP_SWITCH3_3_PRESS;
  if ((page == 3) && (switch_pressed == 4)) STOMP_SWITCH3_4_PRESS;
  if ((page == 3) && (switch_pressed == 5)) STOMP_SWITCH3_5_PRESS;
  if ((page == 3) && (switch_pressed == 6)) STOMP_SWITCH3_6_PRESS;
  if ((page == 3) && (switch_pressed == 7)) STOMP_SWITCH3_7_PRESS;
  if ((page == 3) && (switch_pressed == 8)) STOMP_SWITCH3_8_PRESS;
  if ((page == 3) && (switch_pressed == 9)) STOMP_SWITCH3_9_PRESS;
}

void switchcheck_MODE_STOMPbox_release(uint8_t page)
{
  // OK, very crude, but because I use defines, there is just no other way to do this.
  if ((page == 1) && (switch_released == 1)) STOMP_SWITCH1_1_RELEASE;
  if ((page == 1) && (switch_released == 2)) STOMP_SWITCH1_2_RELEASE;
  if ((page == 1) && (switch_released == 3)) STOMP_SWITCH1_3_RELEASE;
  if ((page == 1) && (switch_released == 4)) STOMP_SWITCH1_4_RELEASE;
  if ((page == 1) && (switch_released == 5)) STOMP_SWITCH1_5_RELEASE;
  if ((page == 1) && (switch_released == 6)) STOMP_SWITCH1_6_RELEASE;
  if ((page == 1) && (switch_released == 7)) STOMP_SWITCH1_7_RELEASE;
  if ((page == 1) && (switch_released == 8)) STOMP_SWITCH1_8_RELEASE;
  if ((page == 1) && (switch_released == 9)) STOMP_SWITCH1_9_RELEASE;

  if ((page == 2) && (switch_released == 1)) STOMP_SWITCH2_1_RELEASE;
  if ((page == 2) && (switch_released == 2)) STOMP_SWITCH2_2_RELEASE;
  if ((page == 2) && (switch_released == 3)) STOMP_SWITCH2_3_RELEASE;
  if ((page == 2) && (switch_released == 4)) STOMP_SWITCH2_4_RELEASE;
  if ((page == 2) && (switch_released == 5)) STOMP_SWITCH2_5_RELEASE;
  if ((page == 2) && (switch_released == 6)) STOMP_SWITCH2_6_RELEASE;
  if ((page == 2) && (switch_released == 7)) STOMP_SWITCH2_7_RELEASE;
  if ((page == 2) && (switch_released == 8)) STOMP_SWITCH2_8_RELEASE;
  if ((page == 2) && (switch_released == 9)) STOMP_SWITCH2_9_RELEASE;
  
  if ((page == 3) && (switch_released == 1)) STOMP_SWITCH3_1_RELEASE;
  if ((page == 3) && (switch_released == 2)) STOMP_SWITCH3_2_RELEASE;
  if ((page == 3) && (switch_released == 3)) STOMP_SWITCH3_3_RELEASE;
  if ((page == 3) && (switch_released == 4)) STOMP_SWITCH3_4_RELEASE;
  if ((page == 3) && (switch_released == 5)) STOMP_SWITCH3_5_RELEASE;
  if ((page == 3) && (switch_released == 6)) STOMP_SWITCH3_6_RELEASE;
  if ((page == 3) && (switch_released == 7)) STOMP_SWITCH3_7_RELEASE;
  if ((page == 3) && (switch_released == 8)) STOMP_SWITCH3_8_RELEASE;
  if ((page == 3) && (switch_released == 9)) STOMP_SWITCH3_9_RELEASE;
}

// Mode 2: GP10 patch mode - select the patch in the current bank
void switchcheck_MODE_GP10_PATCH()
{
  if (switch_pressed < 10) {
    if (GP10_bank_selection_active == false) GP10_bank_number = (GP10_patch_number / 10); //Reset the bank to current patch
    GP10_patch_number = (GP10_bank_number) * 10 + (switch_pressed - 1);
    GP10_SendProgramChange(GP10_patch_number);
    GP10_bank_selection_active = false;
  }
}

// Mode 3: GP10 direct select mode 1 - Now the number pressed is the new bank
void switchcheck_MODE_GP10_DIRECTSELECT1()
{
  if (switch_pressed < 11) {
    GP10_bank_number = switch_pressed % 10; // Modulus 10 makes button 10 zero
    mode = MODE_GP10_DIRECTSELECT2;
    switch10_used = true;
  }

}

// Mode 4: GP10 direct select mode 2 - Now
void switchcheck_MODE_GP10_DIRECTSELECT2()
{
  if (switch_pressed <= 10) {
    select_mode(MODE_GP10_PATCH);
    GP10_patch_number = (GP10_bank_number) * 10 + (switch_pressed % 10) - 1; // switch_pressed % 10 makes buton 10 zero
    switch10_used = true;
    GP10_SendProgramChange(GP10_patch_number);
  }

}

// Mode 5: GR55 patch mode
void switchcheck_MODE_GR55_PATCH()
{
  if (switch_pressed < 10) {
    if (GR55_bank_selection_active == false) GR55_bank_number = ((GR55_patch_number / 9) * 3); // Reset the bank to current patch - bank needs update, so the VController never jumps to other banks
    GR55_patch_number = (GR55_bank_number * 3) + (switch_pressed - 1);
    GR55_SendProgramChange();
    GR55_bank_selection_active = false;
  }
}

// Mode 6: GR55 DirectSelect1 - press the first digit of the bank number
void switchcheck_MODE_GR55_DIRECTSELECT1()
{
  if (switch_pressed < 11) {
    GR55_bank_number = (switch_pressed % 10) * 10; // Modulus 10 makes button 10 zero
    mode = MODE_GR55_DIRECTSELECT2;
    switch10_used = true;
  }
}

// Mode 7: GR55 DirectSelect2 - press the second digit of the bank number
void switchcheck_MODE_GR55_DIRECTSELECT2()
{
  if (switch_pressed < 11) {
    GR55_bank_number = GR55_bank_number + (switch_pressed % 10); // Modulus 10 makes button 10 zero
    mode = MODE_GR55_DIRECTSELECT3;
    switch10_used = true;
  }
}

// Mode 8: GR55 DirectSelect3 - press the actual patch number
void switchcheck_MODE_GR55_DIRECTSELECT3()
{
  if (switch_pressed < 10) {
    GR55_patch_number = (GR55_bank_number - 1) * 3 + ((switch_pressed - 1) % 3);
    GR55_SendProgramChange();
    select_mode(MODE_GR55_PATCH);
  }
}

// Mode 6: VG99 patch mode
void switchcheck_MODE_VG99_PATCH()
{
  if (switch_pressed < 10) {
    if (VG99_bank_selection_active == false) VG99_bank_number = (VG99_patch_number / 10); //Reset the bank to current patch
    VG99_patch_number = (VG99_bank_number) * 10 + (switch_pressed - 1);
    VG99_SendProgramChange();
    VG99_bank_selection_active = false;
  }
}

// Mode 7: VG99 bank mode
void switchcheck_MODE_VG99_DIRECTSELECT1()
{
  if (switch_pressed < 11) {
    VG99_bank_number = (switch_pressed % 10); // Modulus 10 makes button 10 zero
    mode = MODE_VG99_DIRECTSELECT2;
    switch10_used = true;
  }
}

void switchcheck_MODE_VG99_DIRECTSELECT1_long_pressed()  //Long pressing a number button in Direct Select gets you in the bank from 100
{
  if (switch_pressed < 11) {
    VG99_bank_number = (switch_long_pressed % 10) + 10; // Modulus 10 makes button 10 zero
    update_lcd = true;
    switch10_used = true;
  }
}

// Mode 11: VG99 direct select mode 2
void switchcheck_MODE_VG99_DIRECTSELECT2()
{
  if (switch_pressed < 10) {
    select_mode(MODE_VG99_PATCH);
    VG99_patch_number = (VG99_bank_number) * 10 + (switch_pressed - 1);
    VG99_SendProgramChange();
  }
  if (switch_pressed == 10) {
    select_mode(MODE_VG99_PATCH);
    VG99_patch_number = (VG99_bank_number) * 10 - 1;
    VG99_bank_number = VG99_bank_number - 1;
    VG99_SendProgramChange();
    switch10_used = true;
  }
}

void select_mode(uint8_t new_mode) {
  mode = new_mode;
  EEPROM.write(EEPROM_mode, mode);
}

void toggle_mode(uint8_t mode1, uint8_t mode2) {
  if (mode == mode1) select_mode(mode2);
  else select_mode(mode1);
}

void init_LED_pointers() {
  //Initialize LED pointers
  // OK, very crude, but because I use defines, there is just no other way to do this.
  // You also would expect to find this in the LEDs section, but because the LED pointers are not set, it will not compile...
  LEDs_stomp_mode_ptr[0][0] = &STOMP_SWITCH1_1_LED;
  LEDs_stomp_mode_ptr[0][1] = &STOMP_SWITCH1_2_LED;
  LEDs_stomp_mode_ptr[0][2] = &STOMP_SWITCH1_3_LED;
  LEDs_stomp_mode_ptr[0][3] = &STOMP_SWITCH1_4_LED;
  LEDs_stomp_mode_ptr[0][4] = &STOMP_SWITCH1_5_LED;
  LEDs_stomp_mode_ptr[0][5] = &STOMP_SWITCH1_6_LED;
  LEDs_stomp_mode_ptr[0][6] = &STOMP_SWITCH1_7_LED;
  LEDs_stomp_mode_ptr[0][7] = &STOMP_SWITCH1_8_LED;
  LEDs_stomp_mode_ptr[0][8] = &STOMP_SWITCH1_9_LED;

  LEDs_stomp_mode_ptr[1][0] = &STOMP_SWITCH2_1_LED;
  LEDs_stomp_mode_ptr[1][1] = &STOMP_SWITCH2_2_LED;
  LEDs_stomp_mode_ptr[1][2] = &STOMP_SWITCH2_3_LED;
  LEDs_stomp_mode_ptr[1][3] = &STOMP_SWITCH2_4_LED;
  LEDs_stomp_mode_ptr[1][4] = &STOMP_SWITCH2_5_LED;
  LEDs_stomp_mode_ptr[1][5] = &STOMP_SWITCH2_6_LED;
  LEDs_stomp_mode_ptr[1][6] = &STOMP_SWITCH2_7_LED;
  LEDs_stomp_mode_ptr[1][7] = &STOMP_SWITCH2_8_LED;
  LEDs_stomp_mode_ptr[1][8] = &STOMP_SWITCH2_9_LED;

  LEDs_stomp_mode_ptr[2][0] = &STOMP_SWITCH3_1_LED;
  LEDs_stomp_mode_ptr[2][1] = &STOMP_SWITCH3_2_LED;
  LEDs_stomp_mode_ptr[2][2] = &STOMP_SWITCH3_3_LED;
  LEDs_stomp_mode_ptr[2][3] = &STOMP_SWITCH3_4_LED;
  LEDs_stomp_mode_ptr[2][4] = &STOMP_SWITCH3_5_LED;
  LEDs_stomp_mode_ptr[2][5] = &STOMP_SWITCH3_6_LED;
  LEDs_stomp_mode_ptr[2][6] = &STOMP_SWITCH3_7_LED;
  LEDs_stomp_mode_ptr[2][7] = &STOMP_SWITCH3_8_LED;
  LEDs_stomp_mode_ptr[2][8] = &STOMP_SWITCH3_9_LED;
}