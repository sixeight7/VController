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
#define GR55_REQUEST_MODE 0x18000000, 1 //Is 00 in guitar mode, 01 in bass mode (Gumtown in town :-)
#define GR55_REQUEST_PATCHNAME 0x18000001, 16 //Request 17 bytes for current patch name - GR55 send one dummy byte before the patchname data
#define GR55_REQUEST_PATCH_NUMBER 0x01000000, 2 //Request current patch number
#define GR55_CTL_LED_ON 0x18000011, 0x01
#define GR55_CTL_LED_OFF 0x18000011, 0x00

#define GR55_TEMPO 0x1800023C  // Accepts values from 40 bpm - 250 bpm

#define GR55_SYNTH1_SW 0x18002003 // The address of the synth1 switch
#define GR55_SYNTH2_SW 0x18002103 // The address of the synth1 switch
#define GR55_COSM_GUITAR_SW 0x1800100A // The address of the COSM guitar switch
#define GR55_NORMAL_PU_SW 0x18000232 // The address of the COSM guitar switch
bool GR55_request_onoff = false;
uint8_t GR55_current_assign = 255; // The assign that is being read - set to a high value, because it is not time to read an assign yet.

#define GR55_SYSEX_WATCHDOG_LENGTH 1000 // watchdog for messages (in msec)
unsigned long GR55sysexWatchdog = 0;
boolean GR55_sysex_watchdog_running = false;


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

    // Check if it is the GR55 guitar/bass mode (address: 0x18, 0x00, 0x00, 0x00)
    if ((sxdata[6] == 0x12) && (address == 0x18000000) && (sxdata[11] == 0x01)) {
      GR55_preset_banks = 12; // In bass mode
      if (GR55_BANK_MAX > 135) GR55_BANK_MAX = 135; // Check if the maximum bank number does not exceed 135 in bass mode
    }

    // Check if it is the patch name (address: 0x18, 0x00, 0x00, 0x01)
    if ((sxdata[6] == 0x12) && (address == 0x18000001) ) {
      GR55_patch_name = "";
      for (uint8_t count = 11; count < 27; count++) {
        GR55_patch_name = GR55_patch_name + static_cast<char>(sxdata[count]); //Add ascii character to Patch Name String
      }
      update_lcd = true;
      // Sending the current bpm had to be done a little later. Doing at after receiving the patch name gives enough delay.
      if (SEND_GLOBAL_TEMPO_AFTER_PATCH_CHANGE == true) GR55_send_bpm();
      GR55_request_guitar_switch_states();
    }

    // Check if it is the guitar on/off states
    GR55_check_guitar_switch_states(sxdata, sxlength);

    // Check assigns
    read_GR55_CTL_assigns(sxdata, sxlength);
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
    request_GR55(GR55_REQUEST_MODE); // Check for guitar or bass mode
    request_GR55(GR55_REQUEST_PATCH_NUMBER); // Request the current patch number
    GR55_do_after_patch_selection();
  }
}

// ********************************* Section 3: GR55 comon MIDI out functions ********************************************

void write_GR55(uint32_t address, uint8_t value) // For sending one data byte
{
  uint8_t *ad = (uint8_t*)&address; //Split the 32-bit address into four bytes: ad[3], ad[2], ad[1] and ad[0]
  uint8_t checksum = calc_checksum(ad[3] + ad[2] + ad[1] + ad[0] + value); // Calculate the Roland checksum
  uint8_t sysexmessage[14] = {0xF0, 0x41, GR55_device_id, 0x00, 0x00, 0x053, 0x12, ad[3], ad[2], ad[1], ad[0], value, checksum, 0xF7};
  if (GR55_MIDI_port == USBMIDI_PORT) usbMIDI.sendSysEx(14, sysexmessage);
  if (GR55_MIDI_port == MIDI1_PORT) MIDI1.sendSysEx(13, sysexmessage);
  if (GR55_MIDI_port == MIDI2_PORT) MIDI2.sendSysEx(13, sysexmessage);
  if (GR55_MIDI_port == MIDI3_PORT) MIDI2.sendSysEx(13, sysexmessage);
  debug_sysex(sysexmessage, 14, "out(GR55)");
}

