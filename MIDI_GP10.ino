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

// ******************************** MIDI messages and functions for the Boss GP-10 ********************************

// ********************************* Section 1: GP10 SYSEX messages ********************************************

//Messages are abbreviated to just the address and the data bytes. Checksum is calculated automatically
//Example: {0xF0, 0x41, 0x10, 0x00, 0x00, 0x00, 0x05, 0x12, 0x7F, 0x00, 0x00, 0x01, 0x01, 0x7F, 0xF7} is reduced to 0x7F000001, 0x01

#define GP10_EDITOR_MODE_ON 0x7F000001, 0x01 //Gets the GP-10 spitting out lots of sysex data. Should be switched on, otherwise the tuner does not work
#define GP10_EDITOR_MODE_OFF 0x7F000001, 0x00
#define GP10_REQUEST_PATCH_NAME 0x20000000, 12 //Request 12 bytes for current patch name
#define GP10_REQUEST_PATCH_NUMBER 0x00000000, 1 //Request current patch number

#define GP10_TUNER_ON 0x7F000002, 0x02 // Changes the running mode of the GP-10 to Tuner - Got these terms from the VG-99 sysex manual.
#define GP10_TUNER_OFF 0x7F000002, 0x00 //Changes the running mode of the GP10 to play.
#define GP10_SOLO_ON 0x2000500B, 0x01
#define GP10_SOLO_OFF 0x2000500B, 0x00

#define GP10_TEMPO 0x20000801  // Accepts values from 40 bpm - 250 bpm

// ********************************* Section 2: GP10 comon MIDI in functions ********************************************

void check_SYSEX_in_GP10(const unsigned char* sxdata, short unsigned int sxlength)
{
  // Check if it is a message from a GP-10
  if ((sxdata[2] == GP10_device_id) && (sxdata[3] == 0x00) && (sxdata[4] == 0x00) && (sxdata[5] == 0x00) && (sxdata[6] == 0x05) && (sxdata[7] == 0x12)) {
    uint32_t address = (sxdata[8] << 24) + (sxdata[9] << 16) + (sxdata[10] << 8) + sxdata[11]; // Make the address 32 bit
    // Check if it is the patch number (address: 0x00, 0x00, 0x00, 0x00)
    //if ((sxdata[7] == 0x12) && (sxdata[8] == 0x00) && (sxdata[9] == 0x00) && (sxdata[10] == 0x00) && (sxdata[11] == 0x00) ) {
    if (address == 0x00000000) {
      GP10_patch_number = sxdata[12];
      GP10_do_after_patch_selection();
    }

    // Check if it is the patch name (address: 0x20, 0x00, 0x00, 0x00)
    //if ((sxdata[7] == 0x12) && (sxdata[8] == 0x20) && (sxdata[9] == 0x00) && (sxdata[10] == 0x00) && (sxdata[11] == 0x00) ) {
    if (address == 0x20000000) {
      GP10_patch_name = "";
      for (uint8_t count = 12; count < 24; count++) {
        GP10_patch_name = GP10_patch_name + static_cast<char>(sxdata[count]); //Add ascii character to Patch Name String
      }
      update_lcd = true;

    }

    // Check if it is the tuner (address: 0x7F, 0x00, 0x00, 0x02)
    // You can engage the global tuner by entering tuner on the GP-10 :-)
    //if ((sxdata[7] == 0x12) && (sxdata[8] == 0x7F) && (sxdata[9] == 0x00) && (sxdata[10] == 0x00) && (sxdata[11] == 0x02)) {
    if (address == 0x7F000002) {
      if ((sxdata[12] == 0x02) && (mode != MODE_TUNER)) start_global_tuner();
      if ((sxdata[12] == 0x00) && (mode == MODE_TUNER)) stop_global_tuner();
      update_lcd = true;
      update_LEDS = true;
    }

    // Check if it is some other stompbox function and copy the status to the right LED
    GP10_check_stompbox_states(sxdata, sxlength);
  }
}

void check_PC_in_GP10(byte channel, byte program) {
  // Check the source by checking the channel
  if (channel == GP10_MIDI_channel) { // GP10 outputs a program change
    GP10_patch_number = program;
    GP10_do_after_patch_selection();
  }
}

