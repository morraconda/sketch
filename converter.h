#ifndef CONVERTER_H
#define CONVERTER_H

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern const bool USING_LINES;

enum { DX = 0, DY = 1, TOOL = 2, DATA = 3 }; // opcodes
enum { NONE = 0, LINE = 1,BLOCK = 2, COLOUR = 3, TARGETX = 4, TARGETY = 5,
       SHOW = 6, PAUSE = 7, NEXTFRAME = 8 }; // TOOL operands

enum { INVALID, PGM, SK }; // filetypes
enum { RLE, BOX }; // algorithms

extern const int MAX_FILENAME_LENGTH;
extern const int MAX_PGM_HEADER_CHARS;
extern const int HEIGHT;
extern const int WIDTH;
extern const int GREYSCALE_COLOURS;
extern const int SKETCH_DATA_BITS;
extern const unsigned char SKETCH_DATA_MAX;
extern const int MIN_DX;
extern const int MAX_DX;

extern const int CORRECT; // constants for BOX algorithm
extern const int FIXED;
extern const int NOT_FOUND;

typedef int** board;

typedef struct colourInfo {
    unsigned char greyValue;
    int count;
} colourInfo;

typedef struct position {
    unsigned char x;
    unsigned char y;
} position;

// takes a filename and determines whether it is a .sk or a .pgm
int parseFiletype(char filename[]);

// changes a filename ending from .sk to .pgm, or vice versa
// TO the type of the third argument given
void outputFiletype(char filein[], char fileout[], int type);

// converts a greyscale value to its associated RGBA value
unsigned int greyscaleToRGBA(unsigned char g);

// initialise 200x200 pixel grid based on sk file input stream
board initialiseBoard(FILE *in);

// free allocated memory of a board pointer
void freeBoard(board b);

// initialise all the counts of the 256 colours based on the values
// currently stored in the board
colourInfo *initialiseColourInfo(board b);

// free allocated memory of a pointer to a list of colourinfos
void freeColourInfo(colourInfo* c);

// writes colour change commands to sk file
void writeColour(FILE *out, unsigned int rgba);

// resets position to the top of the board, 1 space right of current position
void resety(FILE *out);

// writes to .sk file commands to move PIXELS in the x or y direction
void move(FILE *out, int pixels, const int axisCode);

// writes to .sk file commands to draw an image from .pgm file
// using run length encoding (RLE) algorithm
void writeToSK_RLE(FILE *out, board b);

// writes to .sk file commands to set location to POS in the x or y direction
void set(FILE *out, unsigned char pos, int AxisCode);

// writes to .sk file commands to move position, then updating the current
// position to where you have just moved
void changePosition(FILE *out, position *current, position next, bool drawingBox);

// sets all CORRECT pixels in a board to be FIXED 
void finalise(board b);

// finds position in board of the first pixel of a colour, in reading order
// assuming you read down to the end of the page first then go right
position findPixel(unsigned char greyValue, board b);

// finds the box with the most unfilled in pixels of a colour without 
// overrwriting any fixed pixels
position findBoxEnd(position startPos, board b, unsigned char greyValue);

// updates board state given a box that has just been filled with a colour
void updateBoxBoard(unsigned char colour, position start, position end, board b);

// writes to .sk file commands to fill all pixels of a certain colour 
// making sure not to overwrite any fixed pixels
void fillColour(FILE *out, board b, unsigned char greyValue, position *currentPos, bool usingLines);

int compareColourInfo(const void *p, const void *q);

// writes to .sk file commands to draw an image from .pgm file
// using BOX algorithm
void writeToSK_BOX(FILE *out, board b, colourInfo c[GREYSCALE_COLOURS], bool usingLines);

void writeToSK(FILE *out, board b, colourInfo c[GREYSCALE_COLOURS], int method, bool usingLines);
// converts a .pgm into a .sk file
void convertToSK(char filein[], bool confirmation, bool usingLines);

// signs an signed 6 bit two's complement number
int sign(unsigned char x);

// converts RGBA colour to its corresponding greyscale value
unsigned char RGBAToGreyscale(unsigned int c);

// updates board state when a line is drawn
// does not support diagonal lines
void drawLine(unsigned char c, position start, position end, int b[HEIGHT][WIDTH]);

// updates board state when a box is drawn
void drawBox(unsigned char c, position start, position end, int b[HEIGHT][WIDTH]);

// writes to board the image drawn from the commands in a .sk file
void convertSKToBoard(FILE *in, int b[HEIGHT][WIDTH]);

// converts a .sk file into a .pgm file
void convertToPGM(char filein[]);

#endif
