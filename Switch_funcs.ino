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

void main_switch_funcs() {
  update_tap_tempo_LED();
}

// Common switch functions

void nothing() { // A dummy function that besically does nothing

}

// Mode select functions
void select_mode(uint8_t new_mode) {
  previous_mode = mode; // Store the mode we come from...
  mode = new_mode;
  EEPROM.write(EEPROM_mode, mode);
}

void toggle_mode(uint8_t mode1, uint8_t mode2) {
  if (mode == mode1) select_mode(mode2);
  else select_mode(mode1);
}

// ************************ Start Global tuner ************************
// Call start_global_tuner()

void start_global_tuner() {
  // LED control of global tuner is done directly in the LED section
  mode_before_tuning = mode; // Remember the mode we came from
  mode = MODE_TUNER;

  write_GP10(GP10_TUNER_ON); // Start tuner on GP-10
  write_VG99(VG99_TUNER_ON); // Start tuner on VG-99
  GR55_mute_now(); //Mute the GR55
}

void stop_global_tuner() {
  mode = mode_before_tuning; // Return to previous mode
  if (mode == MODE_TUNER) mode = MODE_GP10_PATCH; // just so we won't get stuck in tuner mode...
  write_GP10(GP10_TUNER_OFF); // Stop tuner on GP-10
  write_VG99(VG99_TUNER_OFF); // Start tuner on VG-99
  GR55_unmute(); //Unmute the GR55
}

// ************************ Bank up and bank down ************************
// Call bankup() or bank-down()

// Some boolean values for the bank up/down code
#define UP true
#define DOWN false

void bank_up() {
  // check the mode we are in and call the right function below to perform the bank up
  if (mode == MODE_GP10_PATCH) {
    bank_size = 10;
    GP10_bank_updown(UP);
  }
  if (mode == MODE_GR55_PATCH) {
    bank_size = 9;
    GR55_bank_updown(UP);
  }
  if (mode == MODE_VG99_PATCH) {
    bank_size = 10;
    VG99_bank_updown(UP);
  }
  if ((mode == MODE_GP10_GR55_COMBI) && (mode_GP10_GR55_combo_bank_change_on_GR55 == false)) {
    bank_size = 5;
    GP10_bank_updown(UP);
  }
  if ((mode == MODE_GP10_GR55_COMBI) && (mode_GP10_GR55_combo_bank_change_on_GR55 == true)) {
    bank_size = 6;
    GR55_bank_updown(UP);
  }
}

void bank_down() {
  // check the mode we are in and call the right function below to perform the bank up
  if (mode == MODE_GP10_PATCH) {
    bank_size = 10;
    GP10_bank_updown(DOWN);
  }
  if (mode == MODE_GR55_PATCH) {
    bank_size = 9;
    GR55_bank_updown(DOWN);
  }
  if (mode == MODE_VG99_PATCH) {
    bank_size = 10;
    VG99_bank_updown(DOWN);
  }
  if ((mode == MODE_GP10_GR55_COMBI) && (mode_GP10_GR55_combo_bank_change_on_GR55 == false)) {
    bank_size = 5;
    GP10_bank_updown(DOWN);
  }
  if ((mode == MODE_GP10_GR55_COMBI) && (mode_GP10_GR55_combo_bank_change_on_GR55 == true)) {
    bank_size = 6;
    GR55_bank_updown(DOWN);
  }
}

void GP10_bank_updown(bool updown) {
  uint8_t bank_number = (GP10_patch_number / bank_size);

  if (GP10_bank_selection_active == false) {
    GP10_bank_selection_active = true;
    GP10_bank_number = bank_number; //Reset the bank to current patch
  }
  // Perform bank up:
  if (updown == UP) {
    if (GP10_bank_number >= ((GP10_PATCH_MAX / bank_size) - 1)) GP10_bank_number = (GP10_PATCH_MIN / bank_size); // Check if we've reached the top
    else GP10_bank_number++; //Otherwise move bank up
  }
  // Perform bank down:
  if (updown == DOWN) {
    if (GP10_bank_number <= (GP10_PATCH_MIN / bank_size)) GP10_bank_number = GP10_PATCH_MAX / bank_size; // Check if we've reached the bottom
    else GP10_bank_number--; //Otherwise move bank down
  }

  if (GP10_bank_number == bank_number) GP10_bank_selection_active = false; //Check whether were back to the original bank
}

