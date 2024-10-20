#include "converter.h"
#include "converterTest.h"

const bool USING_LINES = true;

const int MAX_FILENAME_LENGTH = 100;
const int MAX_PGM_HEADER_CHARS = 20;
const int HEIGHT = 200;
const int WIDTH = 200;
const int GREYSCALE_COLOURS = 256;
const int SKETCH_DATA_BITS = 6;
const unsigned char SKETCH_DATA_MAX = (1 << SKETCH_DATA_BITS) - 1;
const int MIN_DX = -32;
const int MAX_DX = 31;

const int CORRECT = -1; // constants for BOX algorithm
const int FIXED = -2;
const int NOT_FOUND = 255;

// takes a filename and determines whether it is a .sk or a .pgm
int parseFiletype(char filename[]) {
    int len = strlen(filename);

    if (len > 3 && filename[len-3] == '.' && filename[len-2] == 's'
        && filename[len-1] == 'k') return SK;

    if (len > 4 && filename[len-4] == '.' && filename[len-3] == 'p' 
        && filename[len-2] == 'g' && filename[len-1] == 'm') return PGM;
    
    else return INVALID;
}

// changes a filename ending from .sk to .pgm, or vice versa
// TO the type of the third argument given
void outputFiletype(char filein[], char fileout[], int type) {
    int len = strlen(filein);
    strcpy(fileout, filein);
    if (type == PGM) {
        fileout[len-2] = 'p';
        fileout[len-1] = 'g';
        fileout[len] = 'm';
        fileout[len+1] = '\0';
    }

    else if (type == SK) {
        fileout[len-3] = 's';
        fileout[len-2] = 'k';
        fileout[len-1] = '\0';
    }
}

// converts a greyscale value to its associated RGBA value
unsigned int greyscaleToRGBA(unsigned char g) {
    unsigned int uintG = g;
    unsigned int result = (uintG << 24) + (uintG << 16) + (uintG << 8) + 0xff;
    return result;
}

// initialise 200x200 pixel grid based on sk file input stream
board initialiseBoard(FILE *in) {
    board b = malloc(HEIGHT * sizeof(int*));
    for(int i=0; i<HEIGHT; i++) {
        b[i] = malloc(WIDTH * sizeof(int));
        for (int j=0; j<WIDTH; j++) {b[i][j] = fgetc(in);}
    }
    return b;
}

// free allocated memory of a board pointer
void freeBoard(board b) {
    for(int i=0; i<HEIGHT; i++) {free(b[i]);}
    free(b);
}

// initialise all the counts of the 256 colours based on the values
// currently stored in the board
colourInfo *initialiseColourInfo(board b) {
    colourInfo *c = malloc(GREYSCALE_COLOURS * sizeof(colourInfo));

    // first initialise the 256 colours and set counts to 0;
    for (int i=0; i<GREYSCALE_COLOURS; i++) {
        c[i].greyValue = i;
        c[i].count = 0;
    }

    // increment the count of that colour every time it's seen in the board
    for (int i=0; i<HEIGHT; i++) {
        for (int j=0; j<WIDTH; j++) {
            int colour = b[i][j];
            c[colour].count += 1;
        }
    }
    return c;
}

// free allocated memory of a pointer to a list of colourinfos
void freeColourInfo(colourInfo* c) {free(c);}

// writes colour change commands to sk file
void writeColour(FILE *out, unsigned int rgba) {
    // 6 is the maximum number of data commands required to change colour, 
    // as you need to encode 32 bits in 6-bit operands (5 < 32/6 <= 6) 
    for(int i=5; i>=0; i--) {
        unsigned char opcode = DATA << SKETCH_DATA_BITS;
        unsigned char operand = (rgba >> i * SKETCH_DATA_BITS) & 0x3f;
        unsigned char command = opcode + operand;
        if (operand != 0 || i != 5) fputc(command, out);
    }
    unsigned char command = (TOOL << SKETCH_DATA_BITS) + COLOUR;
    fputc(command, out);
}

