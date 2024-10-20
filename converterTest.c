#include "converter.h"
#include "converterTest.h"

void testParseFiletype() {
    assert(parseFiletype("a.pgm") == PGM);
    assert(parseFiletype("b.sk") == SK);
    assert(parseFiletype(".pgm") == INVALID);
    assert(parseFiletype("c.jpg") == INVALID);
    assert(parseFiletype("abcde") == INVALID);
}

void testOutputFiletype() {
    char out[MAX_FILENAME_LENGTH];
    outputFiletype("a.sk", out, PGM);
    assert(strcmp(out, "a.pgm") == 0);
    outputFiletype("b.pgm", out, SK);
    assert(strcmp(out, "b.sk") == 0);
}

void testGreyscaleToRGBA() {
    assert(greyscaleToRGBA(0) == 0xff);
    assert(greyscaleToRGBA(255) == 0xffffffff);
    assert(greyscaleToRGBA(100) == 0x646464ff);
}

void testWriteColour() {
    FILE *out = fopen("testing.txt", "w");
    writeColour(out, 0x000000ff);
    writeColour(out, 0xffffffff);
    writeColour(out, 0x646464ff);
    fclose(out);
 
    // this array corresponds to the correect commands for the above writes, 
    // respectively
    unsigned char commands[20] = {
        0xc0, 0xc0, 0xc0, 0xc3, 0xff, 0x83,
        0xc3, 0xff, 0xff, 0xff, 0xff, 0xff, 0x83,
        0xc1, 0xe4, 0xd9, 0xc6, 0xd3, 0xff, 0x83
    };
   
    FILE *in = fopen("testing.txt", "r");
    for (int i=0; i<20; i++) {
        unsigned char ch = fgetc(in);
        assert(ch == commands[i]);
    }
    fclose(in);
}

void testInitialiseBoard() {
    FILE *in = fopen("bands.pgm", "r");
    // discard file header as this is a known correct format pgm file
    char discard[MAX_PGM_HEADER_CHARS];
    fgets(discard, MAX_PGM_HEADER_CHARS, in); 
    board b = initialiseBoard(in);

    // close and reopen file so we can start reading from the beginning
    // of the file again to check it matches the generated board
    fclose(in);
    in = fopen("bands.pgm", "r");
    fgets(discard, MAX_PGM_HEADER_CHARS, in);
    
    for (int i=0; i<HEIGHT; i++) {
        for (int j=0; j<WIDTH; j++) {
            unsigned char ch = fgetc(in);
            assert(b[i][j] == ch);
        }
    }
    freeBoard(b);
    fclose(in);
}

void testInitialiseColours() {
    FILE *in = fopen("bands.pgm", "r");
    char discard[MAX_PGM_HEADER_CHARS];
    fgets(discard, MAX_PGM_HEADER_CHARS, in); 
    board b = initialiseBoard(in);
    fclose(in);

    // count all instances of each of the 256 colours, individually
    // check if the count matches
    colourInfo *c = initialiseColourInfo(b);
    for (int i=0; i<GREYSCALE_COLOURS; i++) {
        int count = 0;
        for (int j=0; j<HEIGHT; j++) {
            for (int k=0; k<WIDTH; k++) {
                if (b[j][k] == i) count++;
            }
        }
        assert(c[i].count == count);
    }
    freeColourInfo(c);
    freeBoard(b);
}