void GR55_bank_updown(bool updown) {
  uint8_t no_of_banks = bank_size / 3;
  uint8_t bank_number = ((GR55_patch_number / bank_size) * no_of_banks);

  if (GR55_bank_selection_active == false) {
    GR55_bank_selection_active = true;
    GR55_bank_number = bank_number; //start with current bank number
  }
  // Perform bank up:
  if (updown == UP) {
    if (GR55_bank_number >= (GR55_BANK_MAX - no_of_banks)) GR55_bank_number = GR55_BANK_MIN; // Check if we've reached the top
    else GR55_bank_number = GR55_bank_number + no_of_banks; //Otherwise move three banks up

  }
  // Perform bank down:
  if (updown == DOWN) {
    if (GR55_bank_number < (GR55_BANK_MIN + 1)) GR55_bank_number = GR55_BANK_MAX - no_of_banks; // Check if we've reached the bottom
    else GR55_bank_number = GR55_bank_number - no_of_banks; //Otherwise move three banks down
  }

  if (GR55_bank_number == bank_number) GR55_bank_selection_active = false; //Check whether were back to the original bank
}

void VG99_bank_updown(bool updown) {
  if (VG99_bank_selection_active == false) {
    VG99_bank_selection_active = true;
    VG99_bank_number = (VG99_patch_number / bank_size); //Reset the bank to current patch
  }
  // Perform bank up:
  if (updown == UP) {
    if (VG99_bank_number >= VG99_BANK_MAX) VG99_bank_number = VG99_BANK_MIN; // Check if we've reached the top
    else VG99_bank_number++; //Otherwise move bank up

  }
  // Perform bank down:
  if (updown == DOWN) {
    if (VG99_bank_number <= VG99_BANK_MIN) VG99_bank_number = VG99_BANK_MAX; // Check if we've reached the bottom
    else VG99_bank_number--; //Otherwise move bank down
  }

  if (VG99_bank_number == (VG99_patch_number / bank_size)) VG99_bank_selection_active = false; //Check whether were back to the original bank
}

// ************************ Start Global tap tempo ************************
// Call global_tap_tempo()
// We only support bpms from 40 to 250:
#define MIN_TIME 240000 // (60.000.000 / 250 bpm)
#define MAX_TIME 1500000 // (60.000.000 / 40 bpm)

#define NUMBER_OF_TAPMEMS 5 // Tap tempo will do the average of five
uint32_t tap_time[NUMBER_OF_TAPMEMS];
uint8_t tap_time_index = 0;
uint32_t new_time, time_diff, avg_time;
uint32_t prev_time = 0;

void global_tap_tempo() {

  new_time = micros(); //Store the current time
  time_diff = new_time - prev_time;
  prev_time = new_time;
  DEBUGMSG("Tap no:" + String(tap_time_index) + " " + String(time_diff));

  // If time difference between two taps is too short or too long, we will start new tapping sequence
  if ((time_diff < MIN_TIME) || (time_diff > MAX_TIME)) {
    tap_time_index = 0;
  }
  else {

    // Shift tapmems to the left if neccesary
    if (tap_time_index >= NUMBER_OF_TAPMEMS) {
      for (uint8_t i = 1; i < NUMBER_OF_TAPMEMS; i++) {
        tap_time[i - 1] = tap_time[i];
      }
      tap_time_index--;  // A little wild, but now it works!
    }

    // Store time difference in memory
    tap_time[tap_time_index] = time_diff;

    //Calculate the average time
    //First add all the valid times up
    uint32_t total_time = 0;
    for (uint8_t j = 0; j <= tap_time_index; j++) {
      total_time = total_time + tap_time[j];
      Serial.print(String(tap_time[j]) + " ");
    }
    //Then calculate the average time
    avg_time = total_time / (tap_time_index + 1);
    bpm = ((60000000 + (avg_time / 2)) / avg_time); // Calculate the bpm
    EEPROM.write(EEPROM_bpm, bpm);  // Store it in EEPROM
    DEBUGMSG(" tot:" + String(total_time) + " avg:" + String(avg_time));

    // Send it to the devices
    GP10_send_bpm();
    GR55_send_bpm();
    VG99_send_bpm();

    // Move to the next memory slot
    tap_time_index++;
  }
  show_status_message("Tempo " + String(bpm) + " bpm");
  reset_tap_tempo_LED();
}

