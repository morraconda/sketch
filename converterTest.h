#ifndef CONVERTERTEST_H
#define CONVERTERTEST_H

void testConverter();

    // basic function tests
void testParseFiletype();
void testOutputFiletype();
void testGreyscaleToRGBA();
void testWriteColour();
void testInitialiseBoard();
void testInitialiseColours();

    // box algorithm tests
void testMove();
void testSet();
void testChangePosition();
void testFindPixel();
void testFindBoxEnd();
void testUpdateBoxBoard();
void testFinalise();
void testFillColour();
void testWriteToSK_BOX();

    // backwards conversion tests
void testSign();
void testRGBAToGreyscale();
void testDrawLine();
void testDrawBox();
void testConvertSKToBoard();

#endif