void testMove() {
    FILE *out = fopen("testing.txt", "w");
    move(out, 0, DY);
    move(out, 1, DY);
    move(out, 31, DY);
    move(out, 32, DY);
    move(out, 150, DY);

    move(out, 0, DX); // should not output a command
    move(out, 1, DX);
    move(out, 31, DX);
    move(out, 32, DX);
    move(out, 150, DX);

    move(out, -1, DY);
    move(out, -32, DY);
    move(out, -33, DY);
    move(out, -150, DX);

    fclose(out);
    unsigned char commands[40] = {
        0x40, 
        0x41, 
        0x5f, 
        0x5f, 0x41,
        0x5f, 0x5f, 0x5f, 0x5f, 0x5a,

        0x01, 
        0x1f, 
        0x1f, 0x01,
        0x1f, 0x1f, 0x1f, 0x1f, 0x1a,

        0x7f,
        0x60,
        0x60, 0x7f,
        0x20, 0x20, 0x20, 0x20, 0x2a
    };
    FILE *in = fopen("testing.txt", "r");
    for (int i=0; i<28; i++) {
        unsigned char ch = fgetc(in);
        assert(commands[i] == ch);
    }
    fclose(in);    
}

void testSet() {
    FILE *out = fopen("testing.txt", "w");
    set(out, 0, TARGETX);
    set(out, 32, TARGETX);
    set(out, 63, TARGETX);
    set(out, 64, TARGETX);
    set(out, 94, TARGETX);
    set(out, 128, TARGETX);

    set(out, 0, TARGETY);
    set(out, 32, TARGETY);
    set(out, 63, TARGETY);
    set(out, 64, TARGETY);
    set(out, 94, TARGETY);
    set(out, 128, TARGETY);

    fclose(out);
    unsigned char commands[40] = {
        0x84,
        0xe0, 0x84,
        0xff, 0x84,
        0xff, 0x84, 0x01,
        0xff, 0x84, 0x1f,
        0xc2, 0xc0, 0x84,

        0x85, 0x40,
        0xe0, 0x85, 0x40,
        0xff, 0x85, 0x40,
        0xff, 0x85, 0x41,
        0xff, 0x85, 0x5f,
        0xc2, 0xc0, 0x85, 0x40
    };
    FILE *in = fopen("testing.txt", "r");
    for (int i=0; i<32; i++) {
        unsigned char ch = fgetc(in);
        assert(commands[i] == ch);
    }
    fclose(in);  
}

void testChangePosition() {
    FILE *out = fopen("testing.txt", "w");
    position current = (position) {0, 0};
    position next = (position) {0, 0};

    changePosition(out, &current, next, false);
    next.x = 31; next.y = 31;
    changePosition(out, &current, next, false);
    next.x = 63; next.y = 63;
    changePosition(out, &current, next, false);
    next.x = 95; next.y = 95;
    changePosition(out, &current, next, true);
    next.x = 62; next.y = 62;
    changePosition(out, &current, next, false);
    next.x = 1; next.y = 0;
    changePosition(out, &current, next, false);

    fclose(out);

    unsigned char commands[30] = {
        0x40,
        0x1f, 0x5f,
        0xff, 0x84, 0x5f, 0x41,
        0x1f, 0x01, 0xc1, 0xdf, 0x85, 0x40,
        0xfe, 0x84, 0x60, 0x7f,
        0x84, 0x01, 0x60, 0x62
    };  

    FILE *in = fopen("testing.txt", "r");
    for (int i=0; i<21; i++) {
        unsigned char ch = fgetc(in);
        assert(commands[i] == ch);
    }
    fclose(in);     
}

void testFindPixel() {
    FILE *in = fopen("bands.pgm", "r");
    char discard[MAX_PGM_HEADER_CHARS];
    fgets(discard, MAX_PGM_HEADER_CHARS, in); 
    board b = initialiseBoard(in);
    fclose(in);
    
    position pixel = findPixel(0, b);
    assert(pixel.x == 0 && pixel.y == 0); 

    pixel = findPixel(255, b); 
    assert(pixel.x == 0 && pixel.y == 180);

    b[199][167] = 254;
    pixel = findPixel(254, b);
    assert(pixel.x == 167 && pixel.y == 199);
    freeBoard(b);
}

