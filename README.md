# HuiMcuTranslator

A little program that 'sits' between Pro Tools and the MIDI plugin from Trevligaspel for the Elgato Streamdeck (Mackie Control).

Runs only on Mac (Intel & M1), as far as I can tell (I have no Windows PC yet, will try that later).

Compile it with: 'gcc -o HuiMcuTranslator main.c -framework CoreMIDI -framework CoreServices -O3'.

Or use one of the pre-build versions in 'Builds' (Intel & M1).

Copy it to '/usr/bin/local' and run it from a terminal.

Not all of the features of the MIDI plugin from Trevligaspel are working.
This because Pro Tools talks & listens to EUCON & HUI, the plugin talks & listens to MCU.

Some features not working:

- HUI doesn't have Global View (Daw Functions), as far as I can tell
- Some markers & control are not working yet
- Function buttons not implemented yet
- Level meters are kind of working
- many more...
- But a lot of things work! :)

Manual:

- start Elgato Streamdeck software
- open Pro Tools (don't open a session yet)
- start HuiMcuTranslator
- open a session

The reason you should not open a session before starting HuiMcuTranslator is: Pro Tools sends information about that session right after opening a session.

This is my first attempt to write a program in C. I hope you will enjoy!
