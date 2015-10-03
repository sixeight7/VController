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

uint8_t FC300_MIDI_port = 0;

// ******************************** MIDI messages and functions for the Roland FC300 ********************************
// FC300 has two sysex identities. One as itself and one as footcontroller. Therefore every function is double!

// ********************************* Section 1: FC300 SYSEX messages ********************************************

void FC300_identity_check(const unsigned char* sxdata, short unsigned int sxlength)
{
  // Check if it is a GP-10
  if ((sxdata[6] == 0x1E) && (sxdata[7] == 0x02) && (FC300_detected == false)) {
    FC300_detected = true;
    show_status_message("FC-300 detected  ");
    FC300_device_id = sxdata[2]; //Byte 2 contains the correct device ID
    FC300_MIDI_port = Current_MIDI_port; // Set the correct MIDI port for this device
    DEBUGMSG("FC-300 detected on MIDI port " + String(Current_MIDI_port));
  }
}

void write_FC300own(uint32_t address, uint8_t value)
{
  uint8_t *ad = (uint8_t*)&address; //Split the 32-bit address into four bytes: ad[3], ad[2], ad[1] and ad[0]
  uint8_t checksum = calc_checksum(ad[3] + ad[2] + ad[1] + ad[0] + value); // Calculate the Roland checksum
  uint8_t sysexmessage[14] = {0xF0, 0x41, FC300_device_id, 0x00, 0x00, 0x01E, 0x12, ad[3], ad[2], ad[1], ad[0], value, checksum, 0xF7};
  usbMIDI.sendSysEx(14, sysexmessage);
  MIDI1.sendSysEx(13, sysexmessage);
  MIDI2.sendSysEx(13, sysexmessage);
  MIDI3.sendSysEx(13, sysexmessage);
  debug_sysex(sysexmessage, 14, "out(FC300o)");
}

void write_FC300fc(uint16_t address, uint8_t value)
{
  uint8_t *ad = (uint8_t*)&address; //Split the 32-bit address into two bytes: ad[1] and ad[0]
  uint8_t checksum = calc_checksum(ad[1] + ad[0] + value); // Calculate the Roland checksum
  uint8_t sysexmessage[12] = {0xF0, 0x41, FC300_device_id, 0x00, 0x00, 0x020, 0x12, ad[1], ad[0], value, checksum, 0xF7};
  usbMIDI.sendSysEx(12, sysexmessage);
  MIDI1.sendSysEx(11, sysexmessage);
  MIDI2.sendSysEx(11, sysexmessage);
  MIDI3.sendSysEx(11, sysexmessage);
  debug_sysex(sysexmessage, 12, "out(FC300f)");
}