void write_GR55(uint32_t address, uint8_t value1, uint8_t value2) // For sending two data bytes
{
  uint8_t *ad = (uint8_t*)&address; //Split the 32-bit address into four bytes: ad[3], ad[2], ad[1] and ad[0]
  uint8_t checksum = calc_checksum(ad[3] + ad[2] + ad[1] + ad[0] + value1 + value2); // Calculate the Roland checksum
  uint8_t sysexmessage[15] = {0xF0, 0x41, GR55_device_id, 0x00, 0x00, 0x053, 0x12, ad[3], ad[2], ad[1], ad[0], value1, value2, checksum, 0xF7};
  if (GR55_MIDI_port == USBMIDI_PORT) usbMIDI.sendSysEx(15, sysexmessage);
  if (GR55_MIDI_port == MIDI1_PORT) MIDI1.sendSysEx(14, sysexmessage);
  if (GR55_MIDI_port == MIDI2_PORT) MIDI2.sendSysEx(14, sysexmessage);
  if (GR55_MIDI_port == MIDI3_PORT) MIDI2.sendSysEx(14, sysexmessage);
  debug_sysex(sysexmessage, 15, "out(GR55)");
}

void request_GR55(uint32_t address, uint8_t no_of_bytes)
{
  uint8_t *ad = (uint8_t*)&address; //Split the 32-bit address into four bytes: ad[3], ad[2], ad[1] and ad[0]
  uint8_t checksum = calc_checksum(ad[3] + ad[2] + ad[1] + ad[0] +  no_of_bytes); // Calculate the Roland checksum
  uint8_t sysexmessage[17] = {0xF0, 0x41, GR55_device_id, 0x00, 0x00, 0x53, 0x11, ad[3], ad[2], ad[1], ad[0], 0x00, 0x00, 0x00, no_of_bytes, checksum, 0xF7};
  if (GR55_MIDI_port == USBMIDI_PORT) usbMIDI.sendSysEx(17, sysexmessage);
  if (GR55_MIDI_port == MIDI1_PORT) MIDI1.sendSysEx(16, sysexmessage);
  if (GR55_MIDI_port == MIDI2_PORT) MIDI2.sendSysEx(16, sysexmessage);
  if (GR55_MIDI_port == MIDI3_PORT) MIDI2.sendSysEx(16, sysexmessage);
  debug_sysex(sysexmessage, 17, "out(GR55)");
}

// Send CC message to GR-55
void GR55_send_cc(uint8_t control, uint8_t value) {
  if (GR55_MIDI_port == USBMIDI_PORT) usbMIDI.sendControlChange(control, value, GR55_MIDI_channel);
  if (GR55_MIDI_port == MIDI1_PORT) MIDI1.sendControlChange(control, value, GR55_MIDI_channel);
  if (GR55_MIDI_port == MIDI2_PORT) MIDI2.sendControlChange(control, value, GR55_MIDI_channel);
  if (GR55_MIDI_port == MIDI3_PORT) MIDI3.sendControlChange(control, value, GR55_MIDI_channel);
}

// Send Program Change to GR-55
void GR55_SendProgramChange(uint16_t new_patch) {
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
  no_device_check = true; // disables checking for devices during reading of GR_55
  GR55_current_assign = 255; // In case we were still in the middle of a previous patch change, switch off the lot
  GR55_request_onoff = false;
  GR55_sysex_watchdog_running = false;
  
  GR55_on = true;
  GP10_mute();
  VG99_mute();
  GR55_request_name();
  //GR55_request_guitar_switch_states(); // Moet later
  //GR55_request_stompbox_states();
  //if (SEND_GLOBAL_TEMPO_AFTER_PATCH_CHANGE == true) GR55_send_bpm(); // Here is too soon, the GR55 does not pick it up - this line is moved to the Check_MIDI_in_GR55() procedure.
  update_LEDS = true;
  update_lcd = true;
  //EEPROM.write(EEPROM_GR55_PATCH_MSB, (GR55_patch_number / 256));
  //EEPROM.write(EEPROM_GR55_PATCH_LSB, (GR55_patch_number % 256));
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
      Request_GR55_CTL_first_assign(); // Make sure this starts after VG99_request-guitar_onoff_state is done - they often ask for the same info - switches off now

    }
  }
}

void GR55_select_switch() {
  if ((GR55_on) && (!US20_emulation_state_changed)) {
    GR55_always_on_toggle();
  }
  else {
    GR55_unmute();
    GP10_mute();
    VG99_mute();
    if (mode != MODE_GP10_GR55_COMBI) show_status_message(GR55_patch_name); // Show the correct patch name
    US20_emulation_state_changed = false;
  }
}

void GR55_always_on_toggle() {
  GR55_always_on = !GR55_always_on; // Toggle GR55_always_on
  if (GR55_always_on) {
    GR55_unmute();
    show_status_message("GR55 always ON");
  }
  else {
    //GR55_mute();
    show_status_message("GR55 can be muted");
    US20_emulation_state_changed = true;
  }
}

