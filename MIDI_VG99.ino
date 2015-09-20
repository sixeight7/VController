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

// ******************************** MIDI messages and functions for the Roland VG-99 and the FC300 ********************************

// Note: when connecting the VG-99 to the RRC connector make sure that you make the following settings:
// 1) Go to SYSTEM / MIDI / PAGE 4 on the VG99
// 2) Switch on RRC -> main (F1)
// 3) Switch off RRC <- main (F2 and F3)
// The reason is that the VG99 sends back patch changes to the VController, which make the system unresponsive.
// I cannot trace this back to the source, but it may be a firmware error on the VG-99.

// ********************************* Section 1: VG99/FC300 SYSEX messages ********************************************

boolean VG99_FC300_mode = false; // If the FC300 is attached, patch names will be sent in FC300 SYSEX messages

//Sysex messages for the VG-99
#define VG99_REQUEST_PATCHNAME 0x60000000, 16 //Request 16 bytes for current patch name
#define VG99_REQUEST_PATCH_NUMBER 0x71000100, 2 //Request current patch number

#define VG99_EDITOR_MODE_ON 0x70000100, 0x01 //Gets the VG-99 spitting out lots of sysex data. Does not have to be switched on for the tuner to work on the VG99
#define VG99_EDITOR_MODE_OFF 0x70000100, 0x00
#define VG99_TUNER_ON 0x70000000, 0x01 // Changes the running mode to tuner / multi-mode
#define VG99_TUNER_OFF 0x70000000, 0x00 // Changes the running mode to play
#define VG99_TAP_TEMPO_LED_CC 80 // If you have the D-beam switched off on the VG99, make an assign on the VG99: Source CC #80, momentary, target D BEAM - SELECT - Assignable / off on every patch and you will have a flashing LED. 
// Set to zero and VController will send no CC message. You can also set the cc on the VG99 to V-link sw, but that generates a lot of midi data.

#define VG99_PATCH_CHANGE 0x71000000 //00 00 Patch 001 and 03 0F Patch 400
#define VG99_TEMPO 0x60000015  // Accepts values from 40 bpm - 250 bpm
#define VG99_KEY 0x60000017    // Accepts values from 0 (C) - 11 (B)

#define VG99_COSM_GUITAR_A_SW 0x60003000 // The address of the COSM guitar switch
#define VG99_COSM_GUITAR_B_SW 0x60003800 // The address of the COSM guitar switch
uint8_t VG99_COSM_A_onoff = 0;
uint8_t VG99_COSM_B_onoff = 0;
bool VG99_request_onoff = false;

//Sysex messages for FC300 in sysex mode:
//I am trying to fool the VG-99 to believe there is an FC300 attached, but it does not work.
#define FC300_TUNER_ON 0x1002, 0x01 // Does not work for some reason
#define FC300_TUNER_OFF 0x1002, 0x00
#define FC300_SYSEX_MODE 0x1000, 0x01 //Tell the VG-99 we are in sysex mode
#define FC300_NORMAL_MODE 0x1000, 0x00

uint8_t VG99_MIDI_port = 0;
uint8_t VG99_current_assign = 255; // The assign that is being read - set to a high value, because it is not time to read an assign yet.

#define VG99_SYSEX_WATCHDOG_LENGTH 1000 // watchdog for messages (in msec)
unsigned long VG99sysexWatchdog = 0;
boolean VG99_sysex_watchdog_running = false;

// VG99 handshake with FC300
// 1. VG99 sends continually F0 41 7F 00 00 1F 11 00 01 7F F7 on RRC port (not on other ports)
// 2. FC300 responds with ???


// ********************************* Section 2: VG99 comon MIDI in functions ********************************************

void check_SYSEX_in_VG99(const unsigned char* sxdata, short unsigned int sxlength)
{
  // Check if it is a message from a VG-99
  if ((sxdata[2] == VG99_device_id) && (sxdata[3] == 0x00) && (sxdata[4] == 0x00) && (sxdata[5] == 0x1C)) {
    uint32_t address = (sxdata[7] << 24) + (sxdata[8] << 16) + (sxdata[9] << 8) + sxdata[10]; // Make the address 32 bit

    //Check if it is the patch number (address: 0x70, 0x00, 0x01, 0x00)
    if ((sxdata[6] == 0x12) && (address == 0x71000100)) {
      VG99_patch_number = sxdata[11] * 128 + sxdata[12];
      VG99_do_after_patch_selection();
    }

    // Check if it is the patch name (address: 0x60, 0x00, 0x00, 0x00)
    if ((sxdata[6] == 0x12) && (address == 0x60000000) ) {
      VG99_patch_name = "";
      for (uint8_t count = 11; count < 28; count++) {
        VG99_patch_name = VG99_patch_name + static_cast<char>(sxdata[count]); //Add ascii character to Patch Name String
      }
      update_lcd = true;
    }

    //VG99_check_guitar_switch_states(sxdata, sxlength);
    read_FC300_CTL_assigns(sxdata, sxlength);
  }

  // Check if it is a message from the VG-99 RRC port

  if ((sxdata[2] == 0x7F) && (sxdata[3] == 0x00) && (sxdata[4] == 0x00) && (sxdata[5] == 0x1F)) {
    if ((sxdata[6] == 0x11) && (sxdata[7] == 0x00)) { //VG99 looking for FC300
      // Reply with something
      //write_VG99rrc(0x00, 0x01);
    }
  }
}

void check_SYSEX_in_VG99fc(const unsigned char* sxdata, short unsigned int sxlength)
{
  // Check if it is a message from a VG-99 in FC300 mode.
  if ((sxdata[3] == 0x00) && (sxdata[4] == 0x00) && (sxdata[5] == 0x20)) {
    uint16_t address = (sxdata[7] << 8) + sxdata[8]; // Make the address 16 bit

    // Check if it is a data request - here we have to fool the VG99 into believing there is an FC300 attached
    // I found the numbers by packet sniffing the communication between the two devices. The addresses are not mentioned in the FC300 manual.

    if (sxdata[6] == 0x11) {
      if (address == 0x0000) { // Request for address 0x0000
        VG99_FC300_mode = true; //We are in FC300 mode!!!
        write_VG99fc(0x0000, 0x01, 0x00, 0x00); // Answer with three numbers - 01 00 00
        Serial.println("VG99 request for address 0x0000 answered");
      }
      if (address == 0x0400) { // Request for address 0x0400
        //write_VG99fc(0x0400, 0x03); // Answer with one number - 03
        Serial.println("VG99 request for address 0x0400 answered");
      }
      if (address == 0x0600) { // Request for address 0x0600
        write_VG99fc(0x0600, 0x10, 0x02, 0x08); // Answer with three numbers - 10 02 08
        Serial.println("VG99 request for address 0x0600 answered");
        VG99_fix_reverse_pedals();
      }
    }

    // Check if it is the patch name (address: 0x60, 0x40)
    if ((sxdata[6] == 0x12) && (address == 0x6040) ) {
      VG99_patch_name = "";
      for (uint8_t count = 9; count < 26; count++) {
        VG99_patch_name = VG99_patch_name + static_cast<char>(sxdata[count]); //Add ascii character to Patch Name String
      }
      update_lcd = true;
    }
  }
}

void check_PC_in_VG99(byte channel, byte program) {
  // Check the source by checking the channel
  if (channel == VG99_MIDI_channel) { // VG99 outputs a program change
    if (VG99_patch_number != (VG99_CC01 * 100) + program) {
      VG99_patch_number = (VG99_CC01 * 100) + program;
      VG99_do_after_patch_selection();
      Serial.println("Receive PC (VG99):" + String(VG99_patch_number));
    }
  }
}

void VG99_identity_check(const unsigned char* sxdata, short unsigned int sxlength)
{
  // Check if it is a VG-99
  if ((sxdata[6] == 0x1C) && (sxdata[7] == 0x02) && (VG99_detected == false)) {
    VG99_detected = true;
    //show_status_message("VG-99 detected  ");
    VG99_device_id = sxdata[2]; //Byte 2 contains the correct device ID
    VG99_MIDI_port = Current_MIDI_port; // Set the correct MIDI port for this device
    Serial.println("VG-99 detected on MIDI port " + String(Current_MIDI_port));
    //write_VG99(VG99_EDITOR_MODE_ON); // Put the VG-99 into editor mode - saves lots of messages on the VG99 display, but also overloads the buffer
    //write_VG99fc(FC300_SYSEX_MODE); // Wakes up the VG99 to the FC300
    VG99_do_after_patch_selection();
  }
}

// ********************************* Section 3: VG99 comon MIDI out functions *******************************************

