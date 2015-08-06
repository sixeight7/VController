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

// Setup of input ports of switches.
// Check for switch pressed and output the result in the switch_pressed variable.

#include <Bounce.h>

// ***************************** Settings you may want to change *****************************
#define LONG_PRESS_TIMER_LENGTH 1000 // Timer used for detecting long-pressing switch. Time is in milliseconds
unsigned long Long_press_timer = 0;


// ***************************** Hardware settings *****************************
// Define pin numbers
//Pin 0 and 1 reserved for MIDI

//Switches are four rows of three switches, connected like a keypad.
// Assign the pins for the switch row and columns below
// (I found I connected most the rows and columns backwards, but you can assign it any way you like.)
#define SWITCH_ROW1 5 // Row 1 (pin 5) is connected to switch 1,2 and 3
#define SWITCH_ROW2 4 // Row 2 (pin 4) is connected to switch 4,5 and 6
#define SWITCH_ROW3 3 // Row 3 (pin 3) is connected to switch 7,8 and 9
#define SWITCH_ROW4 2 // Row 4 (pin 2) is connected to switch 10,11 and 12
#define SWITCH_COL1 12 // Column 1 (pin 12) is connected to the other side of switch 1,4,7 and 10
#define SWITCH_COL2 11 // Column 2 (pin 11) is connected to the other side of switch 2,5,8 and 11
#define SWITCH_COL3 6 // Column 3 (pin 6) is connected to the other side of switch 3,6,9 and 12

#define SWITCH_EXT1 13   // Pin 13 to 16 are reserved for two external switches
#define SWITCH_EXT2 14  // The switches are connected to these pins and to ground
#define SWITCH_EXT3 15
#define SWITCH_EXT4 16

//Pin 17 reserved for Neopixel LEDs
//Pin 18 and 19 reserved for I2C bus (LCD)

// ************************ Settings you probably won't have to change ************************

int switch_pressed = 0; //Variable set when switch is pressed
int switch_released = 0; //Variable set when switch is released
int switch_long_pressed = 0; //Variable set when switch is pressed long (check LONG_PRESS_TIMER_LENGTH for when this will happen)

int switch_long_pressed_memory = 0;
int active_column = 1; // Which switch column is being read?

boolean switch_ext1_polarity_reversed = false; // Pedal accepts both normally open and normally closed external switches
boolean switch_ext2_polarity_reversed = false;
boolean switch_ext3_polarity_reversed = false;
boolean switch_ext4_polarity_reversed = false;

Bounce switch1 = Bounce(SWITCH_ROW1, 50); // Every switch needs its own bouncer even though they share the same pins
Bounce switch2 = Bounce(SWITCH_ROW1, 50); // State of bouncer is LOW when switch is pressed and high when switch is not pressed
Bounce switch3 = Bounce(SWITCH_ROW1, 50); // Fallingedge means switch is pressed. Risingedge means switch is released.
Bounce switch4 = Bounce(SWITCH_ROW2, 50);
Bounce switch5 = Bounce(SWITCH_ROW2, 50);
Bounce switch6 = Bounce(SWITCH_ROW2, 50);
Bounce switch7 = Bounce(SWITCH_ROW3, 50);
Bounce switch8 = Bounce(SWITCH_ROW3, 50);
Bounce switch9 = Bounce(SWITCH_ROW3, 50);
Bounce switch10 = Bounce(SWITCH_ROW4, 50);
Bounce switch11 = Bounce(SWITCH_ROW4, 50);
Bounce switch12 = Bounce(SWITCH_ROW4, 50);

Bounce switch_ext1 = Bounce(SWITCH_EXT1, 50);
Bounce switch_ext2 = Bounce(SWITCH_EXT2, 50);
Bounce switch_ext3 = Bounce(SWITCH_EXT3, 50);
Bounce switch_ext4 = Bounce(SWITCH_EXT4, 50);

boolean check_released = false; // On the first run release should not be checked, becasue bounce handler states are LOW by default.