// resets position to the top of the board, 1 space right of current position
void resety(FILE *out) {
    fputc(0x80, out); // set tool to NONE
    fputc(0x85, out); // set targetY to 0
    fputc(0x01, out); // dx by 1
    fputc(0x40, out); // set x, y to tx, ty
    fputc(0x81, out); // set tool to LINE
}

// writes to .sk file commands to move PIXELS in the x or y direction
void move(FILE *out, int pixels, const int axisCode) {
    unsigned char opcode = axisCode << 6;
    // have to call DY even if y doesn't change to update current x & y
    if (pixels == 0 && axisCode == DY) fputc(0x40, out); 
    while (pixels != 0) {
        // DX/DY can increment max 31 pixels per command
        // or decrement max -32
        int offset;
        if (pixels > 0) offset = (pixels > MAX_DX) ? MAX_DX : pixels; 
        else offset = (pixels < MIN_DX) ? MIN_DX : pixels;
        pixels -= offset;
        // operand is unsigned so must convert any negative offset
        unsigned char operand = (offset < 0) ? offset + SKETCH_DATA_MAX + 1 : offset;
        unsigned char command = opcode + operand;
        fputc(command, out);  
    }
}

// writes to .sk file commands to draw an image from .pgm file
// using run length encoding (RLE) algorithm
void writeToSK_RLE(FILE *out, board b) {
    unsigned char currentColour = 255; 
    for (int i=0; i<HEIGHT; i++) {
        // recheck for colour mismatch at the start of every column
        if (currentColour != b[0][i]) {
            currentColour = b[0][i];
            writeColour(out, greyscaleToRGBA(currentColour));
            }
        int dy = 0;
        // scan vertically down as dy updates current x and y but not dx
        for (int j=0; j<WIDTH; j++) {
            // if different colour detected, draw a line downwards 
            // to the current point then change the colour
            if (currentColour != b[j][i]) {
                move(out, dy, DY);
                dy = 1;
                currentColour = b[j][i];
                writeColour(out, greyscaleToRGBA(currentColour));
            }
            else dy++;
        }
        // once reached the bottom of the image, draw a line and reset to the top
        move(out, dy, DY);            
        if (i < 199) resety(out);
    }       
}

// writes to .sk file commands to set location to POS in the x or y direction
void set(FILE *out, unsigned char pos, int AxisCode) {
    unsigned char dataOpcode = DATA << SKETCH_DATA_BITS;
    unsigned char AxisCommand = (TOOL << SKETCH_DATA_BITS) + AxisCode;
    // 95-199: 2 data commands (3-4 bytes)
    if (pos > SKETCH_DATA_MAX + MAX_DX) {
        fputc(dataOpcode + (pos >> SKETCH_DATA_BITS), out);
        pos &= SKETCH_DATA_MAX;
        fputc(dataOpcode + pos, out);
        fputc(AxisCommand, out);
        // need to call DY to update position if not already called
        if (AxisCode == TARGETY) fputc(0x40, out); 
    }
    // 32-94: 1 data command, then call move for DX/DY (2-3 bytes)
    else if (pos > MAX_DX){
        unsigned char operand = (pos > SKETCH_DATA_MAX) ? SKETCH_DATA_MAX : pos;
        fputc(dataOpcode + operand, out);
        fputc(AxisCommand, out);

        if (AxisCode == TARGETX) AxisCode = DX;
        else if (AxisCode == TARGETY) AxisCode = DY;
        else exit(-1);
        int pixels = pos - operand;
        move(out, pixels, AxisCode);
    }

    // 0: 0 data commands (1-2 bytes)
    else {
        if (AxisCode == TARGETX) AxisCode = DX;
        else if (AxisCode == TARGETY) AxisCode = DY;
        fputc(AxisCommand, out);
        move(out, pos, AxisCode);
    }
}