void write_VG99(uint32_t address, uint8_t value) // For sending one data byte
{
  uint8_t *ad = (uint8_t*)&address; //Split the 32-bit address into four bytes: ad[3], ad[2], ad[1] and ad[0]
  uint8_t checksum = calc_checksum(ad[3] + ad[2] + ad[1] + ad[0] + value); // Calculate the Roland checksum
  uint8_t sysexmessage[14] = {0xF0, 0x41, VG99_device_id, 0x00, 0x00, 0x01C, 0x12, ad[3], ad[2], ad[1], ad[0], value, checksum, 0xF7};
  if (VG99_MIDI_port == USBMIDI_PORT) usbMIDI.sendSysEx(14, sysexmessage);
  if (VG99_MIDI_port == MIDI1_PORT) MIDI1.sendSysEx(13, sysexmessage);
  if (VG99_MIDI_port == MIDI2_PORT) MIDI2.sendSysEx(13, sysexmessage);
  if (VG99_MIDI_port == MIDI3_PORT) MIDI2.sendSysEx(13, sysexmessage);
  debug_sysex(sysexmessage, 14, "out(VG99)");
}

void write_VG99(uint32_t address, uint8_t value1, uint8_t value2) // For sending two data bytes
{
  uint8_t *ad = (uint8_t*)&address; //Split the 32-bit address into four bytes: ad[3], ad[2], ad[1] and ad[0]
  uint8_t checksum = calc_checksum(ad[3] + ad[2] + ad[1] + ad[0] + value1 + value2); // Calculate the Roland checksum
  uint8_t sysexmessage[15] = {0xF0, 0x41, VG99_device_id, 0x00, 0x00, 0x01C, 0x12, ad[3], ad[2], ad[1], ad[0], value1, value2, checksum, 0xF7};
  if (VG99_MIDI_port == USBMIDI_PORT) usbMIDI.sendSysEx(15, sysexmessage);
  if (VG99_MIDI_port == MIDI1_PORT) MIDI1.sendSysEx(14, sysexmessage);
  if (VG99_MIDI_port == MIDI2_PORT) MIDI2.sendSysEx(14, sysexmessage);
  if (VG99_MIDI_port == MIDI3_PORT) MIDI2.sendSysEx(14, sysexmessage);
  debug_sysex(sysexmessage, 15, "out(VG99)");
}

void write_VG99fc(uint16_t address, uint8_t value) // VG99 writing to the FC300
{
  uint8_t *ad = (uint8_t*)&address; //Split the 32-bit address into two bytes: ad[1] and ad[0]
  uint8_t checksum = calc_checksum(ad[1] + ad[0] + value); // Calculate the Roland checksum
  uint8_t sysexmessage[12] = {0xF0, 0x41, FC300_device_id, 0x00, 0x00, 0x020, 0x12, ad[1], ad[0], value, checksum, 0xF7};
  if (VG99_MIDI_port == USBMIDI_PORT) usbMIDI.sendSysEx(12, sysexmessage);
  if (VG99_MIDI_port == MIDI1_PORT) MIDI1.sendSysEx(11, sysexmessage);
  if (VG99_MIDI_port == MIDI2_PORT) MIDI2.sendSysEx(11, sysexmessage);
  if (VG99_MIDI_port == MIDI3_PORT) MIDI2.sendSysEx(11, sysexmessage);
  debug_sysex(sysexmessage, 12, "out(VG99fc)");
}

void write_VG99fc(uint16_t address, uint8_t value1, uint8_t value2, uint8_t value3) // VG99 writing to the FC300 - 3 bytes version
{
  uint8_t *ad = (uint8_t*)&address; //Split the 32-bit address into two bytes: ad[1] and ad[0]
  uint8_t checksum = calc_checksum(ad[1] + ad[0] + value1 + value2 + value3); // Calculate the Roland checksum
  uint8_t sysexmessage[14] = {0xF0, 0x41, FC300_device_id, 0x00, 0x00, 0x020, 0x12, ad[1], ad[0], value1, value2, value3, checksum, 0xF7};
  if (VG99_MIDI_port == USBMIDI_PORT) usbMIDI.sendSysEx(14, sysexmessage);
  if (VG99_MIDI_port == MIDI1_PORT) MIDI1.sendSysEx(13, sysexmessage);
  if (VG99_MIDI_port == MIDI2_PORT) MIDI2.sendSysEx(13, sysexmessage);
  if (VG99_MIDI_port == MIDI3_PORT) MIDI2.sendSysEx(13, sysexmessage);
  debug_sysex(sysexmessage, 14, "out(VG99fc)");
}

void write_VG99rrc(uint8_t address, uint8_t value) // VG99 writing to the FC300 in undocumented mode
{
  uint8_t checksum = calc_checksum(address + value); // Calculate the Roland checksum
  uint8_t sysexmessage[11] = {0xF0, 0x41, 0x7F, 0x00, 0x00, 0x01F, 0x12, address, value, checksum, 0xF7};
  if (VG99_MIDI_port == USBMIDI_PORT) usbMIDI.sendSysEx(11, sysexmessage);
  if (VG99_MIDI_port == MIDI1_PORT) MIDI1.sendSysEx(10, sysexmessage);
  if (VG99_MIDI_port == MIDI2_PORT) MIDI2.sendSysEx(10, sysexmessage);
  if (VG99_MIDI_port == MIDI3_PORT) MIDI2.sendSysEx(10, sysexmessage);
  debug_sysex(sysexmessage, 11, "out(VG99rrc)");
}

void request_VG99(uint32_t address, uint8_t no_of_bytes)
{
  uint8_t *ad = (uint8_t*)&address; //Split the 32-bit address into four bytes: ad[3], ad[2], ad[1] and ad[0]
  uint8_t checksum = calc_checksum(ad[3] + ad[2] + ad[1] + ad[0] +  no_of_bytes); // Calculate the Roland checksum
  uint8_t sysexmessage[17] = {0xF0, 0x41, VG99_device_id, 0x00, 0x00, 0x1C, 0x11, ad[3], ad[2], ad[1], ad[0], 0x00, 0x00, 0x00, no_of_bytes, checksum, 0xF7};
  if (VG99_MIDI_port == USBMIDI_PORT) usbMIDI.sendSysEx(17, sysexmessage);
  if (VG99_MIDI_port == MIDI1_PORT) MIDI1.sendSysEx(16, sysexmessage);
  if (VG99_MIDI_port == MIDI2_PORT) MIDI2.sendSysEx(16, sysexmessage);
  if (VG99_MIDI_port == MIDI3_PORT) MIDI2.sendSysEx(16, sysexmessage);
  debug_sysex(sysexmessage, 17, "out(VG99)");
}

void VG99_request_name()
{
  request_VG99(VG99_REQUEST_PATCHNAME);
}

void VG99_SendPatchChange(uint16_t new_patch) {
  //if (new_patch == VG99_patch_number) VG99_unmute();
  VG99_patch_number = new_patch;
  Serial.println("Send Patch Change (VG99):" + String(VG99_patch_number));
  //write_VG99fc(FC300_NORMAL_MODE);
  VG99_SendProgramChange();
  //write_VG99(VG99_PATCH_CHANGE, VG99_patch_number / 128, VG99_patch_number % 128);
  VG99_do_after_patch_selection();
}

void VG99_SendProgramChange() {
  if (VG99_MIDI_port == USBMIDI_PORT) {
    usbMIDI.sendControlChange(0 , VG99_patch_number / 100, VG99_MIDI_channel); // First send the bank number to CC00
    usbMIDI.sendProgramChange(VG99_patch_number % 100, VG99_MIDI_channel);
  }
  if (VG99_MIDI_port == MIDI1_PORT) {
    MIDI1.sendControlChange(0 , VG99_patch_number / 100, VG99_MIDI_channel); // First send the bank number to CC00
    MIDI1.sendProgramChange(VG99_patch_number % 100, VG99_MIDI_channel);
  }
  if (VG99_MIDI_port == MIDI2_PORT) {
    MIDI2.sendControlChange(0 , VG99_patch_number / 100, VG99_MIDI_channel); // First send the bank number to CC00
    MIDI2.sendProgramChange(VG99_patch_number % 100, VG99_MIDI_channel);
  }
  if (VG99_MIDI_port == MIDI3_PORT) {
    MIDI3.sendControlChange(0 , VG99_patch_number / 100, VG99_MIDI_channel); // First send the bank number to CC00
    MIDI3.sendProgramChange(VG99_patch_number % 100, VG99_MIDI_channel);
  }
}

void VG99_do_after_patch_selection() {
  no_device_check = true; // disables checking for devices during reading of GR_55
  VG99_current_assign = 255; // In case we were still in the middle of a previous patch change, switch off the lot
  VG99_request_onoff = false;
  VG99_sysex_watchdog_running = false;
  
  GP10_mute();
  GR55_mute();
  //VG99_request_guitar_switch_states();
  if (SEND_GLOBAL_TEMPO_AFTER_PATCH_CHANGE == true) VG99_send_bpm();
  //if (VG99_FC300_mode == false)
  VG99_request_name();
  Request_FC300_CTL_first_assign(); // Make sure this starts after VG99_request-guitar_onoff_state is done - they often ask for the same info - switches off now
  update_LEDS = true;
  update_lcd = true;
  EEPROM.write(EEPROM_VG99_PATCH_MSB, (VG99_patch_number / 256));
  EEPROM.write(EEPROM_VG99_PATCH_LSB, (VG99_patch_number % 256));
}