void GR55_unmute() {
  GR55_on = true;
  GR55_select_LED = GR55_PATCH_COLOUR; //Switch the LED on
  write_GR55(GR55_SYNTH1_SW, GR55_synth1_onoff); // Switch synth 1 off
  write_GR55(GR55_SYNTH2_SW, GR55_synth2_onoff); // Switch synth 1 off
  write_GR55(GR55_COSM_GUITAR_SW, GR55_COSM_onoff); // Switch COSM guitar on
  write_GR55(GR55_NORMAL_PU_SW, GR55_nrml_pu_onoff); // Switch normal pu on
}

void GR55_mute() {
  if (!GR55_always_on) {
    GR55_mute_now();
  }
}

void GR55_mute_now() { // Needed a second version, because the GR55 must always mute when engaging global tuner.
  GR55_on = false;
  GR55_select_LED = GR55_OFF_COLOUR; //Switch the LED off
  write_GR55(GR55_SYNTH1_SW, 0x01); // Switch synth 1 off
  write_GR55(GR55_SYNTH2_SW, 0x01); // Switch synth 1 off
  write_GR55(GR55_COSM_GUITAR_SW, 0x01); // Switch COSM guitar off
  write_GR55(GR55_NORMAL_PU_SW, 0x01); // Switch normal pu off
}

// *************************************************************************************************
// GR55 Assign control. The GR55 has 8 assigns. We control them with 8 cc messages.
// Because we also read what parameter the assign is set to and whether it is on or off,
// assign 1 should always be cc #21, assign 2 cc #22, etc.
// The state of the LED is not always correct, but this is probably due to the firmware on the GR-55
// The state of the LED of the CTL button is quite often wrong too...
// *************************************************************************************************

struct GR55_CTL {       // Datastructure for GR55 assign controllers:
  uint8_t cc;           // Contains the Contrinuous Control number to control the assign
  char name[6];         // Contains the name of the pedal
  uint8_t colour_on;     // Colour when the pedal is on
  uint8_t colour_off;    // Colour when the pedal is off
  uint8_t LED;           // Actual LED displaying one of the previous colours
  bool reversed;         // True when colours are reversed
  uint32_t assign_addr;  // The address of the assign in the GR55
  bool assign_on;        // Assign: on/off
  uint16_t assign_target;// Assign: target
  uint16_t target_address; // The address of the assign target - has to be looked up
  uint16_t assign_min;   // Assign: min-value (switch is off)
  uint16_t assign_max;   // Assign: max_value (switch is on)
  bool assign_latch;     // Assign: momentary (false) or latch (true)
  uint8_t target_byte1;  // Once the assign target is known, the state of the target is read into two bytes
  uint8_t target_byte2;  // This byte often contains the type of the assign - which we exploit in the part of parameter feedback
};

// GR55 Assignment 1-8 sysex addresses
#define GR55_ASSIGN1_assign 0x1800010C
#define GR55_ASSIGN2_assign 0x1800011F
#define GR55_ASSIGN3_assign 0x18000132
#define GR55_ASSIGN4_assign 0x18000145
#define GR55_ASSIGN5_assign 0x18000158
#define GR55_ASSIGN6_assign 0x1800016B
#define GR55_ASSIGN7_assign 0x1800017E
#define GR55_ASSIGN8_assign 0x18000211

#define GR55_NUMBER_OF_CTLS 3 // Set to three, because I don't need that many assigns on the GR55
GR55_CTL GR55_ctls[GR55_NUMBER_OF_CTLS] = {
  //  {21, "ASGN1", GR55_STOMP_COLOUR_ON, GR55_STOMP_COLOUR_OFF, 0, false, GR55_ASSIGN1_assign, false, 0, 0, 0, 0, false, 0, 0},
  //  {22, "ASGN2", GR55_STOMP_COLOUR_ON, GR55_STOMP_COLOUR_OFF, 0, false, GR55_ASSIGN2_assign, false, 0, 0, 0, 0, false, 0, 0},
  //  {23, "ASGN3", GR55_STOMP_COLOUR_ON, GR55_STOMP_COLOUR_OFF, 0, false, GR55_ASSIGN3_assign, false, 0, 0, 0, 0, false, 0, 0},
  //  {24, "ASGN4", GR55_STOMP_COLOUR_ON, GR55_STOMP_COLOUR_OFF, 0, false, GR55_ASSIGN4_assign, false, 0, 0, 0, 0, false, 0, 0},
  //  {25, "ASGN5", GR55_STOMP_COLOUR_ON, GR55_STOMP_COLOUR_OFF, 0, false, GR55_ASSIGN5_assign, false, 0, 0, 0, 0, false, 0, 0},
  {26, "ASGN6", GR55_STOMP_COLOUR_ON, GR55_STOMP_COLOUR_OFF, 0, false, GR55_ASSIGN6_assign, false, 0, 0, 0, 0, false, 0, 0},
  {27, "ASGN7", GR55_STOMP_COLOUR_ON, GR55_STOMP_COLOUR_OFF, 0, false, GR55_ASSIGN7_assign, false, 0, 0, 0, 0, false, 0, 0},
  {28, "ASGN8", GR55_STOMP_COLOUR_ON, GR55_STOMP_COLOUR_OFF, 0, false, GR55_ASSIGN8_assign, false, 0, 0, 0, 0, false, 0, 0}
};