void GP10_identity_check(const unsigned char* sxdata, short unsigned int sxlength)
{
  // Check if it is a GP-10
  if ((sxdata[6] == 0x05) && (sxdata[7] == 0x03) && (GP10_detected == false)) {
    GP10_detected = true;
    show_status_message("GP-10 detected  ");
    GP10_device_id = sxdata[2]; //Byte 2 contains the correct device ID
    GP10_request_patch_number();
    write_GP10(GP10_EDITOR_MODE_ON); // Put the GP10 in EDITOR mode - otherwise tuner will not work
  }
}

// ********************************* Section 3: GP10 comon MIDI out functions ********************************************

void write_GP10(uint32_t address, uint8_t value) // For sending one data byte
{
  uint8_t *ad = (uint8_t*)&address; //Split the 32-bit address into four bytes: ad[3], ad[2], ad[1] and ad[0]
  uint8_t checksum = (0x80 - (ad[3] + ad[2] + ad[1] + ad[0] + value) % 0x80); // Calculate the Roland checksum
  uint8_t sysexmessage[15] = {0xF0, 0x41, GP10_device_id, 0x00, 0x00, 0x00, 0x05, 0x12, ad[3], ad[2], ad[1], ad[0], value, checksum, 0xF7};
  usbMIDI.sendSysEx(15, sysexmessage);
  debug_sysex(sysexmessage, 15, "out(GP10)");
}

void write_GP10(uint32_t address, uint8_t value1, uint8_t value2) // For sending two data bytes
{
  uint8_t *ad = (uint8_t*)&address; //Split the 32-bit address into four bytes: ad[3], ad[2], ad[1] and ad[0]
  uint8_t checksum = (0x80 - (ad[3] + ad[2] + ad[1] + ad[0] + value1 + value2) % 0x80); // Calculate the Roland checksum
  uint8_t sysexmessage[16] = {0xF0, 0x41, GP10_device_id, 0x00, 0x00, 0x00, 0x05, 0x12, ad[3], ad[2], ad[1], ad[0], value1, value2, checksum, 0xF7};
  usbMIDI.sendSysEx(16, sysexmessage);
  debug_sysex(sysexmessage, 16, "out(GP10)");
}

void request_GP10(uint32_t address, uint8_t no_of_bytes)
{
  uint8_t *ad = (uint8_t*)&address; //Split the 32-bit address into four bytes: ad[3], ad[2], ad[1] and ad[0]
  uint8_t checksum = (0x80 - (ad[3] + ad[2] + ad[1] + ad[0] +  no_of_bytes) % 0x80); // Calculate the Roland checksum
  uint8_t sysexmessage[18] = {0xF0, 0x41, GP10_device_id, 0x00, 0x00, 0x00, 0x05, 0x11, ad[3], ad[2], ad[1], ad[0], 0x00, 0x00, 0x00, no_of_bytes, checksum, 0xF7};
  usbMIDI.sendSysEx(18, sysexmessage);
}

uint8_t GP10_previous_patch_number = 0;
uint8_t GP10_patch_memory = 0;

void GP10_SendProgramChange(uint8_t new_patch) {
  //  if (GP10_patch_number == GP10_previous_patch_number) { // Move to the previous patch
  //    GP10_patch_number = GP10_patch_memory; // Go to the stored patch location
  //    GP10_patch_memory = GP10_previous_patch_number; // Remember the current patch - which happens to be the same as the previous one
  //    GP10_bank_number = (GP10_patch_number / 10); // Update the bank number to the new location
  //  }
  //  else { // Choose the current patch
  //    GP10_patch_memory = GP10_previous_patch_number; // Remember the previous patch
  //    GP10_previous_patch_number = GP10_patch_number; // Store the current patch
  //  }

  usbMIDI.sendProgramChange(new_patch, GP10_MIDI_channel);
  GP10_do_after_patch_selection();
}

void GP10_do_after_patch_selection() {
  GP10_request_name();
  GP10_request_stompbox_states();
  if (SEND_GLOBAL_TEMPO_AFTER_PATCH_CHANGE == true) GP10_send_bpm();
  update_LEDS = true;
  update_lcd = true;
  EEPROM.write(EEPROM_GP10_PATCH_NUMBER, GP10_patch_number);
}