void VG99_send_bpm() {
  write_VG99(VG99_TEMPO, (bpm - 40) / 128, (bpm - 40) % 128); // Tempo is modulus 128 on the VG99. And sending 0 gives tempo 40.
}

void VG99_TAP_TEMPO_LED_ON() {
  if ((VG99_TAP_TEMPO_LED_CC > 0) && (VG99_detected)) {
    if (VG99_MIDI_port == USBMIDI_PORT) usbMIDI.sendControlChange(VG99_TAP_TEMPO_LED_CC , 127, VG99_MIDI_channel);
    if (VG99_MIDI_port == MIDI1_PORT) MIDI1.sendControlChange(VG99_TAP_TEMPO_LED_CC , 127, VG99_MIDI_channel);
    if (VG99_MIDI_port == MIDI2_PORT) MIDI2.sendControlChange(VG99_TAP_TEMPO_LED_CC , 127, VG99_MIDI_channel);
    if (VG99_MIDI_port == MIDI3_PORT) MIDI2.sendControlChange(VG99_TAP_TEMPO_LED_CC , 127, VG99_MIDI_channel);
  }
}

void VG99_TAP_TEMPO_LED_OFF() {
  if ((VG99_TAP_TEMPO_LED_CC > 0) && (VG99_detected)) {
    if (VG99_MIDI_port == USBMIDI_PORT) usbMIDI.sendControlChange(VG99_TAP_TEMPO_LED_CC , 0, VG99_MIDI_channel);
    if (VG99_MIDI_port == MIDI1_PORT) MIDI1.sendControlChange(VG99_TAP_TEMPO_LED_CC , 0, VG99_MIDI_channel);
    if (VG99_MIDI_port == MIDI2_PORT) MIDI2.sendControlChange(VG99_TAP_TEMPO_LED_CC , 0, VG99_MIDI_channel);
    if (VG99_MIDI_port == MIDI3_PORT) MIDI2.sendControlChange(VG99_TAP_TEMPO_LED_CC , 0, VG99_MIDI_channel);
  }
}
// ********************************* Section 4: VG99 controller functions ***********************************************

// ** US-20 simulation
// Selecting and muting the VG99 is done by storing the settings of COSM guitar A switch and COSM guitar B switch
// and switching both off when guitar is muted and back to original state when the VG99 is selected


void VG99_request_guitar_switch_states() {
  VG99_select_LED = VG99_PATCH_COLOUR; //Switch the LED on
  request_VG99(VG99_COSM_GUITAR_A_SW, 1);
  request_VG99(VG99_COSM_GUITAR_B_SW, 1);
  VG99_request_onoff = true;
}

void VG99_check_guitar_switch_states(const unsigned char* sxdata, short unsigned int sxlength) {
  if (VG99_request_onoff == true) {
    uint32_t address = (sxdata[7] << 24) + (sxdata[8] << 16) + (sxdata[9] << 8) + sxdata[10]; // Make the address 32 bit
    if (address == VG99_COSM_GUITAR_A_SW) {
      VG99_COSM_A_onoff = sxdata[11];  // Store the value
    }

    if (address == VG99_COSM_GUITAR_B_SW) {
      VG99_COSM_B_onoff = sxdata[11];  // Store the value
      VG99_request_onoff = false;
      //Request_FC300_CTL_first_assign(); // Now request the assigns...
    }
  }
}

void VG99_select_switch() {
  if (VG99_select_LED == VG99_PATCH_COLOUR) {
    VG99_always_on_toggle();
  }
  else {
    VG99_unmute();
    GP10_mute();
    GR55_mute();
    show_status_message(VG99_patch_name); // Show the correct patch name
  }
}

void VG99_always_on_toggle() {
  VG99_always_on = !VG99_always_on; // Toggle VG99_always_on
  if (VG99_always_on) {
    VG99_unmute();
    show_status_message("VG99 always ON");
  }
  else {
    VG99_mute();
    show_status_message("VG99 can be muted");
  }
}

void VG99_unmute() {
  VG99_select_LED = VG99_PATCH_COLOUR; //Switch the LED on
  //write_VG99(VG99_COSM_GUITAR_A_SW, VG99_COSM_A_onoff); // Switch COSM guitar on
  //write_VG99(VG99_COSM_GUITAR_B_SW, VG99_COSM_B_onoff); // Switch normal pu on
  VG99_SendProgramChange(); //Just sending the program change will put the sound back on
}

void VG99_mute() {
  if (VG99_always_on == false) {
    VG99_select_LED = VG99_OFF_COLOUR; //Switch the LED off
    write_VG99(VG99_COSM_GUITAR_A_SW, 0x00); // Switch COSM guitar off
    write_VG99(VG99_COSM_GUITAR_B_SW, 0x00); // Switch normal pu off
  }
}

// VG99 FC300 CTL-1 - CTL-8 control

struct FC300_CTL {       // Datastructure for FC300 controllers:
  uint16_t address;      // Contains the FC300 sysex address of the CTL pedal
  char name[5];         // Contains the name of the pedal
  uint8_t colour_on;     // Colour when the pedal is on
  uint8_t colour_off;    // Colour when the pedal is off
  uint8_t LED;           // Actual LED displaying one of the previous colours
  uint32_t assign_addr;  // The address of the assign in the VG99
  bool assign_on;        // Assign: on/off
  uint16_t assign_target;// Assign: target
  uint16_t assign_min;   // Assign: min-value (switch is off)
  uint16_t assign_max;   // Assign: max_value (switch is on)
  bool assign_latch;     // Assign: momentary (false) or latch (true)
  uint8_t target_byte1;  // Once the assign target is known, the state of the target is read into two bytes
  uint8_t target_byte2;  // This byte often contains the type of the assign - which we exploit in the part of parameter feedback
};

// CTL-1 to CTL-8 addresses - send 0x7F when pressed and 0x00 when released
#define FC300_CTL1 0x2100
#define FC300_CTL2 0x2101
#define FC300_CTL3 0x2402
#define FC300_CTL4 0x2102
#define FC300_CTL5 0x2403
#define FC300_CTL6 0x2103
#define FC300_CTL7 0x2404
#define FC300_CTL8 0x2104

// FC300 CTL-1 to CTL-8 assignments in the VG99
#define VG99_FC300_CTL1_assign 0x60000600
#define VG99_FC300_CTL2_assign 0x60000614
#define VG99_FC300_CTL3_assign 0x60000628
#define VG99_FC300_CTL4_assign 0x6000063C
#define VG99_FC300_CTL5_assign 0x60000650
#define VG99_FC300_CTL6_assign 0x60000664
#define VG99_FC300_CTL7_assign 0x60000678
#define VG99_FC300_CTL8_assign 0x6000070C

#define FC300_NUMBER_OF_CTLS 8
FC300_CTL FC300_ctls[FC300_NUMBER_OF_CTLS] = {
  {FC300_CTL1, "CTL1", VG99_STOMP_COLOUR_ON, VG99_STOMP_COLOUR_OFF, 0, VG99_FC300_CTL1_assign, false, 0, 0, 0, false, 0, 0},
  {FC300_CTL2, "CTL2", VG99_STOMP_COLOUR_ON, VG99_STOMP_COLOUR_OFF, 0, VG99_FC300_CTL2_assign, false, 0, 0, 0, false, 0, 0},
  {FC300_CTL3, "CTL3", VG99_STOMP_COLOUR_ON, VG99_STOMP_COLOUR_OFF, 0, VG99_FC300_CTL3_assign, false, 0, 0, 0, false, 0, 0},
  {FC300_CTL4, "CTL4", VG99_STOMP_COLOUR_ON, VG99_STOMP_COLOUR_OFF, 0, VG99_FC300_CTL4_assign, false, 0, 0, 0, false, 0, 0},
  {FC300_CTL5, "CTL5", VG99_STOMP_COLOUR_ON, VG99_STOMP_COLOUR_OFF, 0, VG99_FC300_CTL5_assign, false, 0, 0, 0, false, 0, 0},
  {FC300_CTL6, "CTL6", VG99_STOMP_COLOUR_ON, VG99_STOMP_COLOUR_OFF, 0, VG99_FC300_CTL6_assign, false, 0, 0, 0, false, 0, 0},
  {FC300_CTL7, "CTL7", VG99_STOMP_COLOUR_ON, VG99_STOMP_COLOUR_OFF, 0, VG99_FC300_CTL7_assign, false, 0, 0, 0, false, 0, 0},
  {FC300_CTL8, "CTL8", VG99_STOMP_COLOUR_ON, VG99_STOMP_COLOUR_OFF, 0, VG99_FC300_CTL8_assign, false, 0, 0, 0, false, 0, 0}
};