void GR55_stomp_press(uint8_t number) {

  // Press the pedal via cc
  GR55_send_cc(GR55_ctls[number].cc , 127);

  //Serial.println("You pressed: "+String(number));

  // Toggle LED status
  if (GR55_ctls[number].LED == GR55_ctls[number].colour_on) GR55_ctls[number].LED = GR55_ctls[number].colour_off;
  else GR55_ctls[number].LED = GR55_ctls[number].colour_on;

  // Display the patch function if the pedal is on
  if (GR55_ctls[number].assign_on) {
    GR55_display_parameter(GR55_ctls[number].assign_target, GR55_ctls[number].target_byte2, number); // Start parameter feedback - target_byte2 contains in some cases the number of the FX type
  }
  else {
    String msg = GR55_ctls[number].name;
    show_status_message("CC#" + String(GR55_ctls[number].cc) + " (" + msg + ")");
  }

}

void GR55_stomp_release(uint8_t number) {

  // Press the pedal via cc
  GR55_send_cc(GR55_ctls[number].cc , 0);

  if (GR55_ctls[number].assign_latch == false) {
    if (GR55_ctls[number].assign_on == true) GR55_ctls[number].LED = GR55_ctls[number].colour_off; // Switch the LED off with the GP10 stomp colour
    else GR55_ctls[number].LED = 0; //Or if the assign is not set, switch the LED off
  }
}

// Reading of the assigns - to avoid MIDI buffer overruns in the GR55, the assigns are read one by one
// 1. Request_GR55_CTL_first_assign() starts this process. It resets the GR55_current_assign and starts the loop
// 2. Request_GR55_CTL_current_assign() puts a request out to the GR55 for the next FC300 CTL button assign.
// 3. read_GR55_CTL_assigns() will receive the settings of the CTL button assign and store it in the GR55_ctrls array.
//    It will then request the GR55 for the state of the target of the GR55 - always requesting two bytes
// 4. read_GR55_CTL_assigns() will receive the setting of the CTL assign target and store it in the GR55_ctrls array and update the LED of the assign.
//    It will then update GR55_current_assign and request the next assign - which brings us back to step 2.

void Request_GR55_CTL_first_assign() {
  GR55_current_assign = 0; //After the name is read, the assigns can be read
  Serial.println("Start reading GR55 assigns");
  GR55_set_sysex_watchdog();
  Request_GR55_CTL_current_assign();
}

void Request_GR55_CTL_current_assign() { //Will request the next assign - the assigns are read one by one, otherwise the data will not arrive!
  if (GR55_current_assign < GR55_NUMBER_OF_CTLS) {
    Serial.println("Request GR55_current_assign=" + String(GR55_current_assign));
    request_GR55(GR55_ctls[GR55_current_assign].assign_addr, 12); // Request 12 bytes for the assign
    GR55_set_sysex_watchdog(); // Set the timer
  }
  else {
    GR55_sysex_watchdog_running = false; // Stop the timer
    no_device_check = false; // restart device check
  }
}

void GR55_set_sysex_watchdog() {
  GR55sysexWatchdog = millis() + GR55_SYSEX_WATCHDOG_LENGTH;
  GR55_sysex_watchdog_running = true;
  Serial.println("GR55 sysex watchdog started");
}

void GR55_check_sysex_watchdog() {
  if ((millis() > GR55sysexWatchdog) && (GR55_sysex_watchdog_running)) {
    Serial.println("GR55 sysex watchdog expired");
    Request_GR55_CTL_current_assign(); // Try reading the assign again
  }
}