// writes to .sk file commands to move position, then updating the current
// position to where you have just moved
void changePosition(FILE *out, position *current, position next, bool drawingBox) {
    int diffX = next.x - current->x;
    int diffY = next.y - current->y;
    // if statements to determine whether to set targetX/targetY, or move with
    // DX/DY based on how many instructions it would take (hardcoded nonsense)
    if (MIN_DX <= diffX && diffX <= MAX_DX) move(out, diffX, DX); // 1 command
    else if (0 <= next.x && next.x <= SKETCH_DATA_MAX) set(out, next.x, TARGETX); // 1-2 commands
    else if (MIN_DX * 2 <= diffX && diffX <= MAX_DX * 2) move(out, diffX, DX); // 2 commands
    else set(out, next.x, TARGETX); // 3 commands
    
    if (MIN_DX <= diffY && diffY <= MAX_DX) move(out, diffY, DY); // 1 command
    // 2-3 commands, can only be used if not drawing a box otherwise it won't fill in the box
    else if (!drawingBox && MIN_DX * 3 <= diffY && diffY <= MAX_DX * 3) {
        move(out, diffY, DY);
    }
    else set(out, next.y, TARGETY); // 2-4 commands
    *current = next;
}

// sets all CORRECT pixels in a board to be FIXED 
void finalise(board b) {
    for (int i=0; i<HEIGHT; i++) {
        for (int j=0; j<WIDTH; j++) {
            if (b[i][j] == CORRECT) b[i][j] = FIXED;
        }
    }
}

// finds position in board of the first pixel of a colour, in reading order
// assuming you read down to the end of the page first then go right
position findPixel(unsigned char greyValue, board b) {
    for (int i=0; i<HEIGHT; i++) {
        for (int j=0; j<WIDTH; j++) {
            if (b[j][i] == greyValue) {
                return (position) {i, j};
            }
        }
    }
    return (position) {NOT_FOUND, NOT_FOUND};
}

// finds the box with the most unfilled in pixels of a colour without 
// overrwriting any fixed pixels
position findBoxEnd(position startPos, board b, unsigned char greyValue) {
    position endPos = startPos;
    bool validLine = true;
    int maxCount = 0;
    // iterates through x values
    for (int i=startPos.x; i<200 && validLine; i++) {
        if (b[startPos.y][i] == FIXED) validLine = false;

        bool validBox = true;
        int boxCount = 0;
        // iterates through y values
        for (int j=startPos.y; j<200 && validBox && validLine; j++) {
            int lineCount = 0;
            // checks if the line from (startPos.x, j) to (i, j) is valid
            for (int k=startPos.x; k<=i && validBox; k++) {
                if (b[j][k] == FIXED) {
                    validBox = false;
                    j--;
                }
                else if (b[j][k] == greyValue) lineCount++;
            }
            // add the line's good pixels to the box's if the line was valid
            if (validBox) boxCount += lineCount;
            // see if the output box is the new best box
            if (boxCount > maxCount) {
                maxCount = boxCount;
                endPos = (position) {i, j};
            }
        }
    }
    endPos.x++; endPos.y++; // block implentation in .sk format is off by one.
    return endPos;
}

// updates board state given a box that has just been filled with a colour
void updateBoxBoard(unsigned char colour, position start, position end, board b) {
    for (int i=start.y; i<end.y; i++) { 
        for (int j=start.x; j<end.x; j++) {
            if (b[i][j] == colour) b[i][j] = CORRECT;
        }
    }
}

