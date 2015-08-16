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
uint8_t GR55_MIDI_port = 0;
// ********************************* Section 1: GR55 SYSEX messages ********************************************

//Sysex messages for Roland GR-55
#define GR55_REQUEST_PATCHNAME 0x18000000, 17 //Request 17 bytes for current patch name - GR55 send one dummy byte before the patchname data
#define GR55_REQUEST_PATCH_NUMBER 0x01000000, 2 //Request current patch number
#define GR55_CTL_LED_ON 0x18000011, 0x01
#define GR55_CTL_LED_OFF 0x18000011, 0x00

#define GR55_TEMPO 0x1800023C  // Accepts values from 40 bpm - 250 bpm

#define GR55_SYNTH1_SW 0x18002003 // The address of the synth1 switch
#define GR55_SYNTH2_SW 0x18002103 // The address of the synth1 switch
#define GR55_COSM_GUITAR_SW 0x1800100A // The address of the COSM guitar switch
#define GR55_NORMAL_PU_SW 0x18000232 // The address of the COSM guitar switch
uint8_t GR55_synth1_onoff = 0;
uint8_t GR55_synth2_onoff = 0;
uint8_t GR55_COSM_onoff = 0;
uint8_t GR55_nrml_pu_onoff = 0;
bool GR55_request_onoff = false;
bool GR55_always_on = false;

// ********************************* Section 2: G%55 comon MIDI in functions ********************************************

void check_SYSEX_in_GR55(const unsigned char* sxdata, short unsigned int sxlength)
{
  // Check if it is a message from a GR-55
  if ((sxdata[2] == GR55_device_id) && (sxdata[3] == 0x00) && (sxdata[4] == 0x00) && (sxdata[5] == 0x53)) {
    uint32_t address = (sxdata[7] << 24) + (sxdata[8] << 16) + (sxdata[9] << 8) + sxdata[10]; // Make the address 32 bit

    // Check if it is the patch number (address: 0x01, 0x00, 0x00, 0x00)
    if ((sxdata[6] == 0x12) && (address == 0x01000000) ) {
      GR55_patch_number = sxdata[11] * 128 + sxdata[12];
      if (GR55_patch_number > 2047) GR55_patch_number = GR55_patch_number - 1751; // There is a gap of 1752 patches in the numbering system of the GR-55. This will close it.
      GR55_do_after_patch_selection();
    }


    // Check if it is the patch name (address: 0x18, 0x00, 0x00, 0x00)
    if ((sxdata[6] == 0x12) && (address == 0x18000000) ) {
      GR55_patch_name = "";
      for (uint8_t count = 12; count < 28; count++) {
        GR55_patch_name = GR55_patch_name + static_cast<char>(sxdata[count]); //Add ascii character to Patch Name String
      }
      update_lcd = true;
      // Sending the current bpm had to be done a little later. Doing at after receiving the patch name gives enough delay.
      if (SEND_GLOBAL_TEMPO_AFTER_PATCH_CHANGE == true) GR55_send_bpm();
      GR55_request_guitar_switch_states();
    }

    // Check if it is the guitar on/off states
    GR55_check_guitar_switch_states(sxdata, sxlength);
  }
}

void check_PC_in_GR55(byte channel, byte program) {
  // Check the source by checking the channel
  if (channel == GR55_MIDI_channel) { // GR55 outputs a program change
    GR55_patch_number = (GR55_CC01 * 128) + program;
    if (GR55_patch_number > 2047) GR55_patch_number = GR55_patch_number - 1751; // There is a gap of 1752 patches in the numbering system of the GR-55. This will close it.
    GR55_do_after_patch_selection();
  }
}

void GR55_identity_check(const unsigned char* sxdata, short unsigned int sxlength)
{
  // Check if it is a GP-10
  if ((sxdata[6] == 0x53) && (sxdata[7] == 0x02) && (GR55_detected == false)) {
    GR55_detected = true;
    show_status_message("GR-55 detected  ");
    GR55_device_id = sxdata[2]; //Byte 2 contains the correct device ID
    GR55_MIDI_port = Current_MIDI_port; // Set the correct MIDI port for this device
    Serial.println("GR-55 detected on MIDI port " + String(Current_MIDI_port));
    GR55_do_after_patch_selection();
  }
}

// ********************************* Section 3: GR55 comon MIDI out functions ********************************************

