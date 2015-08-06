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

// ******************************** MIDI messages and functions for the Roland FC300 ********************************

void FC300_identity_check(const unsigned char* sxdata, short unsigned int sxlength)
{
  // Check if it is a GP-10
  if ((sxdata[6] == 0x1E) && (sxdata[7] == 0x02) && (FC300_detected == false)) {
    FC300_detected = true;
    show_status_message("FC-300 detected  ");
    FC300_device_id = sxdata[2]; //Byte 2 contains the correct device ID
  }
}
