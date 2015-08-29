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

// Functions for LCD control
// LCD is a 16x2 LCD display with a serial i2c module attached

// The top line of the display shows patch numbers and mode
// The bottom line of the display shows patch names and status messages

#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>

// ************************ Settings you probably won't have to change ***********************
boolean update_lcd = true;
String GR55_patch_number_string = "U01-1";
String VG99_patch_number_string = "U001";

#define MESSAGE_TIMER_LENGTH 1500 // time that status messages are shown (in msec)
unsigned long messageTimer = 0;

// ***************************** Hardware settings *****************************
#define I2C_ADDR    0x27 // <<----- Add your address here.  Find it from I2C Scanner
#define BACKLIGHT_PIN     3
#define EN_PIN  2
#define RW_PIN  1
#define RS_PIN  0
#define D4_PIN  4
#define D5_PIN  5
#define D6_PIN  6
#define D7_PIN  7

LiquidCrystal_I2C	lcd(I2C_ADDR, EN_PIN, RW_PIN, RS_PIN, D4_PIN, D5_PIN, D6_PIN, D7_PIN);

void setup_LCD_control()
{
  lcd.begin (16, 2); //  <<----- My LCD was 16x2


  // Switch on the backlight
  lcd.setBacklightPin(BACKLIGHT_PIN, POSITIVE);
  lcd.setBacklight(HIGH);
  lcd.home (); // go home
  lcd.print("V-controller v1");  // Show startup message
  show_status_message("  by SixEight");  //Please give me the credits :-)
}

void main_LCD_control()
{
  // Display mode, unless a status message is being displayed
  if  ((update_lcd == true) && (millis() - messageTimer >= MESSAGE_TIMER_LENGTH)) {
    update_lcd = false;
    lcd.home();
    switch (mode) {
      case MODE_TUNER:
        lcd.print(" *** Tuner ***  ");
        lcd.setCursor (0, 1);       // go to start of 2nd line
        lcd.print("                ");
        break;
      case MODE_STOMP_1:
        lcd.print("Stompbox   GP-10");
        lcd.setCursor (0, 1);       // go to start of 2nd line
        lcd.print("                "); //Clear the line first
        lcd.setCursor (0, 1);       // go to start of 2nd line
        lcd.print(GP10_patch_name);
        break;
      case MODE_STOMP_2:
        lcd.print("Stompbox   GR-55");
        lcd.setCursor (0, 1);       // go to start of 2nd line
        lcd.print("                "); //Clear the line first
        lcd.setCursor (0, 1);       // go to start of 2nd line
        lcd.print(GR55_patch_name);
        break;
      case MODE_STOMP_3:
        lcd.print("Stompbox   VG-99");
        lcd.setCursor (0, 1);       // go to start of 2nd line
        lcd.print("                "); //Clear the line first
        lcd.setCursor (0, 1);       // go to start of 2nd line
        lcd.print(VG99_patch_name);
        break;
      case MODE_STOMP_4:
        lcd.print("FX select  GP-10");
        lcd.setCursor (0, 1);       // go to start of 2nd line
        lcd.print("                "); //Clear the line first
        lcd.setCursor (0, 1);       // go to start of 2nd line
        lcd.print(GP10_patch_name);
        break;

      // *************************************** GP10 LCD modes ***************************************
      case MODE_GP10_PATCH:
        lcd.home ();
        if (GP10_bank_selection_active == false) {
          lcd.print("P" + String((GP10_patch_number + 1) / 10) + String((GP10_patch_number + 1) % 10) + "        GP-10");
        }
        else {
          lcd.print("P" + String(GP10_bank_number) + "-        GP-10");
        }
        lcd.setCursor (0, 1);       // go to start of 2nd line
        lcd.print(GP10_patch_name + "    ");
        break;
      case MODE_GP10_DIRECTSELECT1:
        lcd.print("P__        GP-10");
        break;
      case MODE_GP10_DIRECTSELECT2:
        lcd.print("P" + String(GP10_bank_number) + "_        GP-10");
        break;

      // *************************************** GR55 LCD modes ***************************************
      case MODE_GR55_PATCH:
        display_GR55_patch_string();
        lcd.print(GR55_patch_number_string + "      GR-55");
        lcd.setCursor (0, 1);       // go to start of 2nd line
        lcd.print(GR55_patch_name);
        break;
      case MODE_GR55_DIRECTSELECT1:
        lcd.print("U__-_      GP-10");
        break;
      case MODE_GR55_DIRECTSELECT2:
        lcd.print("U" + String(GR55_bank_number / 10) + "_-_      GP-10");
        break;
      case MODE_GR55_DIRECTSELECT3:
        lcd.print("U" + String(GR55_bank_number / 10) + String(GR55_bank_number % 10) + "-_      GP-10");
        break;

      // *************************************** VG99 LCD modes ***************************************
      case MODE_VG99_PATCH:
        display_VG99_patch_string();
        lcd.home ();
        lcd.print(VG99_patch_number_string + "       VG-99");
        lcd.setCursor (0, 1);       // go to start of 2nd line
        lcd.print(VG99_patch_name + "    ");
        break;
      case MODE_VG99_DIRECTSELECT1:
        lcd.print("U0__       VG-99");
        break;
      case MODE_VG99_DIRECTSELECT2:
        lcd.print("U" + String(VG99_bank_number / 10) + String(VG99_bank_number % 10) + "_       VG-99");
        break;
      case MODE_COLOUR_MAKER:
        lcd.print("ColourMaker mode");
        lcd.setCursor (0, 1);       // go to start of 2nd line
        lcd.print("                "); //Clear the line first
        lcd.setCursor (0, 1);       // go to start of 2nd line
        lcd.print("R:" + String(colour_maker_red) + " G:" + String(colour_maker_green) + " B:" + String(colour_maker_blue));
        break;
        
      default:
        lcd.print("Status unknown  "); // Just in case you forgot to add some status
    }
  }
}

