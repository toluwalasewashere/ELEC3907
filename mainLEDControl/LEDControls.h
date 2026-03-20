#ifndef LEDControls_H
#define LEDControls_H

#include "Arduino.h"


/**/
void setupPins();

void generateRandSeed();

bool inArray(int* arr, int size, int element);

void enableOutputs(bool choice);

void pinStateWrite(int SR_pinIndex, int state);

void turnAllOff();

void writeRed(int SR_pinIndex, int state);

void writeGreen(int SR_pinIndex, int state);

void turnGreenOff();

void turnRedOff();

void writeRandRed(int numActive_SRPins, int* activeRed_SRPins);

void greenSequenceFWD(int* targetSR_pinList, float speedFactor);

void greenSequenceRVR(int* targetSR_pinList, float speedFactor);

void greenSequence(int* targetSR_pinList, float speedFactor, int direction);

/* */

#endif