void setup_switch_check()
{
  //Enable internal pullup resistors for switch row pins
  pinMode(SWITCH_ROW1, INPUT_PULLUP);
  pinMode(SWITCH_ROW2, INPUT_PULLUP);
  pinMode(SWITCH_ROW3, INPUT_PULLUP);
  pinMode(SWITCH_ROW4, INPUT_PULLUP);

  pinMode(SWITCH_COL1, OUTPUT);
  digitalWrite(SWITCH_COL1, LOW);   //Enable the first column
  pinMode(SWITCH_COL2, INPUT);      //Here's the trick - keep the other output ports input, and there will be no shorting the outputs!!!
  pinMode(SWITCH_COL3, INPUT);

  pinMode(SWITCH_EXT1, INPUT_PULLUP); //Also enable input pullup resostors for S1 and S2
  pinMode(SWITCH_EXT2, INPUT_PULLUP);
  pinMode(SWITCH_EXT3, INPUT_PULLUP);
  pinMode(SWITCH_EXT4, INPUT_PULLUP);
   
  // Check polarity of external switches
  // Pedal accepts both normally open and normally closed external switches
  
  delay(10); //Short delay to allow the ports to settle
  if (digitalRead(SWITCH_EXT1) == LOW) switch_ext1_polarity_reversed = true;
  if (digitalRead(SWITCH_EXT2) == LOW) switch_ext2_polarity_reversed = true;
  if (digitalRead(SWITCH_EXT3) == LOW) switch_ext3_polarity_reversed = true;
  if (digitalRead(SWITCH_EXT4) == LOW) switch_ext4_polarity_reversed = true;
}