void read_GR55_CTL_assigns(const unsigned char* sxdata, short unsigned int sxlength) {
  uint32_t address = (sxdata[7] << 24) + (sxdata[8] << 16) + (sxdata[9] << 8) + sxdata[10]; // Make the address 32 bit

  if (GR55_current_assign < GR55_NUMBER_OF_CTLS) { // Check if we have not reached the last assign.
    // Check if it is the CTL assign address
    if ((sxdata[6] == 0x12) && (address == GR55_ctls[GR55_current_assign].assign_addr)) {
      //Serial.println("Received GR55_current_assign=" + String(GR55_current_assign));
      // Copy the data into the FC300 assign array
      uint8_t assign_source_cc = (sxdata[21] - 8); //cc01 - cc31 are stored as 9 - 39
      GR55_ctls[GR55_current_assign].assign_on = ((sxdata[11] == 0x01) && (assign_source_cc == GR55_ctls[GR55_current_assign].cc));
      GR55_ctls[GR55_current_assign].assign_target = (sxdata[12] << 8) + (sxdata[13] << 4) + sxdata[14];
      GR55_ctls[GR55_current_assign].assign_min = ((sxdata[15] - 0x04) << 8) + (sxdata[16] << 4) + sxdata[17];
      GR55_ctls[GR55_current_assign].assign_max = ((sxdata[18] - 0x04) << 8) + (sxdata[19] << 4) + sxdata[20];
      GR55_ctls[GR55_current_assign].assign_latch = ((sxdata[22] == 0x01) && (GR55_ctls[GR55_current_assign].assign_on));

      //Serial.println("Assign_on:" + String(GR55_ctls[GR55_current_assign].assign_on));
      //Serial.println("Assign_target:" + String(GR55_ctls[GR55_current_assign].assign_target, HEX));

      //Request the status of the target from the patch temporary area is assignment is on
      if (GR55_ctls[GR55_current_assign].assign_on == true) {
        GR55_target_lookup(GR55_current_assign); // Lookup the address of the target in the GR55_Parameters array
        Serial.println("Request target of assign " + String(GR55_current_assign) + ": " + String(GR55_ctls[GR55_current_assign].target_address, HEX));
        request_GR55((0x18000000 + GR55_ctls[GR55_current_assign].target_address), 2);
      }
      else {
        GR55_ctls[GR55_current_assign].LED = 0; // Switch the LED off
        GR55_ctls[GR55_current_assign].colour_on = GR55_STOMP_COLOUR_ON; // Set the on colour to default
        GR55_ctls[GR55_current_assign].colour_off = 0; // Set the off colour to LED off
        //Serial.println("Current LED off for GR55_current_assign=" + String(GR55_current_assign));
        GR55_current_assign++; // Select the next assign
        Request_GR55_CTL_current_assign(); //Request the next assign
      }

    }

    // Check if it is the requested data for the previous controller
    uint32_t requested_address = 0x18000000 + GR55_ctls[GR55_current_assign].target_address;
    if ((sxdata[6] == 0x12) && (address == requested_address) && (GR55_ctls[GR55_current_assign].target_address != 0)) {

      Serial.println("Target received of assign " + String(GR55_current_assign) + ": " + String(GR55_ctls[GR55_current_assign].target_address, HEX));

      // Write the received bytes in the array
      GR55_ctls[GR55_current_assign].target_byte1 = sxdata[11];
      GR55_ctls[GR55_current_assign].target_byte2 = sxdata[12];

      GR55_find_colours(GR55_current_assign);

      // Set the LED status for this ctl pedal
      Serial.println("Target byte:" + String(GR55_ctls[GR55_current_assign].target_byte1, HEX) + " == Assign max:" + String(GR55_ctls[GR55_current_assign].assign_max, HEX));
      if (((GR55_ctls[GR55_current_assign].reversed == false) && (GR55_ctls[GR55_current_assign].target_byte1 == GR55_ctls[GR55_current_assign].assign_max)) ||
          ((GR55_ctls[GR55_current_assign].reversed == true) && (GR55_ctls[GR55_current_assign].target_byte1 == GR55_ctls[GR55_current_assign].assign_min))) {
        GR55_ctls[GR55_current_assign].LED = GR55_ctls[GR55_current_assign].colour_on;
      }
      else {
        GR55_ctls[GR55_current_assign].LED = GR55_ctls[GR55_current_assign].colour_off;
      }
      update_LEDS = true;
      GR55_current_assign++; // Select the next assign
      Request_GR55_CTL_current_assign(); //Request the next assign
    }
  }
}

struct Assign {  // Datastructure for parameters:
  uint16_t target; // Target of the assign as given in the assignments of the GR55
  char name[17];    // Name of the assign that will appear on the display
  uint16_t address; // Address of the assign target
  bool reversed; // Some parameters work reversed (PCM tone and guitar are muted when on)
  uint8_t sublist;  // Number of the sublist that exists for this parameter
  uint8_t colour_on; // Colour of the LED when this parameter is on
  uint8_t colour_off; // Colour of the LED when this parameter is off
};