void testFindBoxEnd() {
    FILE *in = fopen("bands.pgm", "r");
    char discard[MAX_PGM_HEADER_CHARS];
    fgets(discard, MAX_PGM_HEADER_CHARS, in); 
    board b = initialiseBoard(in);
    fclose(in);

    position start = (position) {0, 0};
    position end = findBoxEnd(start, b, 255);
    assert(end.x == 200 && end.y == 200);

    b[150][100] = FIXED;
    end = findBoxEnd(start, b, 255);
    assert(end.x == 100 && end.y == 200);

    b[185][0] = FIXED;
    end = findBoxEnd(start, b, 255);
    assert(end.x == 100 && end.y == 185);
    
    b[100][0] = FIXED;
    end = findBoxEnd(start, b, 113);
    assert(end.x == 200 && end.y == 100);

    b[1][0] = FIXED;
    end = findBoxEnd(start, b, 0);
    assert(end.x == 200 && end.y == 1);

    b[0][1] = FIXED;
    end = findBoxEnd(start, b, 0);
    assert(end.x == 1 && end.y == 1);
    freeBoard(b);
}

void testUpdateBoxBoard() {
    FILE *in = fopen("bands.pgm", "r");
    char discard[MAX_PGM_HEADER_CHARS];
    fgets(discard, MAX_PGM_HEADER_CHARS, in); 
    board b = initialiseBoard(in);
    fclose(in);

    updateBoxBoard(226, (position) {10, 150}, (position) {50, 170}, b);
    for (int i=0; i<HEIGHT; i++) {
        for (int j=0; j<WIDTH; j++) {
            if (160 <= i && i < 170 && 10 <= j && j < 50) {
                assert(b[i][j] == CORRECT);
                }
            else assert(b[i][j] != CORRECT); 
        }
    }
    freeBoard(b);
}

void testFinalise() {
    FILE *in = fopen("bands.pgm", "r");
    char discard[MAX_PGM_HEADER_CHARS];
    fgets(discard, MAX_PGM_HEADER_CHARS, in); 
    board b = initialiseBoard(in);
    fclose(in);

    updateBoxBoard(226, (position) {10, 150}, (position) {50, 170}, b);
    finalise(b);
    for (int i=0; i<HEIGHT; i++) {
        for (int j=0; j<WIDTH; j++) {
            if (160 <= i && i < 170 && 10 <= j && j < 50) {
                assert(b[i][j] == FIXED);
                }
            else {
                assert(b[i][j] != CORRECT);
                assert(b[i][j] != FIXED); 
            }
        }
    }
    freeBoard(b);
}

void testFillColour() {
    FILE *in = fopen("bands.pgm", "r");
    board b = initialiseBoard(in);
    fclose(in);

    for(int i=0; i<HEIGHT; i++) {
        for (int j=0; j<WIDTH; j++) {b[i][j] = 100;}
    }
    b[0][0] = 0;
    b[31][31] = 0;
    b[31][32] = 0;
    b[32][33] = 1;

    FILE *out = fopen("testing.txt", "w");
    position currentPos = (position) {0, 0};
    fillColour(out, b, 100, &currentPos, USING_LINES);
    finalise(b);
    fillColour(out, b, 0, &currentPos, USING_LINES);
    finalise(b);
    fillColour(out, b, 1, &currentPos, USING_LINES);

    fclose(out);
    freeBoard(b);

    in = fopen("testing.txt", "r");
    if (USING_LINES) {
        unsigned char commands[33] = {
            0x80, 0x41, // move by (0, 1) to (0, 1)
            0x82, 0xc3, 0xc8, 0x84, 0xc3, 0xc8, 0x85, 0x40, // box to (200, 200)
            0x80, 0x84, 0x01, 0x85, 0x40, // set position to (1, 0)
            0x82, 0xc3, 0xc8, 0x84, 0x41, // box to (200, 1)

            0x80, 0x84, 0x7f, // set position to (0, 0)
            0x81, 0x40, // fill in (0, 0)
            0x80, 0x1f, 0x5f, // move by (31, 31) to (31, 31)
            0x82, 0x02, 0x41, // box by (2, 1) to (33, 32)
            
            0x81, 0x40 // fill in (33, 22)
        };
        for (int i=0; i<33; i++) {
            unsigned char ch = fgetc(in);
            assert(commands[i] == ch);
        }
    }
    else {
        unsigned char commands[35] = {
            0x80, 0x41, // move by (0, 1) to (0, 1)
            0x82, 0xc3, 0xc8, 0x84, 0xc3, 0xc8, 0x85, 0x40, // box to (200, 200)
            0x80, 0x84, 0x01, 0x85, 0x40, // set position to (1, 0)
            0x82, 0xc3, 0xc8, 0x84, 0x41, // box to (200, 1)

            0x80, 0x84, 0x7f, // set position to (0, 0)
            0x82, 0x01, 0x41, // box by (1, 1) to (1, 1)
            0x80, 0x1e, 0x5e, // move by (30, 30) to (31, 31)
            0x82, 0x02, 0x41, // box by (2, 1) to (33, 32)
            0x82, 0x01, 0x41 // box by (1, 1) to (34, 33)
        };
        for (int i=0; i<35; i++) {
            unsigned char ch = fgetc(in);
            assert(commands[i] == ch);
        }   
    }
    fclose(in);
}

