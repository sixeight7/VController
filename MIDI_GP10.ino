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
#define GP10_HARMONIST_KEY 0x20014001 // Sets the key for the harmonist, which is the only effect that is key related.

#define GP10_FOOT_VOL 0x20020803 // The address of the footvolume - values between 0 and 100
#define GP10_COSM_GUITAR_SW 0x20001000 // The address of the COSM guitar switch
#define GP10_NORMAL_PU_SW 0x20000804 // The address of the COSM guitar switch
bool GP10_request_onoff = false;

uint8_t GP10_MIDI_port = 0; // What port is the GP10 connected to (0 - 3)

uint8_t GP10_current_stomp = 255; //Keeps track of which stomp to read

#define GP10_SYSEX_DELAY_LENGTH 10 // time between sysex messages (in msec)
unsigned long GP10sysexDelay = 0;


uint8_t GP10_FX_toggle_LED = 0; // The LED shown for the FX type button

// ********************************* Section 2: GP10 comon MIDI in functions ********************************************

void check_SYSEX_in_GP10(const unsigned char* sxdata, short unsigned int sxlength) {
#ifdef COMPILE_GP10

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
      GP10_request_guitar_switch_states(); // Now read the guitar states
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

    // Check if it is the guitar on/off states
    GP10_check_guitar_switch_states(sxdata, sxlength);

    // Check if it is some other stompbox function and copy the status to the right LED
    GP10_check_stompbox_states(sxdata, sxlength);
  }
#endif
}


void check_PC_in_GP10(byte channel, byte program) {
#ifdef COMPILE_GP10

  // Check the source by checking the channel
  if (channel == GP10_MIDI_channel) { // GP10 outputs a program change
    GP10_patch_number = program;
    GP10_do_after_patch_selection();
  }
#endif
}

void GP10_identity_check(const unsigned char* sxdata, short unsigned int sxlength) {
#ifdef COMPILE_GP10

  // Check if it is a GP-10
  if ((sxdata[6] == 0x05) && (sxdata[7] == 0x03) && (GP10_detected == false)) {
    GP10_detected = true;
    show_status_message("GP-10 detected  ");
    GP10_device_id = sxdata[2]; //Byte 2 contains the correct device ID
    GP10_MIDI_port = Current_MIDI_port; // Set the correct MIDI port for this device
    DEBUGMSG("GP-10 detected on MIDI port " + String(Current_MIDI_port));
    request_GP10(GP10_REQUEST_PATCH_NUMBER);
    write_GP10(GP10_EDITOR_MODE_ON); // Put the GP10 in EDITOR mode - otherwise tuner will not work
    GP10_do_after_patch_selection();
  }
#endif
}

// ********************************* Section 3: GP10 comon MIDI out functions ********************************************

void GP10_check_sysex_delay() { // Will delay if last message was within GP10_SYSEX_DELAY_LENGTH (10 ms)
#ifdef COMPILE_GP10

  while (millis() - GP10sysexDelay <= GP10_SYSEX_DELAY_LENGTH) {}
  GP10sysexDelay = millis();
#endif
}

void write_GP10(uint32_t address, uint8_t value) { // For sending one data byte
#ifdef COMPILE_GP10

  uint8_t *ad = (uint8_t*)&address; //Split the 32-bit address into four bytes: ad[3], ad[2], ad[1] and ad[0]
  uint8_t checksum = calc_checksum(ad[3] + ad[2] + ad[1] + ad[0] + value); // Calculate the Roland checksum
  uint8_t sysexmessage[15] = {0xF0, 0x41, GP10_device_id, 0x00, 0x00, 0x00, 0x05, 0x12, ad[3], ad[2], ad[1], ad[0], value, checksum, 0xF7};
  if (GP10_MIDI_port == USBMIDI_PORT) usbMIDI.sendSysEx(15, sysexmessage);
  if (GP10_MIDI_port == MIDI1_PORT) MIDI1.sendSysEx(14, sysexmessage);
  if (GP10_MIDI_port == MIDI2_PORT) MIDI2.sendSysEx(14, sysexmessage);
  if (GP10_MIDI_port == MIDI3_PORT) MIDI3.sendSysEx(14, sysexmessage);
  debug_sysex(sysexmessage, 15, "out(GP10)");
  //GP10_check_sysex_delay();
#endif
}