// writes to .sk file commands to fill all pixels of a certain colour 
// making sure not to overwrite any fixed pixels
void fillColour(FILE *out, board b, unsigned char greyValue, position *currentPos, bool usingLines) {
    position nextPos = findPixel(greyValue, b);
    while (nextPos.x != NOT_FOUND) {
        // set tool to NONE and move if you need to move
        if (!(currentPos->x == nextPos.x && currentPos->y == nextPos.y)) {
            fputc(0x80, out); 
            changePosition(out, currentPos, nextPos, false); // goto pixel of colour
        } 
        
        nextPos = findBoxEnd(*currentPos, b, greyValue);
        updateBoxBoard(greyValue, *currentPos, nextPos, b);
        // if block is a single pixel wide, use a LINE instead
        // this saves a single [DX 1] instruction over using a BLOCK
        if (currentPos->x + 1 == nextPos.x && usingLines) {
            fputc(0x81, out);
            nextPos.x--; nextPos.y--;
        }
        else fputc(0x82, out); // set tool to BLOCK otherwise
        changePosition(out, currentPos, nextPos, true); 
        nextPos = findPixel(greyValue, b);
    }
}

int compareColourInfo(const void *p, const void *q) {
    colourInfo *x = (colourInfo*)p;
    colourInfo *y = (colourInfo*)q;
    int difference = x->count - y->count;
    if (difference > 0) return -1;
    else if (difference < 0) return 1;
    else return 0;
}

// writes to .sk file commands to draw an image from .pgm file
// using BOX algorithm
void writeToSK_BOX(FILE *out, board b, colourInfo c[GREYSCALE_COLOURS], bool usingLines) {
    // sorts all 256 colours in descending order based on their count
    qsort(c, GREYSCALE_COLOURS, sizeof(colourInfo), compareColourInfo);
    position *currentPos = malloc(sizeof(position));
    *currentPos = (position) {0, 0};
    for (int i=0; i<GREYSCALE_COLOURS; i++) {
        if (c[i].count > 0) {
            unsigned char colour = c[i].greyValue;
            writeColour(out, greyscaleToRGBA(colour));
            // special case for the first colour, just fill the entire grid
            // with that colour
            if (i == 0) {
                fputc(0x82, out);
                updateBoxBoard(colour, *currentPos, (position) {HEIGHT, WIDTH}, b);
                changePosition(out, currentPos, (position) {HEIGHT, WIDTH}, true);
                }
            else fillColour(out, b, colour, currentPos, usingLines);
            finalise(b); // set all CORRECT pixels to FIXED so they don't get overwritten
        }
    } 
    free(currentPos);  
}

void writeToSK(FILE *out, board b, colourInfo c[GREYSCALE_COLOURS], int method, bool usingLines) {
    if (method == RLE) writeToSK_RLE(out, b);
    else if (method == BOX) writeToSK_BOX(out, b, c, usingLines);
}

// converts a .pgm into a .sk file
void convertToSK(char filein[], bool confirmation, bool usingLines) {
    FILE *in = fopen(filein, "r");

    // check if the .pgm file is a 200x200 image with max greyscale value 255
    char header[MAX_PGM_HEADER_CHARS];
    fgets(header, MAX_PGM_HEADER_CHARS, in);

    // if so, converts the file to a .sk
    if (strcmp(header, "P5 200 200 255\n") == 0) {
        board b = initialiseBoard(in);
        colourInfo *c = initialiseColourInfo(b);

        char fileout[MAX_FILENAME_LENGTH];
        outputFiletype(filein, fileout, SK);
        FILE *out = fopen(fileout, "wb");
        writeToSK(out, b, c, BOX, usingLines);

        // free all allocated memory, write success message
        freeBoard(b);
        freeColourInfo(c);
        fclose(in);
        fclose(out);
        if (confirmation) printf("File %s has been written.\n", fileout);
    }

    else {
        printf("Error: .pgm file header mismatch\n");
        fclose(in);
    }
}

// signs an signed 6 bit two's complement number
int sign(unsigned char x) { return (x > 31) ? x - 64 : x; }

// converts RGBA colour to its corresponding greyscale value
unsigned char RGBAToGreyscale(unsigned int c) { return (c >> 8) & 0xff; }