#define GR55_MFX_COLOUR_ON 254 // Just a colour number to pick the colour from the VG99_FX_colours table
#define GR55_MFX_COLOUR_OFF 255 // Just a colour number to pick the colour from the VG99_FX_colours table
#define GR55_MOD_COLOUR_ON 252 // Just a colour number to pick the colour from the VG99_polyFX_colours table
#define GR55_MOD_COLOUR_OFF 253 // Just a colour number to pick the colour from the VG99_polyFX_colours table

#define GR55_PARAMETERS_PARTS 1
#define GR55_PARAMETERS_SIZE 20
const PROGMEM Assign GR55_parameters[GR55_PARAMETERS_PARTS][GR55_PARAMETERS_SIZE] = {
  { // 000 - 100
    {0x000, "SYNTH1 SW", 0x2003, true, 0, FX_GTR_COLOUR_ON, FX_GTR_COLOUR_OFF},
    {0x003, "PCM1 TONE OCT", 0x2005, false, 0, FX_GTR_COLOUR_ON, FX_GTR_COLOUR_OFF},
    {0x03B, "SYNTH2 SW",  0x2103, true, 0, FX_GTR_COLOUR_ON, FX_GTR_COLOUR_OFF},
    {0x03E, "PCM2 TONE OCT",  0x2105, false, 0, FX_GTR_COLOUR_ON, FX_GTR_COLOUR_OFF},
    {0x076, "COSM GT SW", 0x100A, true, 0, FX_GTR_COLOUR_ON, FX_GTR_COLOUR_OFF},
    {0x081, "12STRING SW", 0x101D, false, 0, FX_PITCH_COLOUR_ON, FX_PITCH_COLOUR_OFF},
    {0x0D6, "AMP", 0x0700, false, 2, FX_AMP_COLOUR_ON, FX_AMP_COLOUR_OFF},
    {0x0D7, "AMP GAIN",  0x0702, false, 0, FX_AMP_COLOUR_ON, FX_AMP_COLOUR_OFF},
    {0x0D9, "AMP GAIN SW", 0x0704, false, 0, FX_AMP_COLOUR_ON, FX_AMP_COLOUR_OFF},
    {0x0DA, "AMP SOLO SW", 0x0705, false, 0, FX_AMP_COLOUR_ON, FX_AMP_COLOUR_OFF},
    {0x0E0, "AMP BRIGHT", 0x070B, false, 0, FX_AMP_COLOUR_ON, FX_AMP_COLOUR_OFF},
    {0x0E6, "MOD", 0x0715, false, 3, GR55_MOD_COLOUR_ON, GR55_MOD_COLOUR_OFF},

    {0x128, "NS SWITCH", 0x075A, false, 0, FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF},
    {0x12B, "MFX", 0x0304, false, 1, GR55_MFX_COLOUR_ON, GR55_MFX_COLOUR_OFF},
    {0x1EC, "DELAY SW", 0x0605, false, 0, FX_DELAY_COLOUR_ON, FX_DELAY_COLOUR_OFF},
    {0x1F4, "REVERB SW", 0x060C, false, 0, FX_REVERB_COLOUR_ON, FX_REVERB_COLOUR_OFF},
    {0x1FC, "CHORUS SW", 0x0600, false, 0, FX_MODULATE_COLOUR_ON, FX_MODULATE_COLOUR_OFF},

    {0x204, "EQ SWITCH", 0x0611, false, 0, FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF},
    {0x213, "ALT.TUNING", 0x0234, false, 0, FX_PITCH_COLOUR_ON, FX_PITCH_COLOUR_OFF},
    {0x216, "PATCH LVL", 0x0230, false, 0, FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF},
  }
};