void write_GP10(uint32_t address, uint8_t value1, uint8_t value2) { // For sending two data bytes
#ifdef COMPILE_GP10

  uint8_t *ad = (uint8_t*)&address; //Split the 32-bit address into four bytes: ad[3], ad[2], ad[1] and ad[0]
  uint8_t checksum = calc_checksum(ad[3] + ad[2] + ad[1] + ad[0] + value1 + value2); // Calculate the Roland checksum
  uint8_t sysexmessage[16] = {0xF0, 0x41, GP10_device_id, 0x00, 0x00, 0x00, 0x05, 0x12, ad[3], ad[2], ad[1], ad[0], value1, value2, checksum, 0xF7};
  if (GP10_MIDI_port == USBMIDI_PORT) usbMIDI.sendSysEx(16, sysexmessage);
  if (GP10_MIDI_port == MIDI1_PORT) MIDI1.sendSysEx(15, sysexmessage);
  if (GP10_MIDI_port == MIDI2_PORT) MIDI2.sendSysEx(15, sysexmessage);
  if (GP10_MIDI_port == MIDI3_PORT) MIDI3.sendSysEx(15, sysexmessage);
  debug_sysex(sysexmessage, 16, "out(GP10)");
  //GP10_check_sysex_delay();
#endif
}

void request_GP10(uint32_t address, uint8_t no_of_bytes) {
#ifdef COMPILE_GP10
  
  uint8_t *ad = (uint8_t*)&address; //Split the 32-bit address into four bytes: ad[3], ad[2], ad[1] and ad[0]
  uint8_t checksum = calc_checksum(ad[3] + ad[2] + ad[1] + ad[0] +  no_of_bytes); // Calculate the Roland checksum
  uint8_t sysexmessage[18] = {0xF0, 0x41, GP10_device_id, 0x00, 0x00, 0x00, 0x05, 0x11, ad[3], ad[2], ad[1], ad[0], 0x00, 0x00, 0x00, no_of_bytes, checksum, 0xF7};
  if (GP10_MIDI_port == USBMIDI_PORT) usbMIDI.sendSysEx(18, sysexmessage);
  if (GP10_MIDI_port == MIDI1_PORT) MIDI1.sendSysEx(17, sysexmessage);
  if (GP10_MIDI_port == MIDI2_PORT) MIDI2.sendSysEx(17, sysexmessage);
  if (GP10_MIDI_port == MIDI3_PORT) MIDI3.sendSysEx(17, sysexmessage);
  debug_sysex(sysexmessage, 18, "out(GP10)");
  //GP10_check_sysex_delay();
#endif
}

uint8_t GP10_previous_patch_number = 0;
uint8_t GP10_patch_memory = 0;

void GP10_SendProgramChange(uint8_t new_patch) {
#ifdef COMPILE_GP10

  if (new_patch == GP10_patch_number) GP10_unmute();
  GP10_patch_number = new_patch;
  if (GP10_MIDI_port == USBMIDI_PORT) usbMIDI.sendProgramChange(new_patch, GP10_MIDI_channel);
  if (GP10_MIDI_port == MIDI1_PORT) MIDI1.sendProgramChange(new_patch, GP10_MIDI_channel);
  if (GP10_MIDI_port == MIDI2_PORT) MIDI2.sendProgramChange(new_patch, GP10_MIDI_channel);
  if (GP10_MIDI_port == MIDI3_PORT) MIDI3.sendProgramChange(new_patch, GP10_MIDI_channel);
  DEBUGMSG("out(GP10) PC" + String(new_patch)); //Debug
  GR55_mute();
  VG99_mute();
  GP10_do_after_patch_selection();
#endif
}

