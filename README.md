# VController
Dedicated MIDI controller for Boss GP-10  / Roland GR-55 and Roland VG-99

# Features
* Patch selection - view of patch names for all three devices.
* Direct selection of patches
* GP10: change fixed parameters on each patch. Display tells you which parameter is changed and buttons have a colour that shows you the type of FX you select.
* GR55: control assignments via cc messages. Parameter names and colours displayed for the most common parameters.
* VG99: pedal simulates an FC300 for CTL-1 to CTL-8. Parameter names and colours displayed for the most common parameters.
* Global Tap Tempo: all devices pick up the tempo from the V-Controller. There is the option to keep this tempo on all patches on all devices.
* Global tuner. GP10 and VG99 start tuner simultaniously. GR55 will mute. It is not possible to start the tuner on the GR55 through a midi sysex message.
* US-20 simulation: smart muting of GP10, GR55 or VG99 by switching off the COSM guitar/synth/normal PU on the devices that are not active.
* Autobass mode: sends a CC message with the number of the lowest string that is being played (CC #15)

# Hardware
Teensy LC, 12 switches and 12 neopixel LEDs, 1602 LCD display, some additional parts and an enclosure.
See my project blog for details and schematic.

# Software
Developed on Teensy LC. 
Make sure you have the https://bitbucket.org/fmalpartida/new-liquidcrystal/downloads/LiquidCrystal_V1.2.1.zip library installed. It is the only non-standard library used in this sketch.

Start with VController.ino to browse this code. It has the main setup and loop from where all the other files are called.

I tried to seperate the different tasks of the VController in the different files. Complete seperation is not possible, but it helps a great deal in finding stuff back.

Short desciption of each file:

* B_settings.ino: contains the basic settings for the pedal, like MIDI addresses and function of switches.
* EEPROM.ino: deals with putting some settings in memory.
* LCD.ino: sets up hardware for the LCD and decides what to put on the screen.
* LEDs.ino: sets up the neopixel LEDs and decides when to light up which one.
* MIDI.ino: sets up the MIdI hardware and provides the basic functions
* MIDI_FC300: deals with specific midi functions for the FC 300.
* MIDI_GP10: deals with specific midi functions for the Boss GP-10.
* MIDI_GR55: deals with specific midi functions for the Roland GR-55.
* MIDI_VG99: deals with specific midi functions for the Roland VG-99.
* Switch_check: checks whether a switch is pressed, released or pressed for a longer time.
* Switch_control: basically deals with patch changes and provides the framework for the stompbox modes.
* Switch_funcs: the place for some special pedal functions, like global tuner, global page up and down, etc.

More details are up in my project blog.

# To do
* A new version of the VController is in the making. Watch my blog on vguitarforums for details: http://www.vguitarforums.com/smf/index.php?topic=15154.275
