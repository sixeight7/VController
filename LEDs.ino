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

// Functions for LED control for which I use 12 5mm Neopixel RGB LEDs

#include <Adafruit_NeoPixel.h>
#include <avr/power.h>

// ***************************** Settings you may want to change *****************************

// Which colours are used for which mode?
// pixels.Color takes GRB (Green - Red - Blue) values, from 0,0,0 up to 255,255,255
// The neopixels are very bright. I find setting them to 10 is just fine for an indicator light.

#define GP10_PATCH_COLOUR 4 //Yellow
#define GP10_OFF_COLOUR 14
#define GR55_PATCH_COLOUR 3 //Blue
#define GR55_OFF_COLOUR 13
#define VG99_PATCH_COLOUR 2 //Red
#define VG99_OFF_COLOUR 12
#define GP10_STOMP_COLOUR_ON 4 //Yellow
#define GP10_STOMP_COLOUR_OFF 14 //Green dimmed
#define GR55_STOMP_COLOUR 1 //Green
#define VG99_STOMP_COLOUR_ON 2 //Red
#define VG99_STOMP_COLOUR_OFF 12 //Red dimmed
#define GLOBAL_STOMP_COLOUR_ON 2 //White
#define GLOBAL_STOMP_COLOUR_OFF 12 //White dimmed
#define BPM_COLOUR_ON 10 // Green flasing LED
#define BPM_COLOUR_OFF 0

// Defining FX colours:
#define FX_GTR_COLOUR_ON 6 //White for guitar settings
#define FX_GTR_COLOUR_OFF 16
#define FX_DELAY_COLOUR_ON 10 //Green for delays
#define FX_DELAY_COLOUR_OFF 11
#define FX_MODULATE_COLOUR_ON 3 //Blue for modulation FX
#define FX_MODULATE_COLOUR_OFF 13
#define FX_FILTER_COLOUR_ON 8 //Purple for filter FX
#define FX_FILTER_COLOUR_OFF 18
#define FX_PITCH_COLOUR_ON 5 //Turquoise for pitch FX
#define FX_PITCH_COLOUR_OFF 15
#define FX_AMP_COLOUR_ON 2 //Red for amp FX and amp solo
#define FX_AMP_COLOUR_OFF 12
#define FX_DIST_COLOUR_ON 9 //Pink for distortion FX
#define FX_DIST_COLOUR_OFF 19
#define FX_REVERB_COLOUR_ON 7 //Yellow for reverb FX
#define FX_REVERB_COLOUR_OFF 17

struct colour {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
};

//Lets make some colours (R,G,B)
// Adding 100 to a colour number makes the LED flash!
// To make new colours, set the mode in B_Settings to MODE_COLOUR_MAKER (mode 17)

colour colours[21] = {
  {0, 0, 0} ,   // Colour 0 is LED OFF
  {0, 10, 0} ,  // Colour 1 is Green
  {10, 0, 0} ,  //  Colour 2 is Red
  {0, 0, 10} ,  // Colour 3 is Blue
  {10, 5, 0} ,  // Colour 4 is Orange
  {0, 8, 5} ,  // Colour 5 is Turquoise
  {8, 10, 8} ,  // Colour 6 is White
  {8, 8, 0} ,   // Colour 7 is Yellow
  {4, 0, 8} ,   // Colour 8 is Purple
  {10, 0, 5} ,   // Colour 9 is Pink
  {2, 10, 1} ,   // Colour 10 is available
  {0, 1, 0} ,   // Colour 11 is Green dimmed
  {1, 0, 0} ,   // Colour 12 is Red dimmed
  {0, 0, 1} ,   // Colour 13 is Blue dimmed
  {1, 1, 0} ,   // Colour 14 is Orange dimmed
  {0, 1, 1} ,   // Colour 15 is Turquoise dimmed
  {1, 1, 1} ,   // Colour 16 is White dimmed
  {1, 1, 0} ,   // Colour 17 is Yellow dimmed
  {1, 0, 2} ,   // Colour 18 is Purple dimmed
  {1, 0, 1} ,   // Colour 19 is Pink dimmed
  {0, 0, 0}   // Colour 20 is available
};

#define LEDFLASH_TIMER_LENGTH 500 // Sets the speed with which the LEDs flash (500 means 500 ms on, 500 msec off)
unsigned long LEDflashTimer = 0;

