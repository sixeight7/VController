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

/****************************************************************************
** V-Controller v1 beta
** Dedicated MIDI controller for Boss GP-10  / Roland GR-55 and Roland VG-99
** Hardware: Teensy LC, 12x 5mm Neopixel LEDs, 1402 LCD with serial interface, MIDI in/out port
**
** Author: Catrinus Feddema (aka sixeight at vguitarforums.com)
** Date:   July/August 2015
****************************************************************************/

#include <string.h>

void setup()
{
  setup_LED_control(); //Should be first, to reduce startup flash of LEDs
  Serial.begin(115200);
  while(!Serial); // Wait until the serial communication is ready
  Serial.println("VController started...");
  setup_eeprom();
  setup_switch_check();
  setup_switch_control();
  setup_LCD_control();
  setup_MIDI_common();
  //setup_GP10_stompboxes();
}

void loop()
{
  main_switch_check(); //Check for switches pressed
  main_switch_control(); //Find out what to do with it
  main_eeprom();
  main_LCD_control();
  main_LED_control();
  main_MIDI_common();
  main_switch_funcs();
  //show_status_message("bank:"+String(GR55_bank_number) + " patch:"+String(GR55_patch_number));
}