void FC300_stomp_press(uint8_t number) {

  // Press the FC300 pedal via sysex
  write_VG99fc(FC300_ctls[number].address, 0x7F);
  //Serial.println("You pressed: "+String(number));
  // Toggle LED status
  if (FC300_ctls[number].LED == FC300_ctls[number].colour_on) FC300_ctls[number].LED = FC300_ctls[number].colour_off;
  else FC300_ctls[number].LED = FC300_ctls[number].colour_on;

  // Display the patch function if the pedal is on
  if (FC300_ctls[number].assign_on) {
    VG99_display_parameter(FC300_ctls[number].assign_target, FC300_ctls[number].target_byte2); // Start parameter feedback - target_byte2 contains in some cases the number of the FX type
  }
  else {
    String msg = FC300_ctls[number].name;
    show_status_message(msg + " is off");
  }
}

void FC300_stomp_release(uint8_t number) {

  // Release the FC300 pedal via sysex
  write_VG99fc(FC300_ctls[number].address, 0x00);

  if (FC300_ctls[number].assign_latch == false) {
    if (FC300_ctls[number].assign_on == true) FC300_ctls[number].LED = FC300_ctls[number].colour_off; // Switch the LED off with the GP10 stomp colour
    else FC300_ctls[number].LED = 0; //Or if the assign is not set, switch the LED off
  }
}

void VG99_fix_reverse_pedals() {
  // Pedal 3,5 and 7 are reversed. But by operating them once the VG99 remembers the settings
  write_VG99fc(FC300_CTL3, 0x7F); // Press CTL-3
  write_VG99fc(FC300_CTL3, 0x00); // Release CTL-3
  write_VG99fc(FC300_CTL5, 0x7F); // Press CTL-5
  write_VG99fc(FC300_CTL5, 0x00); // Release CTL-5
  write_VG99fc(FC300_CTL7, 0x7F); // Press CTL-7
  write_VG99fc(FC300_CTL7, 0x00); // Release CTL-7

}

// Reading of the assigns - to avoid MIDI buffer overruns in the VG99, the assigns are read one by one
// 1. Request_FC300_CTL_first_assign() starts this process. It resets the VG99_current_assign and starts the loop
// 2. Request_FC300_CTL_current_assign() puts a request out to the VG99 for the next FC300 CTL button assign.
// 3. read_FC300_CTL_assigns() will receive the settings of the CTL button assign and store it in the FC300_ctrls array.
//    It will then request the VG99 for the state of the target of the VG99 - always requesting two bytes
// 4. read_FC300_CTL_assigns() will receive the setting of the CTL assign target and store it in the FC300_ctrls array and update the LED of the assign.
//    It will then update VG99_current_assign and request the next assign - which brings us back to step 2.

void Request_FC300_CTL_first_assign() {
  VG99_current_assign = 0; //After the name is read, the assigns can be read
  Serial.println("Start reading FC300 CTL assigns");
  VG99_set_sysex_watchdog();
  Request_FC300_CTL_current_assign();
}

void Request_FC300_CTL_current_assign() { //Will request the next assign - the assigns are read one by one, otherwise the data will not arrive!
  if (VG99_current_assign < FC300_NUMBER_OF_CTLS) {
    Serial.println("Request VG99_current_assign=" + String(VG99_current_assign));
    request_VG99(FC300_ctls[VG99_current_assign].assign_addr, 8); // Request 8 bytes for the assign
    VG99_set_sysex_watchdog(); // Set the timer
  }
  else {
    VG99_sysex_watchdog_running = false; // Stop the timer
    no_device_check = false;
  }
}

void VG99_set_sysex_watchdog() {
  VG99sysexWatchdog = millis() + VG99_SYSEX_WATCHDOG_LENGTH;
  VG99_sysex_watchdog_running = true;
  Serial.println("VG99 sysex watchdog started");
}

void VG99_check_sysex_watchdog() {
  if ((millis() > VG99sysexWatchdog) && (VG99_sysex_watchdog_running)) {
    Serial.println("VG99 sysex watchdog expired");
    Request_FC300_CTL_current_assign(); // Try reading the assign again
  }
}

void read_FC300_CTL_assigns(const unsigned char* sxdata, short unsigned int sxlength) {
  uint32_t address = (sxdata[7] << 24) + (sxdata[8] << 16) + (sxdata[9] << 8) + sxdata[10]; // Make the address 32 bit

  if (VG99_current_assign < FC300_NUMBER_OF_CTLS) { // Check if we have not reached the last assign.
    // Check if it is the CTL assign address
    if ((sxdata[6] == 0x12) && (address == FC300_ctls[VG99_current_assign].assign_addr)) {
      //Serial.println("Received VG99_current_assign=" + String(VG99_current_assign));
      // Copy the data into the FC300 assign array
      FC300_ctls[VG99_current_assign].assign_on = (sxdata[11] == 0x01);
      FC300_ctls[VG99_current_assign].assign_target = (sxdata[12] * 256) + sxdata[13];
      FC300_ctls[VG99_current_assign].assign_min = (sxdata[14] * 256) + sxdata[15];
      FC300_ctls[VG99_current_assign].assign_max = (sxdata[16] * 256) + sxdata[17];
      FC300_ctls[VG99_current_assign].assign_latch = (sxdata[18] == 0x01);

      //Serial.println("Assign_on:" + String(FC300_ctls[VG99_current_assign].assign_on));
      //Serial.println("Assign_target:" + String(FC300_ctls[VG99_current_assign].assign_target, HEX));

      //Request the status of the target from the patch temporary area is assignment is on
      if (FC300_ctls[VG99_current_assign].assign_on == true) {
        Serial.println("Request target of assign " + String(VG99_current_assign) + ": " + String(FC300_ctls[VG99_current_assign].assign_target, HEX));
        request_VG99((0x60000000 + FC300_ctls[VG99_current_assign].assign_target), 2);
      }
      else {
        FC300_ctls[VG99_current_assign].LED = 0; // Switch the LED off
        FC300_ctls[VG99_current_assign].colour_on = VG99_STOMP_COLOUR_ON; // Set the on colour to default
        FC300_ctls[VG99_current_assign].colour_off = 0; // Set the off colour to LED off
        //Serial.println("Current LED off for VG99_current_assign=" + String(VG99_current_assign));
        VG99_current_assign++; // Select the next assign
        Request_FC300_CTL_current_assign(); //Request the next assign
      }

    }

    // Check if it is the requested data for the previous controller
    uint32_t requested_address = 0x60000000 + FC300_ctls[VG99_current_assign].assign_target;
    if ((sxdata[6] == 0x12) && (address == requested_address) && (FC300_ctls[VG99_current_assign].assign_target != 0)) {

      Serial.println("Target received of assign " + String(VG99_current_assign) + ": " + String(FC300_ctls[VG99_current_assign].assign_target, HEX));

      // Write the received bytes in the array
      FC300_ctls[VG99_current_assign].target_byte1 = sxdata[11];
      FC300_ctls[VG99_current_assign].target_byte2 = sxdata[12];

      // Set the colour_on and colour_off values
      VG99_find_colours(VG99_current_assign);
      //FC300_ctls[VG99_current_assign].colour_on = VG99_find_colour_on(FC300_ctls[VG99_current_assign].assign_target, FC300_ctls[VG99_current_assign].target_byte2);
      //FC300_ctls[VG99_current_assign].colour_off = VG99_find_colour_off(FC300_ctls[VG99_current_assign].assign_target, FC300_ctls[VG99_current_assign].target_byte2);


      // Set the LED status for this ctl pedal
      if (FC300_ctls[VG99_current_assign].target_byte1 == FC300_ctls[VG99_current_assign].assign_max) {
        FC300_ctls[VG99_current_assign].LED = FC300_ctls[VG99_current_assign].colour_on;
      }
      else {
        FC300_ctls[VG99_current_assign].LED = FC300_ctls[VG99_current_assign].colour_off;
      }
      update_LEDS = true;
      VG99_current_assign++; // Select the next assign
      Request_FC300_CTL_current_assign(); //Request the next assign
    }
  }
}

// ********************************* Section 5: VG99 controller parameter feedback ***********************************************
// Names of parameters are stored here into program memory. This section contains only one function.

struct Parameter {  // Datastructure for parameters:
  uint16_t address; // Address of the assign
  char name[17];    // Name of the assign that will appear on the display
  uint8_t sublist;  // Number of the sublist that exists for this parameter
  uint8_t colour_on; // Colour of the LED when this parameter is on
  uint8_t colour_off; // Colour of the LED when this parameter is off
};