// ***************************** Hardware settings *****************************
// Which pin on the Arduino is connected to the NeoPixels LEDs?
// On a Trinket or Gemma we suggest changing this to 1
#define NEOPIXELLEDPIN            17

// How many NeoPixel LEDs are attached to the Teensy/Arduino?
// Also the LEDs can be connected in any order. This can be
#define NUMLEDS      12
uint8_t LED_order[12] = { 6, 5, 4, 7, 0, 3, 8, 1, 2, 9, 10, 11}; //Order in which the LEDs are connected. First LED = 0

// ************************ Settings you probably won't have to change ************************
uint8_t *LEDs_stomp_mode_ptr[NUMBER_OF_STOMP_BANKS][9]; // array of pointers that point to the stompbox mode LEDs - set them under settings.
boolean update_LEDS = true;
boolean LED_flashing_state_on = true;
uint8_t row_select = 0;

// Some LEDs for switch functions
uint8_t global_tuner_LED = GLOBAL_STOMP_COLOUR_OFF; // Global tuner LED
uint8_t global_tap_tempo_LED = BPM_COLOUR_OFF;      // Tap tempo LED
uint8_t GP10_select_LED = GP10_OFF_COLOUR;              // GP10 on/off LED
uint8_t GR55_select_LED = GR55_OFF_COLOUR;              // GR55 on/off LED
uint8_t VG99_select_LED = VG99_OFF_COLOUR;              // VG99 on/off LED
uint8_t LEDoff = 0; // A dummy placeholder for when a LED of a stompbox has to be switches off

#define STARTUP_TIMER_LENGTH 100 // NeoPixel LED switchoff timer set to 100 ms
unsigned long startupTimer = 0;

// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
// Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
// example for more information on possible values.
Adafruit_NeoPixel LEDs = Adafruit_NeoPixel(NUMLEDS, NEOPIXELLEDPIN, NEO_GRB + NEO_KHZ800);

uint8_t LED_no;

void setup_LED_control()
{
  LEDs.begin(); // This initializes the NeoPixel library.

  //Turn the LEDs off repeatedly for 100 ms to reduce startup flash of LEDs
  unsigned int startupTimer = millis();
  while (millis() - startupTimer <= STARTUP_TIMER_LENGTH) {
    LEDs.show();
  }
}

