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

// Manages the 128 bytes EEPROM of Teensy LC

#include <EEPROM.h>

// ************************ Settings you probably won't have to change ***********************
// Define EEPROM addresses
#define EEPROM_GP10_PATCH_NUMBER 0
#define EEPROM_GR55_PATCH_MSB 1
#define EEPROM_GR55_PATCH_LSB 2
#define EEPROM_VG99_PATCH_MSB 3
#define EEPROM_VG99_PATCH_LSB 4
#define EEPROM_mode 5
#define EEPROM_bpm 6
//#define EEPROM_stomp_base_address 10 // Store stompbox LEDs from address 10 and higher

void setup_eeprom()
{
  // Read data from EEPROM memory
  GP10_patch_number = EEPROM.read(EEPROM_GP10_PATCH_NUMBER);
  GR55_patch_number = (EEPROM.read(EEPROM_GR55_PATCH_MSB) * 256) + EEPROM.read(EEPROM_GR55_PATCH_LSB);
  VG99_patch_number = (EEPROM.read(EEPROM_VG99_PATCH_MSB) * 256) + EEPROM.read(EEPROM_VG99_PATCH_LSB);
  if (mode == 0) mode = EEPROM.read(EEPROM_mode); //if mode is set to a different value under settings, the last state is not read from EEPROM
   bpm = EEPROM.read(EEPROM_bpm);
}

void main_eeprom()
{

}