void main_switch_check()
{
  // Run through the switch columns, One column is being activated on every run of this routine.
  // Check if a switch is pressed of released and set the switch_pressed and switch_released variable accordingly
  switch_pressed = 0;
  switch_released = 0;
  switch_long_pressed = 0;

  switch (active_column) {
    case 1:

      switch10.update();  //Update the switches in the bounce library
      switch7.update();
      switch4.update();
      switch1.update();

      // Check for switches in column 1 being pressed
      if (switch10.fallingEdge()) switch_pressed = 10;
      if ((switch7.fallingEdge()) && (switch10.read() == HIGH)) switch_pressed = 7; // Also check state of the switch above
      if ((switch4.fallingEdge()) && (switch7.read() == HIGH)) switch_pressed = 4; // Also check state of the switch above
      if ((switch1.fallingEdge()) && (switch4.read() == HIGH)) switch_pressed = 1; // Also check state of the switch above

      // Check for switches in column 1 being released
      if ((check_released) && (switch10.risingEdge())) switch_released = 10;
      if ((check_released) && (switch7.risingEdge()) && (switch10.read() == HIGH)) switch_released = 7; // Also check state of the switch above
      if ((check_released) && (switch4.risingEdge()) && (switch7.read() == HIGH)) switch_released = 4; // Also check state of the switch above
      if ((check_released) && (switch1.risingEdge()) && (switch4.read() == HIGH)) switch_released = 1; // Also check state of the switch above

      digitalWrite(SWITCH_COL1, HIGH);
      pinMode(SWITCH_COL1, INPUT);

      pinMode(SWITCH_COL2, OUTPUT); //Switch on the second column
      digitalWrite(SWITCH_COL2, LOW);

      active_column = 2;

      // Also check for EXT1 and EXT2 switches here - EXT3 and 4 are done in the next session
      switch_ext1.update();
      if (switch_ext1_polarity_reversed == false) { //Take switch polarity into account
        if (switch_ext1.fallingEdge()) switch_pressed = 13;
        if ((check_released) && (switch_ext1.risingEdge())) switch_released = 13;
      }
      else {
        if (switch_ext1.risingEdge()) switch_pressed = 13;
        if ((check_released) && (switch_ext1.fallingEdge())) switch_released = 13;
      }

      switch_ext2.update();
      if (switch_ext2_polarity_reversed == false) { //Take switch polarity into account
        if (switch_ext2.fallingEdge()) switch_pressed = 14;
        if ((check_released) && (switch_ext2.risingEdge())) switch_released = 14;
      }
      else {
        if (switch_ext2.risingEdge()) switch_pressed = 14;
        if ((check_released) && (switch_ext2.fallingEdge())) switch_released = 14;
      }

      break;

    case 2:

      switch11.update();
      switch8.update();
      switch5.update();
      switch2.update();

      // Check for switches in column 2 being pressed
      if (switch11.fallingEdge()) switch_pressed = 11;
      if ((switch8.fallingEdge()) && (switch11.read() == HIGH)) switch_pressed = 8; // Also check state of the switch above
      if ((switch5.fallingEdge()) && (switch8.read() == HIGH)) switch_pressed = 5; // Also check state of the switch above
      if ((switch2.fallingEdge()) && (switch5.read() == HIGH)) switch_pressed = 2; // Also check state of the switch above

      // Check for switches in column 2 being released
      if ((check_released) && (switch11.risingEdge())) switch_released = 10;
      if ((check_released) && (switch8.risingEdge()) && (switch11.read() == HIGH)) switch_released = 8; // Also check state of the switch above
      if ((check_released) && (switch5.risingEdge()) && (switch8.read() == HIGH)) switch_released = 5; // Also check state of the switch above
      if ((check_released) && (switch2.risingEdge()) && (switch5.read() == HIGH)) switch_released = 2; // Also check state of the switch above

      digitalWrite(SWITCH_COL2, HIGH);
      pinMode(SWITCH_COL2, INPUT);

      pinMode(SWITCH_COL3, OUTPUT);  //Switch on the third column
      digitalWrite(SWITCH_COL3, LOW);

      active_column = 3;

      // Also check for EXT3 and EXT4 switches here - EXT1 and 2 are done in the previous session
      switch_ext3.update();
      if (switch_ext3_polarity_reversed == false) { //Take switch polarity into account
        if (switch_ext3.fallingEdge()) switch_pressed = 15;
        if ((check_released) && (switch_ext3.risingEdge())) switch_released = 15;
      }
      else {
        if (switch_ext3.risingEdge()) switch_pressed = 15;
        if ((check_released) && (switch_ext3.fallingEdge())) switch_released = 15;
      }

      switch_ext4.update();
      if (switch_ext4_polarity_reversed == false) { //Take switch polarity into account
        if (switch_ext4.fallingEdge()) switch_pressed = 16;
        if ((check_released) && (switch_ext4.risingEdge())) switch_released = 16;
      }
      else {
        if (switch_ext4.risingEdge()) switch_pressed = 16;
        if ((check_released) && (switch_ext4.fallingEdge())) switch_released = 16;
      }

      break;

    case 3:

      switch12.update();
      switch9.update();
      switch6.update();
      switch3.update();

      // Check for switches in column 3 being pressed
      if (switch12.fallingEdge())  switch_pressed = 12;
      if ((switch9.fallingEdge()) && (switch12.read() == HIGH)) switch_pressed = 9; // Also check state of the switch above
      if ((switch6.fallingEdge()) && (switch9.read() == HIGH)) switch_pressed = 6; // Also check state of the switch above
      if ((switch3.fallingEdge()) && (switch6.read() == HIGH)) switch_pressed = 3; // Also check state of the switch above

      // Check for switches in column 3 being released
      if ((check_released) && (switch12.risingEdge())) switch_released = 12;
      if ((check_released) && (switch9.risingEdge()) && (switch12.read() == HIGH)) switch_released = 9; // Also check state of the switch above
      if ((check_released) && (switch6.risingEdge()) && (switch9.read() == HIGH)) switch_released = 6; // Also check state of the switch above
      if ((check_released) && (switch3.risingEdge()) && (switch6.read() == HIGH)) switch_released = 3; // Also check state of the switch above

      check_released = true; //Here all the bounce handler states are high and the release state can be checked now.

      digitalWrite(SWITCH_COL3, HIGH);
      pinMode(SWITCH_COL3, INPUT);

      pinMode(SWITCH_COL1, OUTPUT);   //Switch on the first column
      digitalWrite(SWITCH_COL1, LOW);

      active_column = 1;
      break;
  }

  // Now check for Long pressing a button
  if (switch_pressed > 0) {
    Long_press_timer = millis(); // Set timer on switch pressed
    switch_long_pressed_memory = switch_pressed; // Remember the button that was pressed
  }
  if (switch_released > 0) Long_press_timer = 0;  //Reset the timer on switch released

  if ((millis() - Long_press_timer > LONG_PRESS_TIMER_LENGTH) && (Long_press_timer > 0)) {
    switch_long_pressed = switch_long_pressed_memory; //pass on the buttonvalue we remembered before
    Long_press_timer = 0;  //Reset the timer when timer runs out
  }
}