void testWriteToSK_BOX() {
    FILE *in = fopen("bands.pgm", "r");
    board b = initialiseBoard(in);
    fclose(in);
    for(int i=0; i<HEIGHT; i++) {
        for (int j=0; j<WIDTH; j++) {b[i][j] = 100;}
    }

    b[1][1] = 0;
    b[198][197] = 255;
    b[199][197] = 255;
    colourInfo *c = initialiseColourInfo(b);

    FILE *out = fopen("testing.txt", "w");
    writeToSK_BOX(out, b, c, USING_LINES);

    fclose(out);
    freeColourInfo(c);
    freeBoard(b);

    in = fopen("testing.txt", "r");
    if (USING_LINES) {
        unsigned char commands[40] = {
            0xc1, 0xe4, 0xd9, 0xc6, 0xd3, 0xff, 0x83, // set colour to 100
            0x82, 0xc3, 0xc8, 0x84, 0xc3, 0xc8, 0x85, 0x40, // box to (200, 200)
            0xc3, 0xff, 0xff, 0xff, 0xff, 0xff, 0x83, // set colour to 255
            0x80, 0x3d, 0x7e, // move by (-3, -2) to (197, 198)
            0x81, 0x41, // line 1 down to (197, 199)
            0xc0, 0xc0, 0xc0, 0xc3, 0xff, 0x83, // set colour to 0
            0x80, 0x84, 0x01, 0x85, 0x41, // set position to (1, 1)
            0x81, 0x40 // fill in (1, 1)
        };
        for (int i=0; i<40; i++) {
        unsigned char ch = fgetc(in);
        assert(commands[i] == ch);
        }
    }
    else {
        unsigned char commands[42] = {
            0xc1, 0xe4, 0xd9, 0xc6, 0xd3, 0xff, 0x83, // set colour to 100
            0x82, 0xc3, 0xc8, 0x84, 0xc3, 0xc8, 0x85, 0x40, // box to (200, 200)
            0xc3, 0xff, 0xff, 0xff, 0xff, 0xff, 0x83, // set colour to 255
            0x80, 0x3d, 0x7e, // move by (-3, -2) to (197, 198)
            0x82, 0x01, 0x42, // box by (1, 2) to (198, 200)
            0xc0, 0xc0, 0xc0, 0xc3, 0xff, 0x83, // set colour to 0
            0x80, 0x84, 0x01, 0x85, 0x41, // set position to (1, 1)
            0x82, 0x01, 0x41 // box by (1, 1) to (2, 2)
        };
        for (int i=0; i<42; i++) {
        unsigned char ch = fgetc(in);
        assert(commands[i] == ch);
        }
    }
    fclose(in);
}