void GP10_do_after_patch_selection() {
#ifdef COMPILE_GP10

  GP10_current_stomp = 255; // Stop reading stomps in case we came from there
  GP10_request_onoff = false;
  GP10_on = true;
  GP10_request_name(); // After name is read the guitar states will be read
  // And then the stromps will be requested
  //Request_GP10_first_stomp();
  if (SEND_GLOBAL_TEMPO_AFTER_PATCH_CHANGE == true) GP10_send_bpm();
  update_LEDS = true;
  update_lcd = true;
  EEPROM.write(EEPROM_GP10_PATCH_NUMBER, GP10_patch_number);
#endif
}

void GP10_request_patch_number() {
  #ifdef COMPILE_GP10

  request_GP10(GP10_REQUEST_PATCH_NUMBER);
#endif
}

void GP10_request_name() {
  #ifdef COMPILE_GP10

  GP10_patch_name = "                ";
  request_GP10(GP10_REQUEST_PATCH_NAME);
#endif
}


void GP10_send_bpm() {
#ifdef COMPILE_GP10

  write_GP10(GP10_TEMPO, bpm / 16, bpm % 16); // Tempo is modulus 16. It's all so very logical. NOT.
#endif
}

// ********************************* Section 4: GP10 stompbox functions ********************************************

// ** US-20 simulation
// Selecting and muting the GP10 is done by storing the settings of COSM guitar switch and Normal PU switch
// and switching both off when guitar is muted and back to original state when the GP10 is selected

void GP10_request_guitar_switch_states() {
  #ifdef COMPILE_GP10

  GP10_select_LED = GP10_PATCH_COLOUR; //Switch the LED on
  request_GP10(GP10_COSM_GUITAR_SW, 1);
  request_GP10(GP10_NORMAL_PU_SW, 1);
  GP10_request_onoff = true;
#endif
}

void GP10_check_guitar_switch_states(const unsigned char* sxdata, short unsigned int sxlength) {
#ifdef COMPILE_GP10

  if (GP10_request_onoff == true) {
    uint32_t address = (sxdata[8] << 24) + (sxdata[9] << 16) + (sxdata[10] << 8) + sxdata[11]; // Make the address 32 bit
    if (address == GP10_COSM_GUITAR_SW) {
      GP10_COSM_onoff = sxdata[12];  // Store the value
    }

    if (address == GP10_NORMAL_PU_SW) {
      GP10_nrml_pu_onoff = sxdata[12];  // Store the value
      GP10_request_onoff = false;
      Request_GP10_first_stomp(); //Now read the GP10 stomps
    }
  }
#endif
}

void GP10_select_switch() {
#ifdef COMPILE_GP10

  if ((GP10_on) && (!US20_emulation_state_changed)) {
    GP10_always_on_toggle();
  }
  else {
    GP10_unmute();
    GR55_mute();
    VG99_mute();
    if (mode != MODE_GP10_GR55_COMBI) show_status_message(GP10_patch_name); // Show the correct patch name
    US20_emulation_state_changed = false;
  }
#endif
}

void GP10_always_on_toggle() {
#ifdef COMPILE_GP10

GP10_always_on = !GP10_always_on; // Toggle GP10_always_on
  if (GP10_always_on) {
    GP10_unmute();
    show_status_message("GP10 always ON");
  }
  else {
    //GP10_mute();
    show_status_message("GP10 can be muted");
    US20_emulation_state_changed = true;
  }
#endif
}


void GP10_unmute() {
#ifdef COMPILE_GP10

  GP10_on = true;
  GP10_select_LED = GP10_PATCH_COLOUR; //Switch the LED on
  //write_GP10(GP10_FOOT_VOL, 100); // Switching guitars does not work - the wrong values are read from the GP-10. ?????
  write_GP10(GP10_COSM_GUITAR_SW, GP10_COSM_onoff); // Switch COSM guitar on
  write_GP10(GP10_NORMAL_PU_SW, GP10_nrml_pu_onoff); // Switch normal pu on
#endif
}