void show_status_message(String message)
{
  lcd.home();
  lcd.setCursor (0, 1);       // go to start of 2nd line
  lcd.print("                "); //Clear the line first
  lcd.setCursor (0, 1);       // go to start of 2nd line
  lcd.print(message);
  messageTimer = millis();
}

void display_GR55_patch_string() {
  // Uses GR55_patch_number as input and returns GR55_patch_number_string as output in format "U01-1"
  // First character is L for Lead, R for Rhythm, O for Other or U for User
  // In guitar mode GR55_preset_banks is set to 40, in bass mode it is set to 12, because there a less preset banks in bass mode.

  uint16_t patch_number_corrected = 0; // Need a corrected version of the patch number to deal with the funny numbering system of the GR-55
  uint16_t bank_number_corrected = 0; //Also needed, because with higher banks, we start counting again

  if (GR55_bank_selection_active == false) GR55_bank_number = (GR55_patch_number / 3); // Calculate the bank number

  if (GR55_bank_number < 100) {
    GR55_patch_number_string = "U";
    patch_number_corrected = GR55_patch_number;  //In the User bank all is normal
    bank_number_corrected = GR55_bank_number;
  }

  if (GR55_bank_number >= 99) {   // In the Lead bank we have to adjust the bank and patch numbers so we start with L01-1
    GR55_patch_number_string = "L";
    patch_number_corrected = GR55_patch_number - 297;
    bank_number_corrected = GR55_bank_number - 99;
  }

  if (GR55_bank_number >= (99 + GR55_preset_banks)) {   // In the Rhythm bank we have to adjust the bank and patch numbers so we start with R01-1
    GR55_patch_number_string = "R";
    patch_number_corrected = GR55_patch_number - (297 + (3 * GR55_preset_banks));
    bank_number_corrected = GR55_bank_number - (99 + GR55_preset_banks);
  }

  if (GR55_bank_number >= (99 + (2 * GR55_preset_banks))) {   // In the Other bank we have to adjust the bank and patch numbers so we start with O01-1
    GR55_patch_number_string = "O";
    patch_number_corrected = GR55_patch_number - (297 + (6 * GR55_preset_banks));
    bank_number_corrected = GR55_bank_number - (99 + (2 * GR55_preset_banks));
  }

  // Then add the bank number
  if (GR55_bank_selection_active == false) {
    GR55_patch_number_string = GR55_patch_number_string + String(((patch_number_corrected / 3) + 1) / 10) + String(((patch_number_corrected / 3) + 1) % 10);
    // Finally add the patch number
    GR55_patch_number_string = GR55_patch_number_string + "-" + String((patch_number_corrected % 3) + 1);
  }
  else {
    GR55_patch_number_string = GR55_patch_number_string + String((bank_number_corrected + 1) / 10) + String((bank_number_corrected + 1) % 10) + "--";
  }


}

void display_VG99_patch_string() {
  // Uses VG99_patch_number as input and returns VG99_patch_number_string as output in format "U001"
  // First character is U for User or P for Preset

  if (VG99_bank_selection_active == false) VG99_bank_number = (VG99_patch_number / 10); //Reset the bank to current patch

  if (VG99_bank_number < 20) {
    VG99_patch_number_string = "U";
  }

  if (VG99_bank_number > 19) {
    VG99_patch_number_string = "P";
  }

  // Then add the patch number
  if (VG99_bank_selection_active == false) {
    VG99_patch_number_string = VG99_patch_number_string + String((VG99_patch_number + 1) / 100) + String(((VG99_patch_number + 1) / 10) % 10) + String((VG99_patch_number + 1) % 10);
  }
  else {
    VG99_patch_number_string = VG99_patch_number_string + String(VG99_bank_number / 10) + String((VG99_bank_number) % 10) + "-";
  }
}