void GP10_request_patch_number()
{
  request_GP10(GP10_REQUEST_PATCH_NUMBER);
}

void GP10_request_name()
{
  request_GP10(GP10_REQUEST_PATCH_NAME);
}

void GP10_send_bpm() {
  write_GP10(GP10_TEMPO, bpm / 16, bpm % 16); // Tempo is modulus 16. It's all so very logical. NOT.
}

// ********************************* Section 4: GP10 stompbox functions ********************************************
// Here we define some stompboxes for fixed parameters of the GP10

// Format of this stompbox array is:
// {SYSEX_ADDRESS, NAME FOR DISPLAY, ON_COLOUR, OFF_COLOUR, START_COLOUR}
struct stomper { // Combines all the data we need for controlling a parameter in a device
  uint32_t address;
  char name[17];
  uint8_t colour_on;
  uint8_t colour_off;
  uint8_t LED;
};

//typedef struct stomper Stomper;

// Make sure you edit the GP10_NUMBER_OF_STOMPS when adding new parameters
#define GP10_NUMBER_OF_STOMPS 6

stomper GP10_stomps[GP10_NUMBER_OF_STOMPS] = {
  {0x2000500B, "Amp solo", GP10_STOMP_COLOUR_ON, GP10_STOMP_COLOUR_OFF, GP10_STOMP_COLOUR_OFF},
  {0x20005800, "FX", GP10_STOMP_COLOUR_ON, GP10_STOMP_COLOUR_OFF, GP10_STOMP_COLOUR_OFF},
  {0x10001000, "Guitar2MIDI", GP10_STOMP_COLOUR_ON, GP10_STOMP_COLOUR_OFF, GP10_STOMP_COLOUR_OFF},
  {0x20016800, "Chorus", GP10_STOMP_COLOUR_ON, GP10_STOMP_COLOUR_OFF, GP10_STOMP_COLOUR_OFF},
  {0x20017000, "Delay", GP10_STOMP_COLOUR_ON, GP10_STOMP_COLOUR_OFF, GP10_STOMP_COLOUR_OFF},
  {0x20017800, "Reverb", GP10_STOMP_COLOUR_ON, GP10_STOMP_COLOUR_OFF, GP10_STOMP_COLOUR_OFF},
};


// Sends requests to the GP10 to send the current settings of all the GP10_stomps
void GP10_request_stompbox_states()
{
  for (uint8_t addr_count = 0; addr_count < GP10_NUMBER_OF_STOMPS; addr_count++) {
    request_GP10(GP10_stomps[addr_count].address, 1);
  }
}

// Reads out the parameters that were requested with GP10_request_stompbox_states()
void GP10_check_stompbox_states(const unsigned char* sxdata, short unsigned int sxlength)
{
  uint32_t address = (sxdata[8] << 24) + (sxdata[9] << 16) + (sxdata[10] << 8) + sxdata[11]; // Make the address 32 bit
  for (uint8_t addr_count = 0; addr_count < GP10_NUMBER_OF_STOMPS; addr_count++) {
      if (address == GP10_stomps[addr_count].address) {
        // Copy status to the LED
        if (sxdata[12] == 0x01) GP10_stomps[addr_count].LED = GP10_STOMP_COLOUR_ON; // Switch the LED on
        if (sxdata[12] == 0x00) GP10_stomps[addr_count].LED = GP10_STOMP_COLOUR_OFF; // Switch the LED off
        update_LEDS = true;
      }
    }
}

// Toggle GP10 stompbox parameter
void GP10_stomp(uint8_t number) {
  if (GP10_stomps[number].LED == GP10_stomps[number].colour_off) {
    GP10_stomps[number].LED = GP10_stomps[number].colour_on; // Switch the LED on with the GP10 stomp colour
    write_GP10(GP10_stomps[number].address, 0x01);
    String msg = GP10_stomps[number].name;
    show_status_message(msg + " ON");
  }
  else {
    GP10_stomps[number].LED = GP10_stomps[number].colour_off; // Switch the LED off with the GP10 stomp colour
    write_GP10(GP10_stomps[number].address, 0x00);
    String msg = GP10_stomps[number].name;
    show_status_message(msg + " OFF");
  }
}




