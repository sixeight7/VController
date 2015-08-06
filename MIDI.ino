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

// Setup of usbMIDI and Serial midi in and out
// Specific messages are listed under MIDI_GP10, MIDI_GR55 and MIDI_VG99
#include <MIDI.h>

// ***************************** Settings you may want to change *****************************
#define CHECK4DEVICES_TIMER_LENGTH 3000 // Check every three seconds which Roland devices are connected
unsigned long Check4DevicesTimer = 0;

// ***************************** Hardware settings *****************************
// Setup serial ports - the usb MIDI is set by TeensyDuino (set USB Type to MIDI!)
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI1); //Enables serial1 port for MIDI communication!
MIDI_CREATE_INSTANCE(HardwareSerial, Serial2, MIDI2); //Enables serial2 port for MIDI communication!

// ******************************** MIDI In section ********************************************

// Sysex for detecting Roland devices
#define Anybody_out_there {0xF0, 0x7E, 0x10, 0x06, 0x01, 0xF7}  //Detects GP-10 and GR-55

void OnNoteOn(byte channel, byte note, byte velocity)
{

}

void OnNoteOff(byte channel, byte note, byte velocity)
{

}

void OnProgramChange(byte channel, byte program)
{
  check_PC_in_GP10(channel, program);
  check_PC_in_GR55(channel, program);
  check_PC_in_VG99(channel, program);
}

void OnControlChange(byte channel, byte control, byte value)
{
  // Check the source by checking the channel
  if (channel == GP10_MIDI_channel) { // GP10 outputs a control change message

  }

  if (channel == GR55_MIDI_channel) { // GR55 outputs a control change message
    if (control == 0) {
      GR55_CC01 = value;
    }
  }

  if (channel == VG99_MIDI_channel) { // GR55 outputs a control change message
    if (control == 0) {
      VG99_CC01 = value;
    }
  }
}

void OnSysEx(const unsigned char* sxdata, short unsigned int sxlength, bool sx_comp)
{
  if (sxdata[1] == 0x41) { //Check if it is a message from a Roland device
    check_SYSEX_in_GP10(sxdata, sxlength);
    check_SYSEX_in_GR55(sxdata, sxlength);
    check_SYSEX_in_VG99(sxdata, sxlength);
    check_SYSEX_in_FC300(sxdata, sxlength);
  }

  if (sxdata[1] == 0x7E) { //Check if it is a Universal Non-Real Time message
    check_SYSEX_in_universal(sxdata, sxlength);
  }
  debug_sysex(sxdata, sxlength, "in-USB   ");
  //MIDI1.sendSysEx(sxlength, sxdata); // MIDI through usb to serial
  //MIDI2.sendSysEx(sxlength, sxdata); // MIDI through usb to serial
}

void OnSerialSysEx(byte *sxdata, unsigned sxlength)
{
  if (sxdata[1] == 0x41) { //Check if it is a message from a Roland device
    check_SYSEX_in_GP10(sxdata, sxlength);
    check_SYSEX_in_GR55(sxdata, sxlength);
    check_SYSEX_in_VG99(sxdata, sxlength);
    check_SYSEX_in_FC300(sxdata, sxlength);
  }

  if (sxdata[1] == 0x7E) { //Check if it is a Universal Non-Real Time message
    check_SYSEX_in_universal(sxdata, sxlength);
  }
  debug_sysex(sxdata, sxlength, "in-serial");
  //usbMIDI.sendSysEx(sxlength, sxdata); // MIDI through serial to usb
}

void setup_MIDI_common()
{
  usbMIDI.setHandleNoteOff(OnNoteOff);
  usbMIDI.setHandleNoteOn(OnNoteOn) ;
  usbMIDI.setHandleProgramChange(OnProgramChange);
  usbMIDI.setHandleControlChange(OnControlChange);
  usbMIDI.setHandleSysEx(OnSysEx);

  MIDI1.begin(MIDI_CHANNEL_OMNI);
  MIDI1.setHandleNoteOff(OnNoteOff);
  MIDI1.setHandleNoteOn(OnNoteOn) ;
  MIDI1.setHandleProgramChange(OnProgramChange);
  MIDI1.setHandleControlChange(OnControlChange);
  MIDI1.setHandleSystemExclusive(OnSerialSysEx);

  MIDI2.begin(MIDI_CHANNEL_OMNI);
  MIDI2.setHandleNoteOff(OnNoteOff);
  MIDI2.setHandleNoteOn(OnNoteOn) ;
  MIDI2.setHandleProgramChange(OnProgramChange);
  MIDI2.setHandleControlChange(OnControlChange);
  MIDI2.setHandleSystemExclusive(OnSerialSysEx);
}

void main_MIDI_common()
{
  usbMIDI.read();
  MIDI1.read();
  MIDI2.read();

  check_for_roland_devices();  // Check actively if any roland devices are out there
}

// *************************************** Common functions ***************************************

void check_SYSEX_in_universal(const unsigned char* sxdata, short unsigned int sxlength)
{
  // Check if it is an identity reply from Roland
  // There is no check on the second byte (device ID), in case a device has a different device ID
  if ((sxdata[3] == 0x06) && (sxdata[4] == 0x02) && (sxdata[5] == 0x41)) {
    GP10_identity_check(sxdata, sxlength);
    GR55_identity_check(sxdata, sxlength);
    VG99_identity_check(sxdata, sxlength);
    FC300_identity_check(sxdata, sxlength);
  }
}

// check for Roland devices
void check_for_roland_devices()
{
  // Check if timer needs to be set
  if (Check4DevicesTimer == 0) {
    Check4DevicesTimer = millis();
  }

  // Check if timer runs out
  if (millis() - Check4DevicesTimer > CHECK4DEVICES_TIMER_LENGTH) {
    Check4DevicesTimer = millis(); // Reset the timer

    // Send the message to all MIDI ports
    uint8_t sysexbuffer[6] = Anybody_out_there;
    usbMIDI.sendSysEx(6, sysexbuffer);
    MIDI1.sendSysEx(6, sysexbuffer);
    MIDI2.sendSysEx(6, sysexbuffer);
    //debug_sysex(sysexbuffer, 6, "CKout:");
  }
}

//Debug sysex messages by sending them to the serial monitor
void debug_sysex(const unsigned char* sxdata, short unsigned int sxlength, String my_source) {
  if (sxdata[2] != 0x7F) { // Filter out status messages
    Serial.print(my_source + ":");
    for (uint8_t i = 0; i < sxlength; i++) {
      if (sxdata[i] < 0x10) Serial.print("0" + String(sxdata[i], HEX) + " ");
      else Serial.print(String(sxdata[i], HEX) + " ");
    }
    Serial.println();
  }
}