#define GR55_NO_OF_SUBLISTS 3
#define GR55_SIZE_OF_SUBLISTS 42
const PROGMEM char GR55_sublists[GR55_NO_OF_SUBLISTS][GR55_SIZE_OF_SUBLISTS][9] = {

  // Sublist 1: MFX FX types
  { "EQ", "S FILTR", "PHASER", "STEP PHR", "RING MOD", "TREMOLO", "AUTO PAN", "SLICER", "K ROTARY", "HEXA-CHS",
    "SPACE-D", "FLANGER", "STEP FLR", "AMP SIM", "COMPRESS", "LIMITER", "3TAP DLY", "TIME DLY", "LOFI CPS", "PITCH SH"
  },

  // Sublist 2: Amp types
  { "BOSS CLN", "JC-120", "JAZZ CBO", "FULL RNG", "CLN TWIN", "PRO CRCH", "TWEED", "DELUX CR", "BOSS CRH", "BLUES",
    "WILD CRH", "STACK CR", "VO DRIVE", "VO LEAD", "VO CLEAN", "MATCH DR", "FAT MTCH", "MATCH LD", "BG LEAD", "BG DRIVE",
    "BG RHYTH", "MS'59 I", "MS'59 II", "MS HIGN", "MS SCOOP", "R-FIER V", "R-FIER M", "R-FIER C", "T-AMP LD", "T-AMP CR",
    "T-AMP CL", "BOSS DRV", "SLDN", "LEAD STK", "HEAVY LD", "BOSS MTL", "5150 DRV", "METAL LD", "EDGE LD", "BASS CLN",
    "BASS CRH", "BASS HIG"
  },

  // Sublist 3: MOD FX types
  { "OD/DS", "WAH", "COMP", "LIMITER", "OCTAVE", " PHASER", "FLANGER", "TREMOLO", "ROTARY", "UNI-V", "PAN", "DELAY", "CHORUS", "EQ"
  }
};

const PROGMEM uint8_t GR55_MFX_colours[20][2] = {
  { FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF }, // Colour for "EQ",
  { FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF }, // Colour for "S FILTR",
  { FX_MODULATE_COLOUR_ON, FX_MODULATE_COLOUR_OFF }, // Colour for "PHASER",
  { FX_MODULATE_COLOUR_ON, FX_MODULATE_COLOUR_OFF }, // Colour for "STEP PHR",
  { FX_MODULATE_COLOUR_ON, FX_MODULATE_COLOUR_OFF }, // Colour for "RING MOD",
  { FX_MODULATE_COLOUR_ON, FX_MODULATE_COLOUR_OFF }, // Colour for "TREMOLO",
  { FX_MODULATE_COLOUR_ON, FX_MODULATE_COLOUR_OFF }, // Colour for "AUTO PAN",
  { FX_MODULATE_COLOUR_ON, FX_MODULATE_COLOUR_OFF }, // Colour for "SLICER",
  { FX_MODULATE_COLOUR_ON, FX_MODULATE_COLOUR_OFF }, // Colour for "K ROTARY",
  { FX_MODULATE_COLOUR_ON, FX_MODULATE_COLOUR_OFF }, // Colour for "HEXA-CHS",
  { FX_MODULATE_COLOUR_ON, FX_MODULATE_COLOUR_OFF }, // Colour for "SPACE-D",
  { FX_MODULATE_COLOUR_ON, FX_MODULATE_COLOUR_OFF }, // Colour for "FLANGER",
  { FX_MODULATE_COLOUR_ON, FX_MODULATE_COLOUR_OFF }, // Colour for "STEP FLR",
  { FX_AMP_COLOUR_ON, FX_AMP_COLOUR_OFF }, // Colour for "AMP SIM",
  { FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF }, // Colour for "COMPRESS",
  { FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF }, // Colour for "LIMITER",
  { FX_DELAY_COLOUR_ON, FX_DELAY_COLOUR_OFF }, // Colour for "3TAP DLY",
  { FX_DELAY_COLOUR_ON, FX_DELAY_COLOUR_OFF }, // Colour for "TIME DLY",
  { FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF }, // Colour for "LOFI CPS",
  { FX_PITCH_COLOUR_ON, FX_PITCH_COLOUR_OFF } // Colour for "PITCH SH"
};

const PROGMEM uint8_t GR55_MOD_colours[14][2] = {
  { FX_DIST_COLOUR_ON, FX_DIST_COLOUR_OFF }, // Colour for "OD/DS",
  { FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF }, // Colour for "WAH",
  { FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF }, // Colour for "COMP",
  { FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF }, // Colour for "LIMITER",
  { FX_PITCH_COLOUR_ON, FX_PITCH_COLOUR_OFF }, // Colour for "OCTAVE",
  { FX_MODULATE_COLOUR_ON, FX_MODULATE_COLOUR_OFF }, // Colour for "PHASER",
  { FX_MODULATE_COLOUR_ON, FX_MODULATE_COLOUR_OFF }, // Colour for "FLANGER",
  { FX_MODULATE_COLOUR_ON, FX_MODULATE_COLOUR_OFF }, // Colour for "TREMOLO",
  { FX_MODULATE_COLOUR_ON, FX_MODULATE_COLOUR_OFF }, // Colour for "ROTARY",
  { FX_MODULATE_COLOUR_ON, FX_MODULATE_COLOUR_OFF }, // Colour for "UNI-V",
  { FX_MODULATE_COLOUR_ON, FX_MODULATE_COLOUR_OFF }, // Colour for "PAN",
  { FX_DELAY_COLOUR_ON, FX_DELAY_COLOUR_OFF }, // Colour for "DELAY",
  { FX_MODULATE_COLOUR_ON, FX_MODULATE_COLOUR_OFF }, // Colour for "CHORUS",
  { FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF } // Colour for "EQ"
};