// updates board state when a line is drawn
// does not support diagonal lines
void drawLine(unsigned char c, position start, position end, int b[HEIGHT][WIDTH]) {
    if(start.x != end.x && start.y != end.y) {
        printf("Diagonal Lines are not supported.\n");
        exit(-1);
    }
    for(int i=start.y; i<=end.y; i++) {
        for (int j=start.x; j<=end.x; j++) {
            b[i][j] = c;
        }
    }
}

// updates board state when a box is drawn
void drawBox(unsigned char c, position start, position end, int b[HEIGHT][WIDTH]) {
    for(int i=start.y; i<end.y; i++) {
        for (int j=start.x; j<end.x; j++) {
            b[i][j] = c;
        }
    }
}

// writes to board the image drawn from the commands in a .sk file
void convertSKToBoard(FILE *in, int b[HEIGHT][WIDTH]) {
    // initialise state variables
    int tool = LINE; unsigned char colour = 0; unsigned int data = 0;
    position currentPos = (position) {0, 0}; 
    position nextPos = (position) {0, 0};

    // reads commands from .sk file 
    while(!feof(in)) {
        unsigned char command = fgetc(in);
        unsigned char opcode = command >> 6;
        unsigned char operand = command & 0x3f;
        
        // DX and DY require a signed operand
        if (opcode == DX) nextPos.x += sign(operand);
        else if (opcode == DY) {
            nextPos.y += sign(operand);
            if (tool == LINE) drawLine(colour, currentPos, nextPos, b);
            else if (tool == BLOCK) drawBox(colour, currentPos, nextPos, b);
            currentPos = nextPos;
        }
        // set tool if it's none/line/block, otherwise do the relevant function
        // ignores SHOW, PAUSE instructions as they do not affect the final
        // output of the file
        else if (opcode == TOOL) {
            if (operand == NONE || operand == LINE || operand == BLOCK) tool = operand;
            else if (operand == COLOUR) colour = RGBAToGreyscale(data);
            else if (operand == TARGETX) nextPos.x = data;
            else if (operand == TARGETY) nextPos.y = data;
            data = 0;
        }
        else if (opcode == DATA) {data <<= 6; data += operand;}    
    }
}

// converts a .sk file into a .pgm file
void convertToPGM(char filein[]) {
    FILE *in = fopen(filein, "r");
    char fileout[MAX_FILENAME_LENGTH];
    outputFiletype(filein, fileout, PGM);

    // initialise board with all black pixels
    int b[HEIGHT][WIDTH];
    for(int i=0; i<HEIGHT; i++) {for (int j=0; j<WIDTH; j++) {b[i][j] = 0xff;}}
    convertSKToBoard(in, b); // fill in the board with the correct pixels

    FILE *out = fopen(fileout, "wb");
    fputs("P5 200 200 255\n", out); // PGM File Header
    // output board into pgm output file
    for(int i=0; i<HEIGHT; i++) {for (int j=0; j<WIDTH; j++) {fputc(b[i][j], out);}}
    
    fclose(out);
    fclose(in);
    printf("File %s has been written.\n", fileout);
}

int main(int n, char *args[n]) {
    if (n == 1) testConverter(); // runs tests if no arguments provided
    // attempts to convert file if a filename is provided
    else if (n == 2) { 
        char filename[MAX_FILENAME_LENGTH];
        strcpy (filename, args[1]);
        int type = parseFiletype(filename);

        if (type == INVALID) {
        printf("Error: File provided not a valid .pgm nor .sk file\n");
            return -1;
        }
        else if (type == PGM) {
            convertToSK(filename, true, USING_LINES); 
            return 0;
        }
        else if (type == SK) {
            convertToPGM(filename); 
            return 0;
        }
    }
    // if wrong number of arguments provided, print a usage hint
    else {
        printf("Use ./converter [filename]\n"); 
        return -1;
    } 
}