void GP10_mute() {
#ifdef COMPILE_GP10

  if (GP10_always_on == false) {
    GP10_on = false;
    GP10_select_LED = GP10_OFF_COLOUR; //Switch the LED off
    //write_GP10(GP10_FOOT_VOL, 0);
    write_GP10(GP10_COSM_GUITAR_SW, 0x00); // Switch COSM guitar off
    write_GP10(GP10_NORMAL_PU_SW, 0x00); // Switch normal pu off
  }
#endif
}


// Here we define some stompboxes for fixed parameters of the GP10

// Format of this stompbox array is:
// {SYSEX_ADDRESS, NAME FOR DISPLAY, ON_COLOUR, OFF_COLOUR, START_COLOUR}
struct stomper { // Combines all the data we need for controlling a parameter in a device
  uint32_t address;
  char name[17];
  uint8_t sublist; // Which sublist to read for the FX or amp type - 0 if second byte does not contain the type or if there is no sublist
  uint8_t type; // Type as read from the GP-10
  uint8_t colour_on;
  uint8_t colour_off;
  uint8_t LED;
};

//typedef struct stomper Stomper;
#define GP10_FX_COLOUR_ON 254 // Just a colour number to pick the colour from the GP10_FX_colours table
#define GP10_FX_COLOUR_OFF 255 // Just a colour number to pick the colour from the GP10_FX_colours table

// Make sure you edit the GP10_NUMBER_OF_STOMPS when adding new parameters
#define GP10_NUMBER_OF_STOMPS 6

stomper GP10_stomps[GP10_NUMBER_OF_STOMPS] = {
  {0x2000500B, "Amp solo", 0, 0, FX_AMP_COLOUR_ON, FX_AMP_COLOUR_OFF, FX_AMP_COLOUR_OFF},
  {0x20005800, "FX", 1, 0, GP10_FX_COLOUR_ON, GP10_FX_COLOUR_OFF, GP10_STOMP_COLOUR_OFF},
  {0x10001000, "Guitar2MIDI", 0, 0, GP10_STOMP_COLOUR_ON, GP10_STOMP_COLOUR_OFF, GP10_STOMP_COLOUR_OFF},
  {0x20016800, "Chorus", 0, 0, FX_MODULATE_COLOUR_ON, FX_MODULATE_COLOUR_OFF, FX_MODULATE_COLOUR_OFF},
  {0x20017800, "RVB", 3, 0, FX_REVERB_COLOUR_ON, FX_REVERB_COLOUR_OFF, FX_REVERB_COLOUR_OFF},
  {0x20017000, "DLY", 2, 0, FX_DELAY_COLOUR_ON, FX_DELAY_COLOUR_OFF, FX_DELAY_COLOUR_OFF},
  /*{0x20001000, "COSM GUITAR", GP10_STOMP_COLOUR_ON, GP10_STOMP_COLOUR_OFF, GP10_STOMP_COLOUR_OFF},
  {0x20000804, "NORMAL PU", GP10_STOMP_COLOUR_ON, GP10_STOMP_COLOUR_OFF, GP10_STOMP_COLOUR_OFF},*/
};

#define GP10_FX_stomp 1 // Which of the above stompboxes is used to control the FX. Start counting from zero!

#define GP10_NO_OF_SUBLISTS 3
#define GP10_SIZE_OF_SUBLISTS 17
const PROGMEM char GP10_sublists[GP10_NO_OF_SUBLISTS][GP10_SIZE_OF_SUBLISTS][8] = {
  { "OD/DS", "COMPRSR", "LIMITER", "EQ", "T.WAH", "P.SHIFT", "HARMO", "P. BEND", "PHASER", "FLANGER", "TREMOLO", "PAN", "ROTARY", "UNI-V", "CHORUS", "DELAY" }, // Sublist 1: FX types
  { "SINGLE", "PAN", "STEREO", "DUAL-S", "DUAL-P", "DU L/R", "REVRSE", "ANALOG", "TAPE", "MODLTE"}, // Sublist 2: Delay types
  { "AMBNCE", "ROOM", "HALL1", "HALL2", "PLATE", "SPRING", "MODLTE"} // Sublist 3: Reverb types
};