// Parameter feedback
// First we list the most common parameters by (partial) address and name
// If the last number is 0, there is no sublist
// If there is a number, it will refer to the sublist as set in the VG99_sublists array

#define VG99_FX_COLOUR_ON 254 // Just a colour number to pick the colour from the VG99_FX_colours table
#define VG99_FX_COLOUR_OFF 255 // Just a colour number to pick the colour from the VG99_FX_colours table
#define VG99_POLYFX_COLOUR_ON 252 // Just a colour number to pick the colour from the VG99_polyFX_colours table
#define VG99_POLYFX_COLOUR_OFF 253 // Just a colour number to pick the colour from the VG99_polyFX_colours table

//From the "Control assign target" table on page 59 - 70 of the VG99 MIDI impementation guide
#define VG99_PARAMETERS_PARTS 8
#define VG99_PARAMETERS_SIZE 28
const PROGMEM Parameter VG99_parameters[VG99_PARAMETERS_PARTS][VG99_PARAMETERS_SIZE] = {
  { // 0000 - 1000 Tunings
    {0x0017, "KEY", 0, FX_PITCH_COLOUR_ON, FX_PITCH_COLOUR_OFF},
    {0x0015, "BPM", 0, FX_DELAY_COLOUR_ON, FX_DELAY_COLOUR_OFF},
    {0x0024, "FC AMP CTL1", 0, FX_AMP_COLOUR_ON, FX_AMP_COLOUR_OFF},
    {0x0025, "FC AMP CTL2", 0, FX_AMP_COLOUR_ON, FX_AMP_COLOUR_OFF},
    {0x0D1E, "D BM SELECT", 0, FX_PITCH_COLOUR_ON, FX_PITCH_COLOUR_OFF},
    {0x0D1F, "D BM PITCH TYP", 0, FX_PITCH_COLOUR_ON, FX_PITCH_COLOUR_OFF},
    {0x0D20, "D BM T-ARM CH", 0, FX_PITCH_COLOUR_ON, FX_PITCH_COLOUR_OFF},
    {0x0D21, "D BM T-ARM TYP", 0, FX_PITCH_COLOUR_ON, FX_PITCH_COLOUR_OFF},
    {0x0D26, "D BM FREEZE CH", 0, VG99_STOMP_COLOUR_ON, VG99_STOMP_COLOUR_OFF},
    {0x0D29, "D BM FRZ(A) LVL", 0, VG99_STOMP_COLOUR_ON, VG99_STOMP_COLOUR_OFF},
    {0x0D2D, "D BM FRZ(B) LVL", 0, VG99_STOMP_COLOUR_ON, VG99_STOMP_COLOUR_OFF},
    {0x0D2F, "D BM FILTER CH", 0, FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF},
    {0x0D30, "D BM FLTR TYPE", 0, FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF},
    {0x0D34, "D BM FILTR LVL", 0, FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF},
    {0x0D35, "RIBBON SELECT", 0, VG99_STOMP_COLOUR_ON, VG99_STOMP_COLOUR_OFF},
    {0x0D36, "RBBN T-ARM CH", 0, FX_PITCH_COLOUR_ON, FX_PITCH_COLOUR_OFF},
    {0x0D37, "RBBN T-ARM TYPE", 0, FX_PITCH_COLOUR_ON, FX_PITCH_COLOUR_OFF},
    {0x0D3C, "RBBN FILTER CH", 0, FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF},
    {0x0D3D, "RBBN FILTER TYP", 0, FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF},
    {0x0D41, "RBBN FILTER LVL", 0, FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF}
  },

  { // 1000 - 2000 Alt tuning parameters
    {0x1001, "[A]TU", 5, FX_PITCH_COLOUR_ON, FX_PITCH_COLOUR_OFF},
    {0x1002, "[A]TUNING TYPE", 0, FX_PITCH_COLOUR_ON, FX_PITCH_COLOUR_OFF},
    {0x1007, "[A]BEND SW", 0, FX_PITCH_COLOUR_ON, FX_PITCH_COLOUR_OFF},
    {0x1004, "[A]12STRING SW", 0, FX_PITCH_COLOUR_ON, FX_PITCH_COLOUR_OFF},
    {0x1003, "[A]DETUNE SW", 0, FX_PITCH_COLOUR_ON, FX_PITCH_COLOUR_OFF},
    {0x1005, "[A]HMNY", 3, FX_PITCH_COLOUR_ON, FX_PITCH_COLOUR_OFF},
    {0x1006, "[A]HARMO", 0, FX_PITCH_COLOUR_ON, FX_PITCH_COLOUR_OFF},
    {0x1009, "[B]TU", 5, FX_PITCH_COLOUR_ON, FX_PITCH_COLOUR_OFF},
    {0x100A, "[B]TUNING TYPE", 0, FX_PITCH_COLOUR_ON, FX_PITCH_COLOUR_OFF},
    {0x100F, "[B]BEND SW", 0, FX_PITCH_COLOUR_ON, FX_PITCH_COLOUR_OFF},
    {0x100C, "[B]12STRING SW", 0, FX_PITCH_COLOUR_ON, FX_PITCH_COLOUR_OFF},
    {0x100B, "[B]DETUNE SW", 0, FX_PITCH_COLOUR_ON, FX_PITCH_COLOUR_OFF},
    {0x100D, "[B]HMNY", 3, FX_PITCH_COLOUR_ON, FX_PITCH_COLOUR_OFF},
    {0x100E, "[B]HARMO", 0, FX_PITCH_COLOUR_ON, FX_PITCH_COLOUR_OFF}
  },

  { // 2000 - 3000 Common parameters
    {0x2007, "DYNA SW", 0, VG99_STOMP_COLOUR_ON, VG99_STOMP_COLOUR_OFF},
    {0x2008, "DYNA TYPE", 0, VG99_STOMP_COLOUR_ON, VG99_STOMP_COLOUR_OFF},
    {0x2014, "[A]MIXER PAN", 0, VG99_STOMP_COLOUR_ON, VG99_STOMP_COLOUR_OFF},
    {0x2015, "[A]MIXER LVL", 0, VG99_STOMP_COLOUR_ON, VG99_STOMP_COLOUR_OFF},
    {0x2019, "[A]MIX SW", 0, VG99_STOMP_COLOUR_ON, VG99_STOMP_COLOUR_OFF},
    {0x201A, "[B]MIXER PAN", 0, VG99_STOMP_COLOUR_ON, VG99_STOMP_COLOUR_OFF},
    {0x201B, "[B]MIXER LVL", 0, VG99_STOMP_COLOUR_ON, VG99_STOMP_COLOUR_OFF},
    {0x201F, "[B]MIX SW", 0, VG99_STOMP_COLOUR_ON, VG99_STOMP_COLOUR_OFF},
    {0x202F, "TOTAL EQ SW", 0, VG99_STOMP_COLOUR_ON, VG99_STOMP_COLOUR_OFF},
    {0x2013, "A/B BAL", 0, VG99_STOMP_COLOUR_ON, VG99_STOMP_COLOUR_OFF},
    {0x2000, "PATCH LEVEL", 0, VG99_STOMP_COLOUR_ON, VG99_STOMP_COLOUR_OFF},
    {0x2020, "D/R REVERB SW", 0, FX_REVERB_COLOUR_ON, FX_REVERB_COLOUR_OFF},
    {0x2021, "D/R REVRB TYPE", 0, FX_REVERB_COLOUR_ON, FX_REVERB_COLOUR_OFF},
    {0x2022, "D/R REVRB TIME", 0, FX_REVERB_COLOUR_ON, FX_REVERB_COLOUR_OFF},
    {0x2023, "D/R RVRB PREDY", 0, FX_REVERB_COLOUR_ON, FX_REVERB_COLOUR_OFF},
    {0x2024, "D/R RVRB LWCUT", 0, FX_REVERB_COLOUR_ON, FX_REVERB_COLOUR_OFF},
    {0x2025, "D/R RVRB HICUT", 0, FX_REVERB_COLOUR_ON, FX_REVERB_COLOUR_OFF},
    {0x2026, "D/R REVRB DENS", 0, FX_REVERB_COLOUR_ON, FX_REVERB_COLOUR_OFF},
    {0x2027, "D/R REVERB LVL", 0, FX_REVERB_COLOUR_ON, FX_REVERB_COLOUR_OFF},
    {0x2028, "D/R DELAY SW", 0, FX_DELAY_COLOUR_ON, FX_DELAY_COLOUR_OFF},
    {0x2029, "D/R DELAY TIME", 0, FX_DELAY_COLOUR_ON, FX_DELAY_COLOUR_OFF},
    {0x202B, "D/R DLAY FDBCK", 0, FX_DELAY_COLOUR_ON, FX_DELAY_COLOUR_OFF},
    {0x202C, "D/R DLAY HICUT", 0, FX_DELAY_COLOUR_ON, FX_DELAY_COLOUR_OFF},
    {0x202D, "D/R DLAY LEVEL", 0, FX_DELAY_COLOUR_ON, FX_DELAY_COLOUR_OFF}
  },

  { // 3000 - 4000 Guitar parameters
    {0x3000, "[A]COSM GTR SW", 0, FX_GTR_COLOUR_ON, FX_GTR_COLOUR_OFF},
    {0x3001, "[A]MODEL TYPE", 0, FX_GTR_COLOUR_ON, FX_GTR_COLOUR_OFF},
    {0x301B, "[A]E.GTR TYPE", 0, FX_GTR_COLOUR_ON, FX_GTR_COLOUR_OFF},
    {0x301E, "[A]PU SEL", 0, FX_GTR_COLOUR_ON, FX_GTR_COLOUR_OFF},
    {0x301F, "[A]E.GTR VOL", 0, FX_GTR_COLOUR_ON, FX_GTR_COLOUR_OFF},
    {0x3022, "[A]E.GTR TONE", 0, FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF},
    {0x3045, "[A]AC TYPE", 0, FX_GTR_COLOUR_ON, FX_GTR_COLOUR_OFF},
    {0x3046, "[A]BODY TYPE", 0, FX_GTR_COLOUR_ON, FX_GTR_COLOUR_OFF},
    {0x3069, "[A]BASS TYPE", 0, FX_GTR_COLOUR_ON, FX_GTR_COLOUR_OFF},
    {0x3400, "[A]SYNTH TYPE", 0, FX_GTR_COLOUR_ON, FX_GTR_COLOUR_OFF},
    {0x3002, "[A]GTR EQ SW", 0, FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF},
    {0x3018, "[A]COSM LEVEL", 0, FX_GTR_COLOUR_ON, FX_GTR_COLOUR_OFF},
    {0x301A, "[A]NORM PU LEVEL", 0, FX_GTR_COLOUR_ON, FX_GTR_COLOUR_OFF},
    {0x3457, "[A]NS SW", 0, FX_GTR_COLOUR_ON, FX_GTR_COLOUR_OFF},
    {0x3800, "[B]COSM GTR SW", 0, FX_GTR_COLOUR_ON, FX_GTR_COLOUR_OFF},
    {0x3801, "[B]MODEL TYPE", 0, FX_GTR_COLOUR_ON, FX_GTR_COLOUR_OFF},
    {0x381B, "[B]E.GTR TYPE", 0, FX_GTR_COLOUR_ON, FX_GTR_COLOUR_OFF},
    {0x381E, "[B]PU SEL", 0, FX_GTR_COLOUR_ON, FX_GTR_COLOUR_OFF},
    {0x381F, "[B]E.GTR VOL", 0, FX_GTR_COLOUR_ON, FX_GTR_COLOUR_OFF},
    {0x3822, "[B]E.GTR TONE", 0, FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF},
    {0x3845, "[B]AC TYPE", 0, FX_GTR_COLOUR_ON, FX_GTR_COLOUR_OFF},
    {0x3846, "[B]BODY TYPE", 0, FX_GTR_COLOUR_ON, FX_GTR_COLOUR_OFF},
    {0x3869, "[B]BASS TYPE", 0, FX_GTR_COLOUR_ON, FX_GTR_COLOUR_OFF},
    {0x3C00, "[B]SYNTH TYPE", 0, FX_GTR_COLOUR_ON, FX_GTR_COLOUR_OFF},
    {0x3802, "[B]GTR EQ SW", 0, FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF},
    {0x3818, "[B]COSM LEVEL", 0, FX_GTR_COLOUR_ON, FX_GTR_COLOUR_OFF},
    {0x381A, "[B]NORM PU LEVEL", 0, FX_GTR_COLOUR_ON, FX_GTR_COLOUR_OFF},
    {0x3C57, "[B]NS SW", 0, FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF}
  },

  { // 4000 - 5000 Poly FX
    {0x4000, "POLY FX CHAN", 0, FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF},
    {0x4001, "POLY", 6, VG99_POLYFX_COLOUR_ON, VG99_POLYFX_COLOUR_OFF},
    {0x4002, "POLY FX TYPE", 0, FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF},
    {0x4003, "POLY COMP TYPE", 0, FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF},
    {0x4004, "POLY COMP SUSTN", 0, FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF},
    {0x4005, "POLY COMP ATTACK", 0, FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF},
    {0x4006, "POLY COMP THRSH", 0, FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF},
    {0x4007, "POLY COMP REL", 0, FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF},
    {0x4008, "POLY COMP TONE", 0, FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF},
    {0x4009, "POLY COMP LEVEL", 0, FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF},
    {0x400A, "POLY COMP BAL", 0, FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF},
    {0x400B, "POLY DIST MODE", 0, FX_DIST_COLOUR_ON, FX_DIST_COLOUR_OFF},
    {0x400C, "POLY DIST DRIVE", 0, FX_DIST_COLOUR_ON, FX_DIST_COLOUR_OFF},
    {0x400D, "POLY D HIGH CUT", 0, FX_DIST_COLOUR_ON, FX_DIST_COLOUR_OFF},
    {0x400E, "POLY D POLY BAL", 0, FX_DIST_COLOUR_ON, FX_DIST_COLOUR_OFF},
    {0x400F, "POLY D DRIVE BAL", 0, FX_DIST_COLOUR_ON, FX_DIST_COLOUR_OFF},
    {0x4010, "POLY DIST LEVEL", 0, FX_DIST_COLOUR_ON, FX_DIST_COLOUR_OFF},
    {0x402F, "POLY SG RISETIME", 0, FX_MODULATE_COLOUR_ON, FX_MODULATE_COLOUR_OFF},
    {0x4030, "POLY SG SENS", 0, FX_MODULATE_COLOUR_ON, FX_MODULATE_COLOUR_OFF},
  },

  { // 5000 - 6000 FX and amps chain A
    {0x502B, "[A]COMP SW", 0, FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF},
    {0x502C, "[A]COMP TYPE", 0, FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF},
    {0x5033, "[A]OD", 1, FX_DIST_COLOUR_ON, FX_DIST_COLOUR_OFF}, // Check VG99_odds_type table (1)
    {0x5034, "[A]OD / DS TYPE", 0, FX_DIST_COLOUR_ON, FX_DIST_COLOUR_OFF},
    {0x503F, "[A]WAH SW", 0, FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF},
    {0x5040, "[A]WAH TYPE", 0, FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF},
    {0x5048, "[A]EQ SW", 0, FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF},
    {0x5054, "[A]DELAY SW", 0, FX_DELAY_COLOUR_ON, FX_DELAY_COLOUR_OFF},
    {0x5055, "[A]DELAY TYPE", 0, FX_DELAY_COLOUR_ON, FX_DELAY_COLOUR_OFF},
    {0x506D, "[A]CHORUS SW", 0, FX_MODULATE_COLOUR_ON, FX_MODULATE_COLOUR_OFF},
    {0x506E, "[A]CHORUS MODE", 0, FX_MODULATE_COLOUR_ON, FX_MODULATE_COLOUR_OFF},
    {0x5075, "[A]REVERB SW", 0, FX_REVERB_COLOUR_ON, FX_REVERB_COLOUR_OFF},
    {0x5076, "[A]REVERB TYPE", 0, FX_REVERB_COLOUR_ON, FX_REVERB_COLOUR_OFF},
    {0x5400, "[A]M1", 2, VG99_FX_COLOUR_ON, VG99_FX_COLOUR_OFF}, // Check VG99_mod_type table (2)
    {0x5401, "[A]MOD1 TYPE", 0, VG99_STOMP_COLOUR_ON, VG99_STOMP_COLOUR_OFF},
    {0x5800, "[A]M2", 2, VG99_FX_COLOUR_ON, VG99_FX_COLOUR_OFF}, // Check VG99_mod_type table (2)
    {0x5801, "[A]MOD2 TYPE", 0, VG99_STOMP_COLOUR_ON, VG99_STOMP_COLOUR_OFF},
    {0x507E, "[A]NS SW", 0, FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF},
    {0x500D, "[A]AMP", 4, FX_AMP_COLOUR_ON, FX_AMP_COLOUR_OFF},
    {0x500E, "[A]AMP TYPE", 0, FX_AMP_COLOUR_ON, FX_AMP_COLOUR_OFF},
    {0x500F, "[A]AMP GAIN", 0, FX_AMP_COLOUR_ON, FX_AMP_COLOUR_OFF},
    {0x5015, "[A]AMP BRIGHT", 0, FX_AMP_COLOUR_ON, FX_AMP_COLOUR_OFF},
    {0x5016, "[A]AMP GAIN SW", 0, FX_AMP_COLOUR_ON, FX_AMP_COLOUR_OFF},
    {0x5017, "[A]AMP(SOLO) SW", 0, FX_AMP_COLOUR_ON, FX_AMP_COLOUR_OFF},
    {0x5019, "[A]AMP SP TYPE", 0, FX_AMP_COLOUR_ON, FX_AMP_COLOUR_OFF}
  },

  { // 6000 - 7000 FX and amps chain B
    {0x602B, "[B]COMP SW", 0, FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF},
    {0x602C, "[B]COMP TYPE", 0, FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF},
    {0x6033, "[B]OD", 1, FX_DIST_COLOUR_ON, FX_DIST_COLOUR_OFF}, // Check VG99_odds_type table (1)
    {0x6034, "[B]OD / DS TYPE", 0, FX_DIST_COLOUR_ON, FX_DIST_COLOUR_OFF},
    {0x603F, "[B]WAH SW", 0, FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF},
    {0x6040, "[B]WAH TYPE", 0, FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF},
    {0x6048, "[B]EQ SW", 0, FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF},
    {0x6054, "[B]DELAY SW", 0, FX_DELAY_COLOUR_ON, FX_DELAY_COLOUR_OFF},
    {0x6055, "[B]DELAY TYPE", 0, FX_DELAY_COLOUR_ON, FX_DELAY_COLOUR_OFF},
    {0x606D, "[B]CHORUS SW", 0, FX_MODULATE_COLOUR_ON, FX_MODULATE_COLOUR_OFF},
    {0x606E, "[B]CHORUS MODE", 0, FX_MODULATE_COLOUR_ON, FX_MODULATE_COLOUR_OFF},
    {0x6075, "[A]REVERB SW", 0, FX_REVERB_COLOUR_ON, FX_REVERB_COLOUR_OFF},
    {0x6076, "[A]REVERB TYPE", 0, FX_REVERB_COLOUR_ON, FX_REVERB_COLOUR_OFF},
    {0x6400, "[B]M1", 2, VG99_FX_COLOUR_ON, VG99_FX_COLOUR_OFF}, // Check VG99_mod_type table (2)
    {0x6401, "[B]MOD1 TYPE", 0, VG99_STOMP_COLOUR_ON, VG99_STOMP_COLOUR_OFF},
    {0x6800, "[B]M2", 2, VG99_FX_COLOUR_ON, VG99_FX_COLOUR_OFF}, // Check VG99_mod_type table (2)
    {0x6801, "[B]MOD2 TYPE", 0, VG99_STOMP_COLOUR_ON, VG99_STOMP_COLOUR_OFF},
    {0x607E, "[B]NS SW", 0, FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF},
    {0x600D, "[B]AMP", 4, FX_AMP_COLOUR_ON, FX_AMP_COLOUR_OFF}, // Sublist amps
    {0x600E, "[B]AMP TYPE", 0, FX_AMP_COLOUR_ON, FX_AMP_COLOUR_OFF},
    {0x600F, "[B]AMP GAIN", 0, FX_AMP_COLOUR_ON, FX_AMP_COLOUR_OFF},
    {0x6015, "[B]AMP BRIGHT", 0, FX_AMP_COLOUR_ON, FX_AMP_COLOUR_OFF},
    {0x6016, "[B]AMP GAIN SW", 0, FX_AMP_COLOUR_ON, FX_AMP_COLOUR_OFF},
    {0x6017, "[B]AMP(SOLO) SW", 0, FX_AMP_COLOUR_ON, FX_AMP_COLOUR_OFF},
    {0x6019, "[B]AMP SP TYPE", 0, FX_AMP_COLOUR_ON, FX_AMP_COLOUR_OFF}
  },

  { // 7000 - 8000 Special functions
    {0x7600, "[A]BEND", 0, FX_PITCH_COLOUR_ON, FX_PITCH_COLOUR_OFF},
    {0x7601, "[B]BEND", 0, FX_PITCH_COLOUR_ON, FX_PITCH_COLOUR_OFF},
    {0x7602, "DB T-ARM CONTROL", 0, FX_PITCH_COLOUR_ON, FX_PITCH_COLOUR_OFF},
    {0x7603, "DB T-ARM SW", 0, FX_PITCH_COLOUR_ON, FX_PITCH_COLOUR_OFF},
    {0x7604, "DB FREEZE SW", 0, VG99_STOMP_COLOUR_ON, VG99_STOMP_COLOUR_OFF},
    {0x7606, "DB FILTER CONTRL", 0, FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF},
    {0x7607, "DB FILTER SW", 0, FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF},
    {0x7608, "RB T-ARM CONTROL", 0, FX_PITCH_COLOUR_ON, FX_PITCH_COLOUR_OFF},
    {0x7609, "RB T-ARM SW", 0, FX_PITCH_COLOUR_ON, FX_PITCH_COLOUR_OFF},
    {0x760A, "RB FILTER CONTRL", 0, FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF},
    {0x760B, "RB FILTER SW", 0, FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF},
    {0x760C, "[A]FX DLY REC", 0, FX_DELAY_COLOUR_ON, FX_DELAY_COLOUR_OFF},
    {0x760D, "[A]FX DLY STOP", 0, FX_DELAY_COLOUR_ON, FX_DELAY_COLOUR_OFF},
    {0x760E, "[B]FX DLY REC", 0, FX_DELAY_COLOUR_ON, FX_DELAY_COLOUR_OFF},
    {0x760F, "[B]FX DLY STOP", 0, FX_DELAY_COLOUR_ON, FX_DELAY_COLOUR_OFF},
    {0x7611, "BPM TAP", 0, FX_DELAY_COLOUR_ON, FX_DELAY_COLOUR_OFF},
    {0x7610, "V-LINK SW", 0, FX_AMP_COLOUR_ON, FX_AMP_COLOUR_OFF},
    {0x7F7F, "OFF", 0, 0, 0},
  }
};


