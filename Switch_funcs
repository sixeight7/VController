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

// Common switch functions

void nothing() { // A dummy function that besically does nothing

}

// ************************ Start Global tuner ************************
// Call start_global_tuner()

void start_global_tuner() {

  previous_mode = mode; // Remember the mode we came from
  mode = MODE_TUNER;
  global_tuner_LED = GLOBAL_STOMP_COLOUR_ON;
  write_GP10(GP10_TUNER_ON); // Start tuner on GP-10
  write_VG99(VG99_TUNER_ON); // Start tuner on VG-99
}

void stop_global_tuner() {

  mode = previous_mode; // Return to previous mode
  global_tuner_LED = GLOBAL_STOMP_COLOUR_OFF;

  write_GP10(GP10_TUNER_OFF); // Stop tuner on GP-10
  write_VG99(VG99_TUNER_OFF); // Start tuner on VG-99

}

// ************************ Bank up and bank down ************************
// Call bankup() or bank-down()

// Some boolean values for the bank up/down code
#define UP true
#define DOWN false

void bank_up() {
  // check the mode we are in and call the right function below to perform the bank up
  if (mode == MODE_GP10_PATCH) GP10_bank_updown(UP);
  if (mode == MODE_GR55_PATCH) GR55_bank_updown(UP);
  if (mode == MODE_VG99_PATCH) VG99_bank_updown(UP);
}

void bank_down() {
  // check the mode we are in and call the right function below to perform the bank up
  if (mode == MODE_GP10_PATCH) GP10_bank_updown(DOWN);
  if (mode == MODE_GR55_PATCH) GR55_bank_updown(DOWN);
  if (mode == MODE_VG99_PATCH) VG99_bank_updown(DOWN);
}

void GP10_bank_updown(bool updown) {
  if (GP10_bank_selection_active == false) {
    GP10_bank_selection_active = true;
    GP10_bank_number = (GP10_patch_number / 10); //Reset the bank to current patch
  }
  // Perform bank up:
  if (updown == UP) {
    if (GP10_bank_number >= (GP10_BANK_MAX - 1)) GP10_bank_number = GP10_BANK_MIN; // Check if we've reached the top
    else GP10_bank_number++; //Otherwise move bank up

  }
  // Perform bank down:
  if (updown == DOWN) {
    if (GP10_bank_number <= GP10_BANK_MIN) GP10_bank_number = GP10_BANK_MAX; // Check if we've reached the bottom
    else GP10_bank_number--; //Otherwise move bank down
  }

  if (GP10_bank_number == (GP10_patch_number / 10)) GP10_bank_selection_active = false; //Check whether were back to the original bank
}

void GR55_bank_updown(bool updown) {
  if (GR55_bank_selection_active == false) {
    GR55_bank_selection_active = true;
    GR55_bank_number = ((GR55_patch_number / 9) * 3); //start with current bank number
  }
  // Perform bank up:
  if (updown == UP) {
    if (GR55_bank_number > (GR55_BANK_MAX - 4)) GR55_bank_number = GR55_BANK_MIN; // Check if we've reached the top
    else GR55_bank_number = GR55_bank_number + 3; //Otherwise move three banks up

  }
  // Perform bank down:
  if (updown == DOWN) {
    if (GR55_bank_number < (GR55_BANK_MIN + 1)) GR55_bank_number = GR55_BANK_MAX - 3; // Check if we've reached the bottom
    else GR55_bank_number = GR55_bank_number - 3; //Otherwise move three banks down
  }

  if (GR55_bank_number == GR55_patch_number / 3) GR55_bank_selection_active = false; //Check whether were back to the original bank
}

void VG99_bank_updown(bool updown) {
  if (VG99_bank_selection_active == false) {
    VG99_bank_selection_active = true;
    VG99_bank_number = (VG99_patch_number / 10); //Reset the bank to current patch
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

  if (VG99_bank_number == (VG99_patch_number / 10)) VG99_bank_selection_active = false; //Check whether were back to the original bank
}