uint8_t GP10_FX_colours[17][2] = {
  { FX_DIST_COLOUR_ON, FX_DIST_COLOUR_OFF }, // Colour for "OD/DS"
  { FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF }, // Colour for "COMPRSR"
  { FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF }, // Colour for "LIMITER"
  { FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF }, // Colour for "EQ"
  { FX_FILTER_COLOUR_ON, FX_FILTER_COLOUR_OFF }, // Colour for "T.WAH"
  { FX_PITCH_COLOUR_ON, FX_PITCH_COLOUR_OFF }, // Colour for "P.SHIFT"
  { FX_PITCH_COLOUR_ON, FX_PITCH_COLOUR_OFF }, // Colour for  "HARMO"
  { FX_PITCH_COLOUR_ON, FX_PITCH_COLOUR_OFF }, // Colour for "P. BEND"
  { FX_MODULATE_COLOUR_ON, FX_MODULATE_COLOUR_OFF }, // Colour for "PHASER"
  { FX_MODULATE_COLOUR_ON, FX_MODULATE_COLOUR_OFF }, // Colour for "FLANGER"
  { FX_MODULATE_COLOUR_ON, FX_MODULATE_COLOUR_OFF }, // Colour for "TREMOLO"
  { FX_MODULATE_COLOUR_ON, FX_MODULATE_COLOUR_OFF }, // Colour for "PAN"
  { FX_MODULATE_COLOUR_ON, FX_MODULATE_COLOUR_OFF }, // Colour for  "ROTARY"
  { FX_MODULATE_COLOUR_ON, FX_MODULATE_COLOUR_OFF }, // Colour for "UNI-V"
  { FX_MODULATE_COLOUR_ON, FX_MODULATE_COLOUR_OFF }, // Colour for "CHORUS"
  { FX_DELAY_COLOUR_ON, FX_DELAY_COLOUR_OFF }, // Colour for "DELAY"
};

// Sends requests to the GP10 to send the current settings of all the GP10_stomps
void Request_GP10_first_stomp() {
#ifdef COMPILE_GP10
  GP10_current_stomp = 0; //After the name is read, the assigns can be read
  GP10_request_next_stomp();
#endif
}

void GP10_request_next_stomp() {
#ifdef COMPILE_GP10

  if (GP10_current_stomp < GP10_NUMBER_OF_STOMPS) {
    request_GP10(GP10_stomps[GP10_current_stomp].address, 2); // We read two bytes, because the second one often contains the FX type
  }
#endif
}

// Reads out the parameters that were requested with GP10_request_stompbox_states()
void GP10_check_stompbox_states(const unsigned char* sxdata, short unsigned int sxlength) {
#ifdef COMPILE_GP10
  uint32_t address = (sxdata[8] << 24) + (sxdata[9] << 16) + (sxdata[10] << 8) + sxdata[11]; // Make the address 32 bit
  for (uint8_t addr_count = 0; addr_count < GP10_NUMBER_OF_STOMPS; addr_count++) {
    if (address == GP10_stomps[addr_count].address) {
      if (sxlength > 15) GP10_stomps[addr_count].type = sxdata[13]; // Copy the FX type if it is being sent. Changing the parameter also sends this data, but shorter
      // Copy status to the LED - but check if it is not FX
      if (sxdata[12] == 0x01) GP10_stomps[addr_count].LED = GP10_stomp_LED_on_colour(addr_count); // Switch the LED on
      if (sxdata[12] == 0x00) GP10_stomps[addr_count].LED = GP10_stomp_LED_off_colour(addr_count); // Switch the LED off
      update_LEDS = true;
      DEBUGMSG("Stompbox:" + String(addr_count) + " Type:" + String(sxdata[13]));
      if (GP10_current_stomp == GP10_FX_stomp) GP10_set_FX_LEDS(); // Set LEDs of FX stomps when neccesary
      if (GP10_current_stomp < GP10_NUMBER_OF_STOMPS) GP10_current_stomp++; //Move to the next stomp if we have to...
    }
  }
#endif
}

