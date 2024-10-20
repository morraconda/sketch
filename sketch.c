// Basic program skeleton for a Sketch File (.sk) Viewer
#include "displayfull.h"
#include "sketch.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// typedef struct state { int x, y, tx, ty; unsigned char tool; 
// unsigned int start, data; bool end;} state;

// Allocate memory for a drawing state and initialise it
state *newState(void) {
  state *p = malloc(sizeof(state));
  *p = (state) {0, 0, 0, 0, LINE, 0, 0, false};
  return p; 
}

// Release all memory associated with the drawing state
void freeState(state *s) {
  free(s);
}

// Extract an opcode from a byte (two most significant bits).
int getOpcode(byte b) {
  return b >> 6;
}

// Extract an operand (-32..31) from the rightmost 6 bits of a byte.
int getOperand(byte b) {
  b &= 0x3F;
  return (b < 32) ? b : b - 64; 
}

int unsign(int x) {return (x < 0) ? x + 64 : x;}

// Execute the next byte of the command sequence.
void obey(display *d, state *s, byte op) {
  int inst = getOpcode(op);
  int operand = getOperand(op);

  if (inst == DX) s->tx += operand;
  else if (inst == DY) {
    s->ty += operand;
    if (s->tool == LINE) line(d, s->x, s->y, s->tx, s->ty);
    else if (s->tool == BLOCK) block(d, s->x, s->y, s->tx - s->x, s->ty - s->y);
    s->x = s->tx; s->y = s->ty;
  }
  else if (inst == TOOL) {
    if (operand == NONE || operand == LINE | operand == BLOCK) s->tool = operand;
    else if (operand == COLOUR) colour(d, s->data);
    else if (operand == TARGETX) s->tx = s->data;
    else if (operand == TARGETY) s->ty = s->data;
    else if (operand == SHOW) show(d);
    else if (operand == PAUSE) pause(d, s->data);
    else if (operand == NEXTFRAME) s->end = true;
    s->data = 0;
  }
  else if (inst == DATA) {s->data <<= 6; s->data += unsign(operand);}
}

// Draw a frame of the sketch file. For basic and intermediate sketch files
// this means drawing the full sketch whenever this function is called.
// For advanced sketch files this means drawing the current frame whenever
// this function is called.

bool processSketch(display *d, const char pressedKey, void *data) {
    //NOTE: TO GET ACCESS TO THE DRAWING STATE USE... state *s = (state*) data;
    //NOTE: DO NOT FORGET TO CALL show(d); AND TO RESET THE DRAWING STATE APART FROM
    //      THE 'START' FIELD AFTER CLOSING THE FILE
  if (data == NULL) return (pressedKey == 27);
  state *s = (state*) data;
  char *filename = getName(d);
  FILE *in = fopen(filename, "rb");

  byte ch = fgetc(in);
  for (int i=0; i<s->start; i++) {ch = fgetc(in);}

  while (!feof(in) && !s->end) {
    obey(d, s, ch);
    s->start++;
    ch = fgetc(in);
  }

  fclose(in); show(d);
  if (!s->end) s->start = 0;
  *s = (state) {0, 0, 0, 0, LINE, s->start, 0, false};
  return (pressedKey == 27);
}

// View a sketch file in a 200x200 pixel window given the filename
void view(char *filename) {
  display *d = newDisplay(filename, 200, 200);
  state *s = newState();
  run(d, s, processSketch);
  freeState(s);
  freeDisplay(d);
}

// Include a main function only if we are not testing (make sketch),
// otherwise use the main function of the test.c file (make test).
#ifndef TESTING
int main(int n, char *args[n]) {
  if (n != 2) { // return usage hint if not exactly one argument
    printf("Use ./sketch file\n");
    exit(1);
  } else view(args[1]); // otherwise view sketch file in argument
  return 0;
}
#endif