void write_GR55(uint32_t address, uint8_t value) // For sending one data byte
{
  uint8_t *ad = (uint8_t*)&address; //Split the 32-bit address into four bytes: ad[3], ad[2], ad[1] and ad[0]
  uint8_t checksum = (0x80 - (ad[3] + ad[2] + ad[1] + ad[0] + value) % 0x80); // Calculate the Roland checksum
  uint8_t sysexmessage[14] = {0xF0, 0x41, GR55_device_id, 0x00, 0x00, 0x053, 0x12, ad[3], ad[2], ad[1], ad[0], value, checksum, 0xF7};
  if (GR55_MIDI_port == USBMIDI_PORT) usbMIDI.sendSysEx(14, sysexmessage);
  if (GR55_MIDI_port == MIDI1_PORT) MIDI1.sendSysEx(14, sysexmessage);
  if (GR55_MIDI_port == MIDI2_PORT) MIDI2.sendSysEx(14, sysexmessage);
  if (GR55_MIDI_port == MIDI3_PORT) MIDI2.sendSysEx(14, sysexmessage);
  debug_sysex(sysexmessage, 14, "out(GR55)");
}

void write_GR55(uint32_t address, uint8_t value1, uint8_t value2) // For sending two data bytes
{
  uint8_t *ad = (uint8_t*)&address; //Split the 32-bit address into four bytes: ad[3], ad[2], ad[1] and ad[0]
  uint8_t checksum = (0x80 - (ad[3] + ad[2] + ad[1] + ad[0] + value1 + value2) % 0x80); // Calculate the Roland checksum
  uint8_t sysexmessage[15] = {0xF0, 0x41, GR55_device_id, 0x00, 0x00, 0x053, 0x12, ad[3], ad[2], ad[1], ad[0], value1, value2, checksum, 0xF7};
  if (GR55_MIDI_port == USBMIDI_PORT) usbMIDI.sendSysEx(15, sysexmessage);
  if (GR55_MIDI_port == MIDI1_PORT) MIDI1.sendSysEx(15, sysexmessage);
  if (GR55_MIDI_port == MIDI2_PORT) MIDI2.sendSysEx(15, sysexmessage);
  if (GR55_MIDI_port == MIDI3_PORT) MIDI2.sendSysEx(15, sysexmessage);
  debug_sysex(sysexmessage, 15, "out(GR55)");
}

void request_GR55(uint32_t address, uint8_t no_of_bytes)
{
  uint8_t *ad = (uint8_t*)&address; //Split the 32-bit address into four bytes: ad[3], ad[2], ad[1] and ad[0]
  uint8_t checksum = (0x80 - (ad[3] + ad[2] + ad[1] + ad[0] +  no_of_bytes) % 0x80); // Calculate the Roland checksum
  uint8_t sysexmessage[17] = {0xF0, 0x41, GR55_device_id, 0x00, 0x00, 0x53, 0x11, ad[3], ad[2], ad[1], ad[0], 0x00, 0x00, 0x00, no_of_bytes, checksum, 0xF7};
  if (GR55_MIDI_port == USBMIDI_PORT) usbMIDI.sendSysEx(17, sysexmessage);
  if (GR55_MIDI_port == MIDI1_PORT) MIDI1.sendSysEx(17, sysexmessage);
  if (GR55_MIDI_port == MIDI2_PORT) MIDI2.sendSysEx(17, sysexmessage);
  if (GR55_MIDI_port == MIDI3_PORT) MIDI2.sendSysEx(17, sysexmessage);
  debug_sysex(sysexmessage, 17, "out(GR55)");
}

// Send Program Change to GR-55
void GR55_SendProgramChange(uint8_t new_patch) {
  if (new_patch == GR55_patch_number) GR55_unmute();
  GR55_patch_number = new_patch;
  
  uint16_t GR55_patch_send = 0; // Temporary value
  if (GR55_patch_number > 296) {
    GR55_patch_send = GR55_patch_number + 1751; // There is a gap of 1752 patches in the numbering system of the GR-55. This will recreate it.
  }
  else {
    GR55_patch_send = GR55_patch_number;
  }

  if (GR55_MIDI_port == USBMIDI_PORT) {
    usbMIDI.sendControlChange(0 , GR55_patch_send / 128, GR55_MIDI_channel); // First send the bank number to CC00
    usbMIDI.sendProgramChange(GR55_patch_send % 128, GR55_MIDI_channel);
  }
  if (GR55_MIDI_port == MIDI1_PORT) {
    MIDI1.sendControlChange(0 , GR55_patch_send / 128, GR55_MIDI_channel); // First send the bank number to CC00
    MIDI1.sendProgramChange(GR55_patch_send % 128, GR55_MIDI_channel);
  }
  if (GR55_MIDI_port == MIDI2_PORT) {
    MIDI2.sendControlChange(0 , GR55_patch_send / 128, GR55_MIDI_channel); // First send the bank number to CC00
    MIDI2.sendProgramChange(GR55_patch_send % 128, GR55_MIDI_channel);
  }
  if (GR55_MIDI_port == MIDI3_PORT) {
    MIDI3.sendControlChange(0 , GR55_patch_send / 128, GR55_MIDI_channel); // First send the bank number to CC00
    MIDI3.sendProgramChange(GR55_patch_send % 128, GR55_MIDI_channel);
  }
  GR55_do_after_patch_selection();
}