// Toggle GP10 stompbox parameter
void GP10_stomp(uint8_t number) {
#ifdef COMPILE_GP10

  if (number == GP10_FX_stomp) GP10_FX_type_select(); // If we are toggling FX, first select the right type
  if (GP10_stomps[number].LED == GP10_stomp_LED_off_colour(number)) {
    GP10_stomps[number].LED = GP10_stomp_LED_on_colour(number); // Switch the LED on with the GP10 stomp colour
    write_GP10(GP10_stomps[number].address, 0x01);
    String msg = GP10_stomps[number].name;
    if (GP10_stomps[number].sublist > 0) { // Check if a sublist exists
      String type_name = GP10_sublists[GP10_stomps[number].sublist - 1][GP10_stomps[number].type];
      msg = msg + " (" + type_name + ")";
    }
    show_status_message(msg + " ON");
  }
  else {
    GP10_stomps[number].LED = GP10_stomp_LED_off_colour(number); // Switch the LED off with the GP10 stomp colour
    write_GP10(GP10_stomps[number].address, 0x00);
    String msg = GP10_stomps[number].name;
    if (GP10_stomps[number].sublist > 0) { // Check if a sublist exists
      String type_name = GP10_sublists[GP10_stomps[number].sublist - 1][GP10_stomps[number].type];
      msg = msg + " (" + type_name + ")";
    }
    show_status_message(msg + " OFF");
  }
#endif
}

#ifdef COMPILE_GP10
// Finds the right colour for an FX LED that is on
uint8_t GP10_stomp_LED_on_colour(uint8_t number) {
  if (GP10_stomps[number].colour_on == GP10_FX_COLOUR_ON) { // Check if we have to pick the colour from the GP10_FX_colours table
    return GP10_FX_colours[GP10_stomps[number].type][0];
  }
  else {
    return GP10_stomps[number].colour_on; // Switch the LED on with the normal GP10 stomp colour
  }
}

// Finds the right colour for an FX LED that is off
uint8_t GP10_stomp_LED_off_colour(uint8_t number) {
  if (GP10_stomps[number].colour_off == GP10_FX_COLOUR_OFF) { // Check if we have to pick the colour from the GP10_FX_colours table
    return GP10_FX_colours[GP10_stomps[number].type][1];
  }
  else {
    return GP10_stomps[number].colour_off; // Switch the LED on with the normal GP10 stomp colour
  }
}
#endif

// Buttons to set the FX to a specific type
#define NUMBER_OF_FX_TYPE_LEDS 16
uint8_t GP10_fx_type_LEDs[NUMBER_OF_FX_TYPE_LEDS]; //One LED for every fx LED (16 max)

// GP10_fx_type_button sets the FX block of the GP-10 to the specified effect and switches it on
// FX type numbers:
// 0:OD/DS, 1:COMPRSR, 2:LIMITER, 3:EQ, 4:T.WAH, 5:P.SHIFT, 6:HARMO, 7:P. BEND, 8:PHASER, 9:FLANGER, 10:TREMOLO, 11:PAN, 12:ROTARY, 13:UNI-V, 14:CHORUS, 15:DELAY
void GP10_fx_type_button(uint8_t type) {
#ifdef COMPILE_GP10
  // Set the FX type
  uint8_t previous_type = GP10_stomps[GP10_FX_stomp].type;
  if (type == previous_type) {
    // Toggle the FX type on/off
    GP10_stomp(GP10_FX_stomp);
  }
  else {
    // Set the FX type to type
    write_GP10(0x20005800, 0x01, type); // Switch FX on and set the type.
    // Update the GP10_stomps array with the current settings
    GP10_stomps[GP10_FX_stomp].type = type; // Update the array
    GP10_stomps[GP10_FX_stomp].LED = GP10_stomp_LED_on_colour(GP10_FX_stomp); // Update the LED in the array on with the GP10 stomp colour
    GP10_FX_type_lookup(type); // This will update the FX pointer used in the second method
    // Display message
    String msgtype = GP10_sublists[0][type];
    show_status_message("FX:" + msgtype + " ON");
  }
  // Update the LEDs
  GP10_fx_type_LEDs[previous_type] = GP10_FX_colours[previous_type][1]; // Switches off the old LED
  GP10_fx_type_LEDs[type] = GP10_FX_colours[type][0]; // New LED on
#endif
}