#define VG99_NO_OF_SUBLISTS 7
#define VG99_SIZE_OF_SUBLISTS 49
const PROGMEM char VG99_sublists[VG99_NO_OF_SUBLISTS][VG99_SIZE_OF_SUBLISTS][8] = {
  // Sublist 0 left empty - not used
  { },

  // Sublist 1 from the "Control assign target" table on page 57 of the VG99 MIDI impementation guide
  { "BOOST", "BLUES", "CRUNCH", "NATURAL", "TURBO", "FAT OD", "OD - 1", "TSCREAM", "WARM OD", "DIST",
    "MILD DS", "DRIVE", "RAT", "GUV DS", "DST + ", "SOLID", "MID DS", "STACK", "MODERN", "POWER", "R - MAN",
    "METAL", "HVY MTL", "LEAD", "LOUD", "SHARP", "MECHA", "60 FUZZ", "OCTFUZZ", "BIGMUFF", "CUSTOM"
  },

  // Sublist 2 from the "FX mod type" table on page 71 of the VG99 MIDI impementation guide
  { "COMPRSR", "LIMITER", "T. WAH", "AUTOWAH", "T_WAH", "", "TREMOLO", "PHASER", //00 - 07
    "FLANGER", "PAN", "VIB", "UNI - V", "RINGMOD", "SLOW GR", "DEFRET", "", //08 - 0F
    "FEEDBKR", "ANTI FB", "HUMANZR", "SLICER", "", "SUB EQ", "HARMO", "PITCH S", //10 - 17
    "P. BEND", "OCTAVE", "ROTARY", "", "", "", "", "", //18 - 1F
    "S DELAY"
  },

  // Sublist 3 from the "Harmony" table on page 56 of the VG99 MIDI impementation guide
  { "-2oct", "-14th", "-13th", "-12th", "-11th", "-10th", "-9th",
    "-1oct", "-7th", "-6th", "-5th", "-4th", "-3rd", "-2nd", "TONIC",
    "+2nd", "+3rd", "+4th", "+5th", "+6th", "+7th", "+1oct", "+9th", "+10th", "+11th"
  },

  // Sublist 4 from the "COSM AMP" table on page 71 of the VG99 MIDI impementation guide
  { "JC-120", "JC WRM", "JC JZZ", "JC FLL", "JC BRT", "CL TWN", "PRO CR", "TWEED", "WRM CR", "CRUNCH",
    "BLUES", "WILD C", "C STCK", "VXDRIV", "VXLEAD", "VXCLN", "MTCH D", "MTCH F", "MTCH L", "BGLEAD",
    "BGDRIV", "BRHYTM", "SMOOTH", "BGMILD", "MS1959", "MS1959", "MS1959", "MS HI", "MS PWR", "RF-CLN",
    "RF-RAW", "RF-VT1", "RF-MN1", "RF-VT2", "RF-MN1", "T-CLN", "T-CNCH", "T-LEAD", "T-EDGE", "SLDANO",
    "HI-DRV", "HI-LD", "HI-HVY", "5150", "MODERN", "M LEAD", "CUSTOM", "BASS V", "BASS M"
  },

  // Sublist 5 from page 17 of the VG99 MIDI impementation guide
  { "OPEN-D", "OPEN-E", "OPEN-G", "OPEN-A", "DROP-D", "D-MODAL", "-1 STEP", "-2 STEP", "BARITON", "NASHVL", "-1 OCT", "+1 OCT", "USER" },

  // Sublist 6 for poly FX
  { "COMPR", "DISTORT", "OCTAVE", "SLOW GR"}
};

