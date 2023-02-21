# HuiMcuTranslator

A little program that 'sits' between Pro Tools and the MIDI plugin from Trevligaspel for the Elgato Streamdeck (Mackie Control).

Runs only on Mac (Intel & M1), as far as I can tell (I have no Windows PC yet, will try that later).

Compile it with: 'gcc -o main main.c -framework CoreMIDI -framework CoreServices -O3'.

Copy it to '/usr/bin/local' and run it from a terminal.

Compiled version will come soon.

Not all of the features of the MIDI plugin from Trevligaspel are working.
This because Pro Tools talks & listens to EUCON & HUI, the plugin talks & listens to MCU.

Some features not working:

- HUI doesn't have Global View (Daw Functions), as far as I can tell
- Some markers & control are not working yet
- Function buttons not implemented yet