// Looks for the on_colour as specified in the VG99_parameters array.
void GR55_target_lookup(uint8_t current_assign) {
  uint8_t part;
  uint16_t target; // the address of the current assign target

  target = GR55_ctls[current_assign].assign_target;
  part = 0; // No parts yet

  // Lookup in array
  for (uint8_t i = 0; i < GR55_PARAMETERS_SIZE; i++) {
    //if (GR55_parameters[part][i].address == 0) break; //Break the loop if there is no more useful data
    if (target == GR55_parameters[part][i].target) { //Check is we've found the right target
      // Read out the on address
      GR55_ctls[current_assign].target_address = GR55_parameters[part][i].address;
    }
  }
}

// Will lookup parameter name in a table- for the fx1 and fx2 types it will also mention the type of effect.
void GR55_display_parameter(uint16_t target, uint8_t type, uint8_t number) {
  String msg, msg1, msg2;
  uint8_t part;
  uint8_t sublist;

  msg = GR55_ctls[number].name;

  part = 0; // As the array is divided in addresses by 1000, it is easy to find the right part
  // Lookup in array
  for (uint8_t i = 0; i < GR55_PARAMETERS_SIZE; i++) {
    //if (GR55_parameters[part][i].address == 0) break; //Break the loop if there is no more useful data
    if (target == GR55_parameters[part][i].target) { //Check is we've found the right target
      if (GR55_parameters[part][i].sublist == 0) { // If sublist number is zero, we can just go and display the name
        msg = GR55_parameters[part][i].name;
      }
      else {                                       // If the sublist number is not zero, we will combine the name with the FX name
        sublist = GR55_parameters[part][i].sublist - 1;
        msg1 = GR55_parameters[part][i].name;
        msg2 = GR55_sublists[sublist][type]; // Lookup the name of the FX type in the GR55_sublists array
        msg = msg1 + " " + msg2 + " SW";
      }
    }
  }
  show_status_message(msg);
}

void GR55_find_colours(uint8_t current_assign) {

  uint8_t on_colour = GR55_STOMP_COLOUR_ON; // Set the default on colour
  uint8_t off_colour = GR55_STOMP_COLOUR_OFF; // Set the default on colour
  bool reversed = false; //Default is LEDs are not reversed

  uint8_t part;
  uint16_t target; // the address of the current assign target
  uint8_t type;
  //uint16_t address = 0;

  target = GR55_ctls[current_assign].assign_target;
  type = GR55_ctls[current_assign].target_byte2;
  part = 0; // No parts yet

  for (uint8_t i = 0; i < GR55_PARAMETERS_SIZE; i++) {
    //if (GR55_parameters[part][i].address == 0) break; //Break the loop if there is no more useful data
    if (target == GR55_parameters[part][i].target) { //Check is we've found the right target
      // Read out the on colour
      on_colour = GR55_parameters[part][i].colour_on;
      if (on_colour == GR55_MFX_COLOUR_ON) on_colour = GR55_MFX_colours[type][0]; // For FX1 and FX2 pick the colour from the FX_colours array
      if (on_colour == GR55_MOD_COLOUR_ON) on_colour = GR55_MOD_colours[type][0]; // For FX1 and FX2 pick the colour from the FX_colours array
      // Read out the off colour
      off_colour = GR55_parameters[part][i].colour_off;
      if (off_colour == GR55_MFX_COLOUR_OFF) off_colour = GR55_MFX_colours[type][1]; // For FX1 and FX2 pick the colour from the FX_colours array
      if (off_colour == GR55_MOD_COLOUR_OFF) off_colour = GR55_MOD_colours[type][1]; // For FX1 and FX2 pick the colour from the FX_colours array
      // Read reversed setting
      reversed = GR55_parameters[part][i].reversed;
    }
  }
  if (GR55_ctls[current_assign].assign_max > GR55_ctls[current_assign].assign_min) {
    GR55_ctls[current_assign].colour_on = on_colour; // Store the colours in the assign array
    GR55_ctls[current_assign].colour_off = off_colour;
  }
  else {
    GR55_ctls[current_assign].colour_on = off_colour; // Store the colours reversed when assign_min and max are reversed as well
    GR55_ctls[current_assign].colour_off = on_colour;
  }
  GR55_ctls[current_assign].reversed = reversed;
}