#define BPM_LED_ON_TIME 50 // The time the bpm LED is on in msec
#define BPM_LED_ADJUST 1   // LED is running a little to slow. This is an adjustment of a few msecs
uint32_t bpm_LED_timer = 0;
uint32_t bpm_LED_timer_length = BPM_LED_ON_TIME;

void update_tap_tempo_LED() {

  // Check if timer needs to be set
  if (bpm_LED_timer == 0) {
    bpm_LED_timer = millis();
  }

  // Check if timer runs out
  if (millis() - bpm_LED_timer > bpm_LED_timer_length) {
    bpm_LED_timer = millis(); // Reset the timer

    // If LED is currently on
    if (global_tap_tempo_LED == BPM_COLOUR_ON) {
      global_tap_tempo_LED = BPM_COLOUR_OFF;  // Turn the LED off
      VG99_TAP_TEMPO_LED_OFF();
      bpm_LED_timer_length = (60000 / bpm) - BPM_LED_ON_TIME - BPM_LED_ADJUST; // Set the time for the timer
    }
    else {
      global_tap_tempo_LED = BPM_COLOUR_ON;   // Turn the LED on
      VG99_TAP_TEMPO_LED_ON();
      bpm_LED_timer_length = BPM_LED_ON_TIME; // Set the time for the timer
    }
    update_LEDS = true;
  }
}

void reset_tap_tempo_LED() {
  bpm_LED_timer = millis();
  global_tap_tempo_LED = BPM_COLOUR_ON;    // Turn the LED on
  //VG99_TAP_TEMPO_LED_ON();
  bpm_LED_timer_length = BPM_LED_ON_TIME;  // Set the time for the timer
}

// Bass mode: sends a CC message with the number of the lowest string that is being played.
// By making smart assigns on a device, you can hear just the bass note played
#define GUITAR_TO_MIDI_CHANNEL 1 //The MIDI channel of the first string
#define BASS_CC_NUMBER 15 //The CC number for the bass control
#define BASS_CC_CHANNEL GR55_MIDI_channel //The MIDI channel on which this cc must be sent
#define BASS_CC_PORT GR55_MIDI_port //The MIDI port on which this cc must be sent
#define BASS_MIN_VEL 100 // Minimum velocity - messages below it ae ignored
uint8_t bass_string = 0; //remembers the current midi channel

void bass_mode_note_on(byte channel, byte note, byte velocity) {
  if ((channel >= GUITAR_TO_MIDI_CHANNEL) && (channel <= GUITAR_TO_MIDI_CHANNEL + 6)) {
    uint8_t string_played = channel - GUITAR_TO_MIDI_CHANNEL + 1;
    if ((string_played > bass_string) && (velocity >= BASS_MIN_VEL)) {
      bass_string = string_played; //Set the bass play channel to the current channel
      if (BASS_CC_PORT == USBMIDI_PORT) usbMIDI.sendControlChange(BASS_CC_NUMBER , bass_string, BASS_CC_CHANNEL);
      if (BASS_CC_PORT == MIDI1_PORT) MIDI1.sendControlChange(BASS_CC_NUMBER , bass_string, BASS_CC_CHANNEL);
      if (BASS_CC_PORT == MIDI2_PORT) MIDI2.sendControlChange(BASS_CC_NUMBER , bass_string, BASS_CC_CHANNEL);
      if (BASS_CC_PORT == MIDI3_PORT) MIDI3.sendControlChange(BASS_CC_NUMBER , bass_string, BASS_CC_CHANNEL);
    }
  }
}

void bass_mode_note_off(byte channel, byte note, byte velocity) {
  if ((channel >= GUITAR_TO_MIDI_CHANNEL) && (channel <= GUITAR_TO_MIDI_CHANNEL + 6)) {
    uint8_t string_played = channel - GUITAR_TO_MIDI_CHANNEL + 1;
    if (string_played == bass_string) bass_string = 0; //Reset the bass play channel
  }
}
