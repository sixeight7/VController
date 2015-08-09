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

// ******************************** MIDI messages and functions for the Roland GR-55 ********************************

// ********************************* Section 1: GR55 SYSEX messages ********************************************

//Sysex messages for Roland GR-55
#define GR55_REQUEST_PATCHNAME 0x18000000, 17 //Request 17 bytes for current patch name - GR55 send one dummy byte before the patchname data
#define GR55_REQUEST_PATCH_NUMBER 0x01000000, 2 //Request current patch number
#define GR55_CTL_LED_ON 0x18000011, 0x01
#define GR55_CTL_LED_OFF 0x18000011, 0x00

// ********************************* Section 2: G%55 comon MIDI in functions ********************************************

void check_SYSEX_in_GR55(const unsigned char* sxdata, short unsigned int sxlength)
{
  // Check if it is a message from a GR-55
  if ((sxdata[2] == GR55_device_id) && (sxdata[3] == 0x00) && (sxdata[4] == 0x00) && (sxdata[5] == 0x53)) {

    // Check if it is the patch number (address: 0x01, 0x00, 0x00, 0x00)
    if ((sxdata[6] == 0x12) && (sxdata[7] == 0x01) && (sxdata[8] == 0x00) && (sxdata[9] == 0x00) && (sxdata[10] == 0x00) ) {
      GR55_patch_number = sxdata[11] * 128 + sxdata[12];
      if (GR55_patch_number > 2047) GR55_patch_number = GR55_patch_number - 1751; // There is a gap of 1752 patches in the numbering system of the GR-55. This will close it.
      update_lcd = true;
      update_LEDS = true;
    }
  }

  // Check if it is the patch name (address: 0x18, 0x00, 0x00, 0x00)
  if ((sxdata[6] == 0x12) && (sxdata[7] == 0x18) && (sxdata[8] == 0x00) && (sxdata[9] == 0x00) && (sxdata[10] == 0x00) ) {
    GR55_patch_name = "";
    for (uint8_t count = 12; count < 28; count++) {
      GR55_patch_name = GR55_patch_name + static_cast<char>(sxdata[count]); //Add ascii character to Patch Name String
    }
    update_lcd = true;
  }
}

void check_PC_in_GR55(byte channel, byte program) {
  // Check the source by checking the channel
  if (channel == GR55_MIDI_channel) { // GR55 outputs a program change
    GR55_patch_number = (GR55_CC01 * 128) + program;
    if (GR55_patch_number > 2047) GR55_patch_number = GR55_patch_number - 1751; // There is a gap of 1752 patches in the numbering system of the GR-55. This will close it.
    GR55_request_name();
    update_LEDS = true;
    update_lcd = true;
  }
}

void GR55_identity_check(const unsigned char* sxdata, short unsigned int sxlength)
{
  // Check if it is a GP-10
  if ((sxdata[6] == 0x53) && (sxdata[7] == 0x02) && (GR55_detected == false)) {
    GR55_detected = true;
    show_status_message("GR-55 detected  ");
    GR55_device_id = sxdata[2]; //Byte 2 contains the correct device ID
    GR55_request_patch_number();
    GR55_request_name();
  }
}

// ********************************* Section 3: GR55 comon MIDI out functions ********************************************

void write_GR55(uint32_t address, uint8_t value) // For sending one data byte
{
  uint8_t *ad = (uint8_t*)&address; //Split the 32-bit address into four bytes: ad[3], ad[2], ad[1] and ad[0]
  uint8_t checksum = (0x80 - (ad[3] + ad[2] + ad[1] + ad[0] + value) % 0x80); // Calculate the Roland checksum
  uint8_t sysexmessage[14] = {0xF0, 0x41, GR55_device_id, 0x00, 0x00, 0x053, 0x12, ad[3], ad[2], ad[1], ad[0], value, checksum, 0xF7};
  usbMIDI.sendSysEx(14, sysexmessage);
  MIDI1.sendSysEx(14, sysexmessage);
  MIDI2.sendSysEx(14, sysexmessage);
  debug_sysex(sysexmessage, 14, "out(GR55)");
}

