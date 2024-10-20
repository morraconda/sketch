Converter converts both ways .pgm <-> .sk 
fractal.sk 70,838 bytes if using lines and blocks.
fractal.sk 79,994 bytes if only using blocks.

Input "./converter" to run tests.
Input "./converter [filename]" to convert.

Due to SDL's anti-aliasing making the sketch viewer image potentially imperfect 
when using the line drawing function, I have included an option to not use any 
lines, and written separate tests for the functions that this affects. 

This can be toggled by editing the value of the "USING_LINES" constant boolean, 
which is by default on. 

PGM Encoding Algorithm 2D Run-Length Encoding with some extra steps: 
1: Sort all colours in descending order of occurrences within the .pgm file.
2: Find a pixel of a certain colour. (I find the first in "reading" order)
3: Find all boxes from that pixel that do not overwrite any fixed pixels.
4: See which box fills in the most new pixels of that colour.
5: Fill in that box.
6: Repeat 2-5 until all pixels of that colour filled in.
7: Fix all pixels of the most recently filled in colour.
8: Move to the next colour, repeat 2-7 until all colours fully filled in.