void main_LED_control()
{
  if (update_LEDS == true) {
    update_LEDS = false;
    switch (mode) {
      case MODE_TUNER:
        turn_all_LEDs_off();
        // Check if we came from stompbox mode and if there was a LED set to global tap tempo
        if (previous_mode >= 20) {
          for (uint8_t i = 0; i < 9; i++) {
            if (LEDs_stomp_mode_ptr[previous_mode - 20][i] == &global_tap_tempo_LED) show_colour(i, GLOBAL_STOMP_COLOUR_ON); // If so switch it on
          }
        }
        LEDs.show();
        break;

      case MODE_STOMP_1:
        turn_all_LEDs_off();
        // Copy the LEDs from the LEDS_STOM_GP10 array
        for (uint8_t count = 0; count < 9; count++) {
          show_colour(count, *LEDs_stomp_mode_ptr[0][count]);
        }
        show_colour(GP10_LED - 1, GP10_STOMP_COLOUR_ON); // And show the GP10 mode LED
        LEDs.show();
        break;
      case MODE_STOMP_2:
        turn_all_LEDs_off();
        // Copy the LEDs from the LEDS_STOM_GP10 array
        for (uint8_t count = 0; count < 9; count++) {
          show_colour(count, *LEDs_stomp_mode_ptr[1][count]);
        }
        show_colour(GR55_LED - 1, GR55_PATCH_COLOUR); // And show the GR55 mode LED
        LEDs.show();
        break;
      case MODE_STOMP_3:
        turn_all_LEDs_off();
        // Copy the LEDs from the LEDS_STOM_GP10 array
        for (uint8_t count = 0; count < 9; count++) {
          show_colour(count, *LEDs_stomp_mode_ptr[2][count]);
        }
        show_colour(VG99_LED - 1, VG99_PATCH_COLOUR); // And show the VG99 mode LED
        LEDs.show();
        break;

      // *************************************** GP10 LED modes ***************************************
      case MODE_GP10_PATCH:
        // Show the LED that matches the patch when you are in the right bank
        turn_all_LEDs_off();
        LED_no = (GP10_patch_number % 10); // Calculate the right LED from the patchnumber
        //if (GP10_bank_number == GP10_patch_number / 10) {
        if (GP10_bank_selection_active == false) show_colour(LED_no, GP10_PATCH_COLOUR);
        else show_colour(LED_no, GP10_PATCH_COLOUR + 100); // Set colour to flashing
        //}
        show_colour(GP10_LED - 1, GP10_PATCH_COLOUR); // And show the GP10 mode LED
        LEDs.show();
        break;

      case MODE_GP10_DIRECTSELECT1:
        turn_all_LEDs_off();
        show_colour(GP10_LED - 1, GP10_PATCH_COLOUR); // Show the GP10 mode LED
        // Flash the LEDs 1 to 10
        for (uint8_t count = 0; count < 10; count++) {
          show_colour(count, GP10_PATCH_COLOUR + 100);
        }
        LEDs.show();
        break;

      case MODE_GP10_DIRECTSELECT2:
        turn_all_LEDs_off();
        show_colour(GP10_LED - 1, GP10_PATCH_COLOUR); // Show the GP10 mode LED
        // Flash the LED of the bank number that was selected in the previous step
        LED_no = (GP10_bank_number - 1);
        if (GP10_bank_number == 0) LED_no = 9;
        show_colour(LED_no, GP10_PATCH_COLOUR + 100); // + 100 makes the LED flash
        LEDs.show();
        break;

      // *************************************** GR55 LED modes ***************************************
      case MODE_GR55_PATCH:
        // Show the LED that matches the patch
        // When in bank selection mode flash the three bottom LEDs, because they match the bank number that is being displayed

        turn_all_LEDs_off();
        LED_no = (GR55_patch_number % 9); // Calculate the right LED from the patchnumber

        if (GR55_bank_selection_active == false) {  // Normally just show the LED of the patch you selected
          show_colour(LED_no, GR55_PATCH_COLOUR);
        }
        else {  // Flash bottom bank
          show_colour(0, GR55_PATCH_COLOUR + 100);
          show_colour(1, GR55_PATCH_COLOUR + 100);
          show_colour(2, GR55_PATCH_COLOUR + 100);
        }
        show_colour(GR55_LED - 1, GR55_PATCH_COLOUR); // And show the GR55 mode LED
        LEDs.show();
        break;

      case MODE_GR55_DIRECTSELECT1:
        show_colour(GR55_LED - 1, GR55_PATCH_COLOUR); // Show the GR55 mode LED
        // Flash the LEDs 1 to 10
        // Flash LEDs 1 to 10
        for (uint8_t count = 0; count < 10; count++) {
          show_colour(count, GR55_PATCH_COLOUR + 100); // Moderately bright green color.
        }
        LEDs.show();
        break;

      case MODE_GR55_DIRECTSELECT2:
        turn_all_LEDs_off();
        show_colour(GR55_LED - 1, GR55_PATCH_COLOUR); // Show the GR55 mode LED
        // Flash the LED of the bank number that was selected in the previous step
        LED_no = (GR55_bank_number / 10) - 1; //Calculate which LED to turn on
        show_colour(LED_no, GR55_PATCH_COLOUR + 100); // + 100 makes the LED flash
        LEDs.show();
        break;
      case MODE_GR55_DIRECTSELECT3:
        // Make the row of LEDs flash that contains the selected bank
        turn_all_LEDs_off();
        show_colour(GR55_LED - 1, GR55_PATCH_COLOUR); // Show the GR55 mode LED

        row_select = (GR55_bank_number - 1) % 3;

        if (row_select == 0) {
          show_colour(0, GR55_PATCH_COLOUR + 100); // + 100 makes the LED flash
          show_colour(1, GR55_PATCH_COLOUR + 100);
          show_colour(2, GR55_PATCH_COLOUR + 100);
        }
        if (row_select == 1) {
          show_colour(3, GR55_PATCH_COLOUR + 100);
          show_colour(4, GR55_PATCH_COLOUR + 100);
          show_colour(5, GR55_PATCH_COLOUR + 100);
        }
        if (row_select == 2) {
          show_colour(6, GR55_PATCH_COLOUR + 100);
          show_colour(7, GR55_PATCH_COLOUR + 100);
          show_colour(8, GR55_PATCH_COLOUR + 100);
        }
        LEDs.show();
        break;

      // *************************************** VG99 LED modes ***************************************
      case MODE_VG99_PATCH:
        // Show the LED that matches the patch when you are in the right bank
        turn_all_LEDs_off();
        show_colour(VG99_LED - 1, VG99_PATCH_COLOUR); // Show the VG-99 mode LED
        if (VG99_bank_number == VG99_patch_number / 10) {
          LED_no = (VG99_patch_number % 10); // Calculate the right LED from the patchnumber
          if (VG99_bank_selection_active == false) show_colour(LED_no, VG99_PATCH_COLOUR);
          else show_colour(LED_no, VG99_PATCH_COLOUR + 100); // Make it flash
        }
        LEDs.show();
        break;

      case MODE_VG99_DIRECTSELECT1:
        // Flash the LEDs 1 to 10
        turn_all_LEDs_off();
        show_colour(VG99_LED - 1, VG99_PATCH_COLOUR); // Show the VG-99 mode LED
        for (uint8_t count = 0; count < 10; count++) {
          show_colour(count, VG99_PATCH_COLOUR + 100); // + 100 makes the LED flash
        }

        LEDs.show();
        break;

      case MODE_VG99_DIRECTSELECT2:
        // Flash the LED of the bank number that was selected in the previous step
        turn_all_LEDs_off();
        show_colour(VG99_LED - 1, VG99_PATCH_COLOUR); // Show the VG-99 mode LED
        LED_no = (VG99_bank_number - 1);
        if (VG99_bank_number == 0) LED_no = 9;
        show_colour(LED_no, VG99_PATCH_COLOUR + 100); // + 100 makes the LED flash
        LEDs.show();
        break;
      
      case MODE_COLOUR_MAKER:
        // Show the colour on the top 6 LEDS
        for (uint8_t i=6; i<12; i++) LEDs.setPixelColor(LED_order[i], LEDs.Color(colour_maker_green, colour_maker_red, colour_maker_blue));
        // Button 1 and 4 are RED
        LEDs.setPixelColor(LED_order[0], LEDs.Color(0, 1, 0));
        LEDs.setPixelColor(LED_order[3], LEDs.Color(0, 10, 0));
        // Button 2 and 5 are GREEN
        LEDs.setPixelColor(LED_order[1], LEDs.Color(1, 0, 0));
        LEDs.setPixelColor(LED_order[4], LEDs.Color(10, 0, 0));
        // Button 3 and 6 are BLUE
        LEDs.setPixelColor(LED_order[2], LEDs.Color(0, 0, 1));
        LEDs.setPixelColor(LED_order[5], LEDs.Color(0, 0, 10));
        LEDs.show();
        break;

      default:   // Just in case you forgot to add some status - white flashing LEDs
        for (uint8_t count = 0; count < 12; count++) {
          show_colour(count, 109);
          LEDs.show();
        }
    }
  }

  // Check here if LEDs need to flash and make them do it
  flash_LEDs();

}