void testSign() {
    assert(sign(0) == 0);
    assert(sign(1) == 1);
    assert(sign(15) == 15);
    assert(sign(31) == 31);
    assert(sign(32) == -32);
    assert(sign(48) == -16);
    assert(sign(63) == -1);
}

void testRGBAToGreyscale() {
    assert(RGBAToGreyscale(0x000000ff) == 0);
    assert(RGBAToGreyscale(0x444444ff) == 0x44);
    assert(RGBAToGreyscale(0xffffffff) == 0xff);
}

void testDrawLine() {
    int b[HEIGHT][WIDTH];
    for(int i=0; i<HEIGHT; i++) {for (int j=0; j<WIDTH; j++) {b[i][j] = 0xff;}}
    position start = (position) {1, 1};
    position end = (position) {1, 11};
    drawLine(0, start, end, b);
    for(int i=0; i<HEIGHT; i++) {
        for(int j=0; j<WIDTH; j++) {
            if (1 <= i && i <= 11 && j == 1) assert(b[i][j] == 0);
            else assert(b[i][j] == 0xff); 
        }
    }

    for(int i=0; i<HEIGHT; i++) {for (int j=0; j<WIDTH; j++) {b[i][j] = 0xff;}}
    end = (position) {11, 1};
    drawLine(100, start, end, b);
    for(int i=0; i<HEIGHT; i++) {
        for(int j=0; j<WIDTH; j++) {
            if (i == 1 & 1 <= j && j <= 11) assert(b[i][j] == 100);
            else assert(b[i][j] == 0xff); 
        }
    }
}

void testDrawBox() {
    int b[HEIGHT][WIDTH];
    for(int i=0; i<HEIGHT; i++) {for (int j=0; j<WIDTH; j++) {b[i][j] = 0xff;}}
    position start = (position) {1, 1};
    position end = (position) {11, 11};

    drawBox(0, start, end, b);
    for(int i=0; i<HEIGHT; i++) {
        for(int j=0; j<WIDTH; j++) {
            if (1 <= i && i < 11 && 1 <= j && j < 11) assert(b[i][j] == 0);
            else assert(b[i][j] == 0xff); 
        }
    }
}

void testConvertSKToBoard() {
    FILE *in = fopen("fractal.pgm", "rb");
    char discard[MAX_PGM_HEADER_CHARS];
    fgets(discard, MAX_PGM_HEADER_CHARS, in); 
    board original = initialiseBoard(in);
    fclose(in);

    convertToSK("fractal.pgm", false, USING_LINES);
    in = fopen("fractal.sk", "rb");
    int new[HEIGHT][WIDTH];
    convertSKToBoard(in, new);
    for(int i=0; i<HEIGHT; i++) {
        for(int j=0; j<WIDTH; j++) {
            assert(original[i][j] == new[i][j]);
        }
    }
    freeBoard(original);
    fclose(in);
}

void testConverter() {
    printf("Running Tests\n");
    // basic function tests
    testParseFiletype();
    testOutputFiletype();
    testGreyscaleToRGBA();
    testWriteColour();
    testInitialiseBoard();
    testInitialiseColours();
    printf("Basic Function Tests Passed\n");

    // box algorithm tests
    testMove();
    testSet();
    testChangePosition();
    testFindPixel();
    testFindBoxEnd();
    testUpdateBoxBoard();
    testFinalise();
    testFillColour();
    testWriteToSK_BOX();
    printf(".pgm -> .sk 2D RLE Conversion Algorithm Tests Passed\n");

    // backwards conversion tests
    testSign();
    testRGBAToGreyscale();
    testDrawLine();
    testDrawBox();
    testConvertSKToBoard();
    printf(".sk -> .pgm Reverse Conversion Tests Passed\n");
    printf("All Tests Passed\n");
}
