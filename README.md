# HuiMcuTranslator

## What is it?
A little command-line program that 'sits' between Pro Tools and the MIDI plugin from Trevliga Spel for the Elgato Streamdeck (Mackie Control).

Runs only on Mac (Intel & M1), as far as I can tell (I have no Windows PC yet, will try that later).

## Compile
Download 'main.c' and compile with: 'gcc -o HuiMcuTranslator main.c -framework CoreMIDI -framework CoreServices -O3'.

## Build
Or use one of the pre-build versions in 'Builds' (Intel & Apple M1). No Windows or Linux version.

## Installation
Copy the compiled or pre-build versio to '/usr/bin/local'.

## Features
Not all of the features of the MIDI plugin from Trevligaspel are working.
This because Pro Tools talks & listens to EUCON & HUI, the plugin talks & listens to MCU and for some HUI functions there is no MCU available.

Some features not working:

- HUI doesn't have Global View (Daw Functions), as far as I can tell
- Pro Tools doesn't send a message for every function, so without a message no translation :(
- Some markers & control are not working yet
- Level meters are kind of working
- many more...
- But a lot of things do work! :)

For some Mackie features you have to press a modifier. These are implemented;

## Manual:

- Open 'Audio/MIDI-configuration'
- Open 'MIDI-studio'
- Double click on 'IAC'
- Add 4 ports and name them: 'PT IN', 'PT OUT', 'SD IN' and 'SD OUT'
- It really doesn't matter what name you use, as long as 'PT IN', 'PT OUT' etc are in the name; so 'MY PT IN' or 'Happy SD OUT' are all fine
- Close 'Audio/MIDI-configuration'
- Open Pro Tools
- Go to 'Setup->Peripharels->MIDI Controllers'
- Add 'Type': 'HUI'
- Add 'Receive From': 'PT IN'
- Add 'Send To': 'PT OUT'
- Close Pro Tools
- Start Elgato Streamdeck software
- Install Trevliga Spel MIDI plugin
- Add a Midi->Mackie Control button
- Under 'Layout and Midi ports': 'Midi Out port': 'SD OUT' and 'Midi In port': 'SD IN'
- Choose a button type (Section, Function etc)
- Read the above message in the Elgato Streamdeck software: 'Please note that...'
- Add more buttons...
- Open a Terminal window; type HuiMcuTranslator and press <Enter>
- Open Pro Tools and open a session
- Voila!
- When you're finished, press <Enter> to stop the translator; all faders go down, pans to the center and button lights out

The reason you should not open a session in Pro Tools before starting HuiMcuTranslator is: Pro Tools sends information about a session right after opening that session. Once that information is send, it is lost.

This is my first attempt to write a program in C, so please be kind... :)

Enjoy!