void GP10_set_FX_LEDS() {
#ifdef COMPILE_GP10
  for (uint8_t i = 0; i < NUMBER_OF_FX_TYPE_LEDS; i++) {
    GP10_fx_type_LEDs[i] = GP10_FX_colours[i][1]; //Set the FX colours to the off-state
  }
  uint8_t type = GP10_stomps[GP10_FX_stomp].type;
  GP10_fx_type_LEDs[type] = GP10_FX_colours[type][0]; // Current LED on
  GP10_FX_type_lookup(type); // Also LED in second method on
#endif
}

// Second implemantation of FX type. This is one button, that will toggle through a number of FX types
#define GP10_FX_NUMBER_OF_SELECTABLE_FX 7
uint8_t GP10_FX[GP10_FX_NUMBER_OF_SELECTABLE_FX] = {0, 4, 8, 9, 10, 12, 13}; //<= Set these to the effects you want to be able to select

uint8_t GP10_FX_pointer = 0; // Pointer to he current selected FX
bool GP10_FX_selection_active = false;

void GP10_FX_toggle_button() {
#ifdef COMPILE_GP10

  //Check if we are already in selection mode
  if (!GP10_FX_selection_active) {
    GP10_FX_selection_active = true; //if not switch it on
  }
  else {
    // Select the next FX
    GP10_FX_pointer++;
    if (GP10_FX_pointer >= GP10_FX_NUMBER_OF_SELECTABLE_FX) GP10_FX_pointer = 0;
  }
  // Update display and LED
  uint8_t type = GP10_FX[GP10_FX_pointer]; // Lookup the FX type in the array
  String msgtype = GP10_sublists[0][type];
  show_status_message("FX type: " + msgtype);
  GP10_FX_toggle_LED = GP10_FX_colours[type][0]; // Set LED to the right colour
#endif
}

void GP10_FX_type_select() {
#ifdef COMPILE_GP10

  if (GP10_FX_selection_active) {
    uint8_t type = GP10_FX[GP10_FX_pointer]; // Lookup the FX type in the array
    // Set the FX type to type
    write_GP10(0x20005801, type); // Set the FX type.
    // Update the GP10_stomps array with the current settings
    GP10_stomps[GP10_FX_stomp].type = type; // Update the array
    GP10_stomps[GP10_FX_stomp].LED = GP10_stomp_LED_off_colour(GP10_FX_stomp); //We'll switch the LED off - so it can be switched on by the GP10_stomp function
    GP10_FX_selection_active = false;
  }
#endif
}

// Will set the GP10_FX_pointer to the effect that is equal to type
void GP10_FX_type_lookup(uint8_t type) {
#ifdef COMPILE_GP10
  GP10_FX_toggle_LED = 0; // Switch it off, in case we don't have the selected FX in the array
  for (uint8_t i = 0; i < GP10_FX_NUMBER_OF_SELECTABLE_FX; i++) {
    if (GP10_FX[i] == type) {
      GP10_FX_pointer = i; // Set the pointer
      GP10_FX_toggle_LED = GP10_FX_colours[GP10_FX[i]][0]; // Set LED to the right colour
    }
  }
#endif
}