void GR55_do_after_patch_selection() {
  GP10_mute();
  VG99_mute();
  GR55_request_name();
  //GR55_request_guitar_switch_states(); // Moet later
  //GR55_request_stompbox_states();
  //if (SEND_GLOBAL_TEMPO_AFTER_PATCH_CHANGE == true) GR55_send_bpm(); // Here is too soon, the GR55 does not pick it up - this line is moved to the Check_MIDI_in_GR55() procedure.
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

void GR55_send_bpm() {
  write_GR55(GR55_TEMPO, bpm / 16, bpm % 16); // Tempo is modulus 16. It's all so very logical. NOT.
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

// ********************************* Section 4: GR55 stompbox functions ********************************************

// ** US-20 simulation 
// Selecting and muting the GR55 is done by storing the settings of Synth 1 and 2 and COSM guitar switch and Normal PU switch
// and switching both off when guitar is muted and back to original state when the GR55 is selected


void GR55_request_guitar_switch_states() {
  GR55_select_LED = GR55_PATCH_COLOUR; //Switch the LED on
  request_GR55(GR55_SYNTH1_SW, 1);
  request_GR55(GR55_SYNTH2_SW, 1);
  request_GR55(GR55_COSM_GUITAR_SW, 1);
  request_GR55(GR55_NORMAL_PU_SW, 1);
  GR55_request_onoff = true;
}

void GR55_check_guitar_switch_states(const unsigned char* sxdata, short unsigned int sxlength) {

  if (GR55_request_onoff == true) {
    uint32_t address = (sxdata[7] << 24) + (sxdata[8] << 16) + (sxdata[9] << 8) + sxdata[10]; // Make the address 32 bit

    if (address == GR55_SYNTH1_SW) {
      GR55_synth1_onoff = sxdata[11];  // Store the value
    }

    if (address == GR55_SYNTH2_SW) {
      GR55_synth2_onoff = sxdata[11];  // Store the value
    }

    if (address == GR55_COSM_GUITAR_SW) {
      GR55_COSM_onoff = sxdata[11];  // Store the value
    }

    if (address == GR55_NORMAL_PU_SW) {
      GR55_nrml_pu_onoff = sxdata[11];  // Store the value
      GR55_request_onoff = false;
    }
  }
}

void GR55_select_switch() {
  if (GR55_select_LED == GR55_PATCH_COLOUR) {
    GR55_always_on = !GR55_always_on; // Toggle GR55_always_on
    if (GR55_always_on) show_status_message("GR55 always ON");
    else show_status_message("GR55 can be muted");
  }
  else {
    GR55_unmute();
    show_status_message(GR55_patch_name); // Show the correct patch name
  }
  GP10_mute();
  VG99_mute();
}

void GR55_unmute() {
  GR55_select_LED = GR55_PATCH_COLOUR; //Switch the LED on
  write_GR55(GR55_SYNTH1_SW, GR55_synth1_onoff); // Switch synth 1 off
  write_GR55(GR55_SYNTH2_SW, GR55_synth2_onoff); // Switch synth 1 off
  write_GR55(GR55_COSM_GUITAR_SW, GR55_COSM_onoff); // Switch COSM guitar on
  write_GR55(GR55_NORMAL_PU_SW, GR55_nrml_pu_onoff); // Switch normal pu on
}

void GR55_mute() {
  if (GR55_always_on == false) {
    GR55_select_LED = GR55_OFF_COLOUR; //Switch the LED off
    write_GR55(GR55_SYNTH1_SW, 0x01); // Switch synth 1 off
    write_GR55(GR55_SYNTH2_SW, 0x01); // Switch synth 1 off
    write_GR55(GR55_COSM_GUITAR_SW, 0x01); // Switch COSM guitar off
    write_GR55(GR55_NORMAL_PU_SW, 0x01); // Switch normal pu off
  }
}
