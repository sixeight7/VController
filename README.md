# VController
Dedicated MIDI controller for Boss GP-10  / Roland GR-55 and Roland VG-99

# prerequisites
Developed on Teensy LC. 
Make sure you have the https://bitbucket.org/fmalpartida/new-liquidcrystal/downloads/LiquidCrystal_V1.2.1.zip library installed. It is the only non-standard library used in this sketch.

Start with VController.ino to browse this code. It has the main setup and loop from where all the other files are called.

I tried to seperate the different tasks of the VController in the different files. Complete seperation is not possible, but it helps a great deal in finding stuff back.

Short desciption of each file:

B_settings.ino: contains the basic settings for the pedal, like MIDI addresses and function of switches.
EEPROM.ino: deals with putting some settings in memory.
LCD.ino: sets up hardware for the LCD and decides what to put on the screen.
lEDs.ino: sets up the neopixel LEDs and decides when to light up which one.
MIDI.ino: sets up the MIdI hardware and provides the basic functions
MIDI_FC300: deals with specific midi functions for the FC 300.
MIDI_GP10: deals with specific midi functions for the Boss GP-10.
MIDI_GR55: deals with specific midi functions for the Roland GR-55.
MIDI_VG99: deals with specific midi functions for the Roland VG-99.
Switch_check: checks whether a switch is pressed, released or pressed for a longer time.
Switch_control: basically deals with patch changes and provides the framework for the stonpbox modes.
Switch_funcs: the place for some special pedal functions, like global tuner, global page up and down, etc.

More details are up in my project blog.

# To do
Stompbox mode for GR55.
Improve stompbox mode for VG99. It still has some issues.
Global tap tempo