const PROGMEM uint8_t VG99_FX_colours[33][2] = {
  { FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF }, // Colour for "COMPRSR"
  { FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF }, // Colour for "LIMITER"
  { FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF }, // Colour for "T. WAH"
  { FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF }, // Colour for "AUTOWAH"
  { FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF }, // Colour for "T_WAH"
  { 0, 0 }, // Colour for ""
  { FX_MODULATE_COLOUR_ON, FX_MODULATE_COLOUR_OFF }, // Colour for "TREMOLO"
  { FX_MODULATE_COLOUR_ON, FX_MODULATE_COLOUR_OFF }, // Colour for "PHASER"
  { FX_MODULATE_COLOUR_ON, FX_MODULATE_COLOUR_OFF }, // Colour for "FLANGER"
  { FX_MODULATE_COLOUR_ON, FX_MODULATE_COLOUR_OFF }, // Colour for "PAN"
  { FX_MODULATE_COLOUR_ON, FX_MODULATE_COLOUR_OFF }, // Colour for "VIB"
  { FX_MODULATE_COLOUR_ON, FX_MODULATE_COLOUR_OFF }, // Colour for "UNI - V"
  { FX_MODULATE_COLOUR_ON, FX_MODULATE_COLOUR_OFF }, // Colour for "RINGMOD"
  { FX_MODULATE_COLOUR_ON, FX_MODULATE_COLOUR_OFF }, // Colour for "SLOW GR"
  { FX_DIST_COLOUR_ON, FX_DIST_COLOUR_OFF }, // Colour for "DEFRET"
  { 0, 0 }, // Colour for ""
  { FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF }, // Colour for "FEEDBKR"
  { FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF }, // Colour for "ANTI FB"
  { FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF }, // Colour for "HUMANZR"
  { FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF }, // Colour for "SLICER"
  { 0, 0 }, // Colour for ""
  { FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF }, // Colour for "SUB EQ"
  { FX_PITCH_COLOUR_ON, FX_PITCH_COLOUR_OFF }, // Colour for "HARMO"
  { FX_PITCH_COLOUR_ON, FX_PITCH_COLOUR_OFF }, // Colour for "PITCH S"
  { FX_PITCH_COLOUR_ON, FX_PITCH_COLOUR_OFF }, // Colour for "P. BEND"
  { FX_PITCH_COLOUR_ON, FX_PITCH_COLOUR_OFF }, // Colour for "OCTAVE"
  { FX_MODULATE_COLOUR_ON, FX_MODULATE_COLOUR_OFF }, // Colour for "ROTARY"
  { 0, 0 }, // Colour for ""
  { 0, 0 }, // Colour for ""
  { 0, 0 }, // Colour for ""
  { 0, 0 }, // Colour for ""
  { 0, 0 }, // Colour for ""
  { FX_DELAY_COLOUR_ON, FX_DELAY_COLOUR_OFF }, // Colour for "S DELAY"
};

