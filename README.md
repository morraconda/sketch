Sketch coursework (2023)

This project is primarily focused on "compressing" a .pgm file (greyscale image) into a custom .sk file format (detailed below).  
Backwards conversion from .sk -> .pgm is also supported.  
The converter was made entirely by me, including converterTest.  
The .sketch file visualiser was completed off the provided skeleton code, and I did not write any of the sketch tests (test.c)

Based on a 200x200 (40 KiB) bands.pgm and fractal.pgm file to compress:  
2D run-length encoding: bands.sk 0.15 KiB, fractal.sk 70.8 KiB  
1D run-length encoding: bands.sk 16.4 KiB, fractal.sk 129.7 KiB  
Writing pixel by pixel: bands.sk ~350 KiB, fractal.sk ~350 KiB

Usage:
"./converter" or "./sketch" to run tests.   
"./converter [filename]" to convert .sk <-> .pgm (file ending must be specified).  
"./sketch [filename]" to visualise a .sketch file using SDL2. [requires SDL2 to work]

As you may notice, the compression isn't very good for fractal, in fact coming out larger than the original image. This is due to the nature of the .sketch file format, only being able to have a 6-bit operand per byte. This means it takes 6+2 bytes to specify a change in 32-bit RGBA colour, and 4+1 bytes to specify a co-ordinate above (31, 31). 

The .sketch file viewer stores a current position, target position, the current drawing tool being used and an (unsigned) accumulator. Each byte is 1 command, consisting of a 2-bit opcode and a 6-bit operand. 

The commands are as follows:  
DX: Increase target position x by signed operand (-32 to +31), set current position to target position.  
DY: Increase target position y by signed operand (-32 to +31).  
DATA: Left shift accumulator by 6 bits, add unsigned operand to accumulator.  
TOOL: Set drawing tool.
whenever a command is called, the accumulator value is then reset to 0.

Drawing Tools:   
NONE: do nothing  
LINE: draw a line from current position to target position whenever DX is called.  
BLOCK: draw a rectangle from current position to target position whenever DX is called.  
COLOUR: Set drawing colour (32-bit RGBA) to the accumulator, defaults to 0x0. 
TARGETX: Set target position x to the accumulator.  
TARGETY: Set target position y to the accumulator.  

Due to SDL's anti-aliasing making the sketch viewer image potentially imperfect when using the line drawing function, I have included an option to not use any 
lines, and written separate tests for the functions that this affects. This can be toggled by editing the value of the "USING_LINES" constant boolean, which is by default on. fractal.sk comes out to 80.0 KiB if only using blocks.  
You can switch to using 1D RLE by changing the BOX to RLE on line 372. (this should be easier to change but i am lazy)

.sk -> .pgm "compression" uses 2D Run-Length Encoding with some extra steps:  
1: Sort all colours in descending order of occurrences within the .pgm file.  
2: Find an arbitrary pixel of a certain colour. Here I find the first in reading order.  
3: Find all boxes from that pixel that do not overwrite any fixed pixels.  
4: See which box fills in the most new pixels of that colour.  
5: Fill in that box.  
6: Repeat 2-5 until all pixels of that colour filled in.   
7: Fix all pixels of the most recently filled in colour.  
8: Move to the next colour, repeat 2-7 until all colours fully filled in.  