void write_GR55(uint32_t address, uint8_t value1, uint8_t value2) // For sending two data bytes
{
  uint8_t *ad = (uint8_t*)&address; //Split the 32-bit address into four bytes: ad[3], ad[2], ad[1] and ad[0]
  uint8_t checksum = (0x80 - (ad[3] + ad[2] + ad[1] + ad[0] + value1 + value2) % 0x80); // Calculate the Roland checksum
  uint8_t sysexmessage[15] = {0xF0, 0x41, GR55_device_id, 0x00, 0x00, 0x053, 0x12, ad[3], ad[2], ad[1], ad[0], value1, value2, checksum, 0xF7};
  usbMIDI.sendSysEx(15, sysexmessage);
  MIDI1.sendSysEx(15, sysexmessage);
  MIDI2.sendSysEx(15, sysexmessage);
  debug_sysex(sysexmessage, 15, "out(GR55)");
}

void request_GR55(uint32_t address, uint8_t no_of_bytes)
{
  uint8_t *ad = (uint8_t*)&address; //Split the 32-bit address into four bytes: ad[3], ad[2], ad[1] and ad[0]
  uint8_t checksum = (0x80 - (ad[3] + ad[2] + ad[1] + ad[0] +  no_of_bytes) % 0x80); // Calculate the Roland checksum
  uint8_t sysexmessage[17] = {0xF0, 0x41, GR55_device_id, 0x00, 0x00, 0x53, 0x11, ad[3], ad[2], ad[1], ad[0], 0x00, 0x00, 0x00, no_of_bytes, checksum, 0xF7};
  usbMIDI.sendSysEx(17, sysexmessage);
  MIDI1.sendSysEx(17, sysexmessage);
  MIDI2.sendSysEx(17, sysexmessage);
}

// Send Program Change to GR-55
void GR55_SendProgramChange() {
  uint16_t GR55_patch_send = 0; // Temporary value
  if (GR55_patch_number > 296) {
    GR55_patch_send = GR55_patch_number + 1751; // There is a gap of 1752 patches in the numbering system of the GR-55. This will recreate it.
  }
  else {
      GR55_patch_send = GR55_patch_number;
  }
  
  MIDI1.sendControlChange(0 , GR55_patch_send / 128, GR55_MIDI_channel); // First send the bank number to CC00
  MIDI1.sendProgramChange(GR55_patch_send % 128, GR55_MIDI_channel);
  MIDI2.sendControlChange(0 , GR55_patch_send / 128, GR55_MIDI_channel); // First send the bank number to CC00
  MIDI2.sendProgramChange(GR55_patch_send % 128, GR55_MIDI_channel);
  usbMIDI.sendControlChange(0 , GR55_patch_send / 128, GR55_MIDI_channel); // First send the bank number to CC00
  usbMIDI.sendProgramChange(GR55_patch_send % 128, GR55_MIDI_channel);
  GR55_request_name();
  update_LEDS = true;
  update_lcd = true;
  EEPROM.write(EEPROM_GR55_PATCH_MSB, (GR55_patch_number / 256));
  EEPROM.write(EEPROM_GR55_PATCH_LSB, (GR55_patch_number % 256));
}

void GR55_request_patch_number()
{
  request_GR55(GR55_REQUEST_PATCH_NUMBER);
}

void GR55_request_name()
{
  request_GR55(GR55_REQUEST_PATCHNAME);
}

// Switch GR-55 CTL pedal on and off
boolean CTL_LED = false;
void GR55_toggle_CTL_LED()
{
  CTL_LED = !CTL_LED;
  if (CTL_LED) {
    show_status_message("CTL pedal on     ");
    write_GR55(GR55_CTL_LED_ON);
    //    uint8_t sysexbuffer[15] = GR55_CTL_LED_ON;
    //    usbMIDI.sendSysEx(15, sysexbuffer);
  }
  else
  {
    show_status_message("CTL pedal off    ");
    write_GR55(GR55_CTL_LED_OFF);
    //    uint8_t sysexbuffer[15] = GR55_CTL_LED_OFF;
    //    usbMIDI.sendSysEx(15, sysexbuffer);
  }
}