const PROGMEM uint8_t VG99_polyFX_colours[4][2] = {
  { FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF }, // Colour for "COMPR"
  { FX_DIST_COLOUR_ON, FX_DIST_COLOUR_OFF }, // Colour for "DISTORT"
  { FX_PITCH_COLOUR_ON, FX_PITCH_COLOUR_OFF }, // Colour for "OCTAVE"
  { FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF }, // Colour for "SLOW GR"
};

// Will lookup parameter name in a table- for the fx1 and fx2 types it will also mention the type of effect.
void VG99_display_parameter(uint16_t address, uint8_t type) {
  String msg1, msg2;
  uint8_t part;
  uint8_t sublist;

  part = (address / 0x1000); // As the array is divided in addresses by 1000, it is easy to find the right part
  // Lookup in array
  for (uint8_t i = 0; i < VG99_SIZE_OF_SUBLISTS; i++) {
    if (VG99_parameters[part][i].address == 0) break; //Break the loop if there is no more useful data
    if (address == VG99_parameters[part][i].address) { //Check is we've found the right address
      if (VG99_parameters[part][i].sublist == 0) { // If sublist number is zero, we can just go and display the name
        show_status_message(VG99_parameters[part][i].name);
      }
      else {                                       // If the sublist number is not zero, we will combine the name with the FX name
        sublist = VG99_parameters[part][i].sublist;
        msg1 = VG99_parameters[part][i].name;
        msg2 = VG99_sublists[sublist][type]; // Lookup the name of the FX type in the VG99_sublists array
        show_status_message(msg1 + " " + msg2 + " SW");
      }
    }
  }
}

// Looks for the on_colour as specified in the VG99_parameters array.
void VG99_find_colours(uint8_t VG99_current_assign) {
  uint8_t part;
  uint16_t address; // the address of the current assign target
  uint8_t type;
  uint8_t on_colour = VG99_STOMP_COLOUR_ON; // Set the default on colour
  uint8_t off_colour = VG99_STOMP_COLOUR_OFF; // Set the default on colour

  address = FC300_ctls[VG99_current_assign].assign_target;
  type = FC300_ctls[VG99_current_assign].target_byte2;
  part = (address / 0x1000); // As the array is divided in addresses by 1000, it is easy to find the right part

  // Lookup in array
  for (uint8_t i = 0; i < VG99_SIZE_OF_SUBLISTS; i++) {
    if (VG99_parameters[part][i].address == 0) break; //Break the loop if there is no more useful data
    if (address == VG99_parameters[part][i].address) { //Check is we've found the right address
      // Determine the on colour
      on_colour = VG99_parameters[part][i].colour_on;
      if (on_colour == VG99_FX_COLOUR_ON) on_colour = VG99_FX_colours[type][0]; // For FX1 and FX2 pick the colour from the FX_colours array
      if (on_colour == VG99_POLYFX_COLOUR_ON) on_colour = VG99_polyFX_colours[type][0]; // For FX1 and FX2 pick the colour from the FX_colours array
      // Determine the off colour
      off_colour = VG99_parameters[part][i].colour_off;
      if (off_colour == VG99_FX_COLOUR_OFF) off_colour = VG99_FX_colours[type][1]; // For FX1 and FX2 pick the colour from the FX_colours array
      if (off_colour == VG99_POLYFX_COLOUR_OFF) off_colour = VG99_polyFX_colours[type][1]; // For FX1 and FX2 pick the colour from the FX_colours array
    }
  }
  FC300_ctls[VG99_current_assign].colour_on = on_colour; // Store the colours in the assign array
  FC300_ctls[VG99_current_assign].colour_off = off_colour;
}
/*
uint8_t VG99_find_colour_on(uint16_t address, uint8_t type) {
  uint8_t part;
  uint8_t colour = VG99_STOMP_COLOUR_ON; // Set the default colour

  part = (address / 0x1000); // As the array is divided in addresses by 1000, it is easy to find the right part
  // Lookup in array
  for (uint8_t i = 0; i < VG99_SIZE_OF_SUBLISTS; i++) {
    if (VG99_parameters[part][i].address == 0) break; //Break the loop if there is no more useful data
    if (address == VG99_parameters[part][i].address) { //Check is we've found the right address
      colour = VG99_parameters[part][i].colour_on;
      if (colour == VG99_FX_COLOUR_ON) colour = VG99_FX_colours[type][0]; // For FX1 and FX2 pick the colour from the FX_colours array
      if (colour == VG99_POLYFX_COLOUR_ON) colour = VG99_polyFX_colours[type][0]; // For FX1 and FX2 pick the colour from the FX_colours array
    }
  }
  return colour;
}

// Looks for the off_colour as specified in the VG99_parameters array.
uint8_t VG99_find_colour_off(uint16_t address, uint8_t type) {
  uint8_t part;
  uint8_t colour = VG99_STOMP_COLOUR_OFF; // Set the default colour

  part = (address / 0x1000); // As the array is divided in addresses by 1000, it is easy to find the right part
  // Lookup in array
  for (uint8_t i = 0; i < VG99_SIZE_OF_SUBLISTS; i++) {
    if (VG99_parameters[part][i].address == 0) break; //Break the loop if there is no more useful data
    if (address == VG99_parameters[part][i].address) { //Check is we've found the right address
      colour = VG99_parameters[part][i].colour_off;
      if (colour == VG99_FX_COLOUR_OFF) colour = VG99_FX_colours[type][1]; // For FX1 and FX2 pick the colour from the FX_colours array
      if (colour == VG99_POLYFX_COLOUR_OFF) colour = VG99_polyFX_colours[type][1]; // For FX1 and FX2 pick the colour from the FX_colours array
    }
  }
  return colour;
}*/