void show_colour(uint8_t LED_number, uint8_t colour_number) { // Sets the specified LED to the specified colour
  uint8_t number_fixed = colour_number % 100; // In case of flashing LEDS this will take off the extra 100.
  if (colour_number < 100) { // Check if it is not a flashing LED
    // Turn the LED on
    LEDs.setPixelColor(LED_order[LED_number], LEDs.Color(colours[number_fixed].green, colours[number_fixed].red, colours[number_fixed].blue));
  }
  else if (LED_flashing_state_on == true) {
    // Turn the LED on
    LEDs.setPixelColor(LED_order[LED_number], LEDs.Color(colours[number_fixed].green, colours[number_fixed].red, colours[number_fixed].blue));
  }
  else {
    // Turn the LED off
    LEDs.setPixelColor(LED_order[LED_number], 0, 0, 0);
  }
}

void turn_all_LEDs_off() {
  for (uint8_t count = 0; count < NUMLEDS; count++) {
    LEDs.setPixelColor(count, LEDs.Color(0, 0, 0));
  }
  // LEDs.show();
}

void flash_LEDs() {
  // Check if timer needs to be set
  if (LEDflashTimer == 0) {
    LEDflashTimer = millis();
  }

  // Check if timer runs out
  if (millis() - LEDflashTimer > LEDFLASH_TIMER_LENGTH) {
    LEDflashTimer = millis(); // Reset the timer
    LED_flashing_state_on = !LED_flashing_state_on;
    update_LEDS = true; // Get the LEDs to update
  }
}
