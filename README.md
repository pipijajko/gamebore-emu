Gamebore - a Gameboy Emulator
===========================

Hey! 

Gamebore is a Nintendo Gameboy emulator written in C. 

I started it as an experiment in a bit more "modern" C writing style, and kind of went from there. I tried to find a different approach in few common schemes for emulators e.g. N->1 opcode bit-mask to function mapping (opcodes.inc), my own idea for MBC memory section switching; which ended up probably being a bit sub-optimal, but I'm at peace with that. 

----------
I tried to keep 3rd party dependencies to minimum, the only dependency is a tiny tinee freeglut (which was not such a good choice in the end, but it does the job). 
Currently compiling only on Windows, but no particular Windows dependencies except for QueryPerfomanceCounter use --  which is few lines.

![ SML running - sorry for that fat gif](https://i.imgur.com/TruHVjW.gif)
Usage
----------------------
	gamebore.exe <rom_name>


Controls
----------------
	arrow keys - D-PAD
	Z, X       - A, B buttons
	Home       - Start button
	End        - Select button
	Q          - Start debug tracing 
	q          - Stop debug tracing

*Note:* Debug tracing, will print all GameBoy CPU operations and registers in the console and to the text file. Expect severe slowdowns.


Completness
-------------
* All CPU instructions and interrupts implemented, and correctly, at least according to the Blargg's test roms:
* GPU supports all original DMG features: sprites, tile maps, transparency, BW palettes.
* Suports both ROM-Only images, as well as MBC1 with switchable ROM/RAM. 
* Have a gtest suite to verify most of the instructions. But they were not meant to be really comprehensive, I wrote them mostly when I wanted to check some stuff.
* Full tracing support - can trace every instruction along with CPU registers to text file (might come in handy for other emulator devs).


What's still missing?
----------------------
* Real-Time save upport,
* MBC save support,
* GBC support (should be rather easy with current memory infra),
* MBC2,3... and RTC support,
* Maybe a nicer UI, and a debugger.


More GIFs if you need them
------------------
![Blargg's CPU instruction tests](http://i.imgur.com/tXBUkWO.gif)
![dr mario running](http://i.imgur.com/26IMnVy.gif)
