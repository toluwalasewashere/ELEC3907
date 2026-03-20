
#include "LEDControls.h" //Incluing user-defined header file for LED control


/*-------------------------------------Definition of Constants-------------------------------------*/
const int dataPin = 12; /*Arduino pin 12 will be connected to SR pin 14 (Serial input pin)*/
const int enablePin = 11; /*Arduino pin 11 will be connected to SR pin 13 (Output enable pin) */
const int latchClockPin = 10; /*Arduino pin 10 will be connected to SR pin 12 (Storage register/Latch clock input)*/
const int shiftClockPin = 9; /*Arduino pin 9 will be connected to SR pin 12 (Shift register clock input)*/

const int SHIFT_REG_NUM = 4; /*The total number of shift registers being daisy-chained*/
const int PIN_TOTAL = 8 * SHIFT_REG_NUM; /*Total number of output pins available*/
const int RGB_LED_NUM = 12; /*The total number of RGB LEDs*/
const int MAX_RAND_ACTIVE = 5; /*The maximum number of random red LEDs that can be active at a time*/

/*Bitmap/Bit array that stores the ON/OFF state for all output bits sent to the shift registers*/
bool pinState[PIN_TOTAL];

/*There are 12 LEDs with 2 anodes each; 24 anodes total(12 Red, 12 Green), each is driven by SR output pins.
 *The redPin and greenPin arrays map the 24 LED anodes to specific bit positions in the pinState array. 
*/

int redPin[] = {5,8,10,12,14,17,19,21,24,26,28,30}; /*Bit indices (in pinState) corresponding to red anodes of LEDs*/
int greenPin[] = {6,9,11,13,16,18,20,22,25,27,29,31};/*Bit indices (in pinState) corresponding to green anodes of LEDs*/

/* */


void setupPins(){
  pinMode(dataPin, OUTPUT);
  pinMode(enablePin, OUTPUT);
  pinMode(latchClockPin, OUTPUT);
  pinMode(shiftClockPin, OUTPUT);
  Serial.println("\nPin setup complete.");
}

/*The generateRandSeed() function is used to generate the seed for the random number generator*/
void generateRandSeed(){
  randomSeed(analogRead(A0));/*The seed for the random generator comes from a floating analog pin.*/ 
}

/* The function inArray searches through an array of integers to find an element.
 * If the element is found, the function returns true
 * Otherwise, it returns false.*/
bool inArray (int* arr, int size, int element){
  for (int i = 0; i < size; i++){
    if (arr[i] == element){ 
      return true;
    }
  }
  return false;
}

/*The enableOutputs function turns all shift register outputs ON or OFF dependent on the boolean outputState 
 *If outputState is true, all shift register outputs are turned ON. Otherwise, they are turned OFF.
*/
void enableOutputs(bool choice){
  if (choice){
    digitalWrite(enablePin,LOW); /*The 74HC595 has active low output enable. So by driving the enable pin LOW, we enable all outputs.*/
  } else {
    digitalWrite(enablePin, HIGH);
  }
}

/*The pinStateWrite function is supposed to allow control the state of each SR pin linked to an LED Anode.
 *Parameter value is either HIGH/'1' or LOW/'0'.
*/
void pinStateWrite(int SR_pinIndex, int state){
  digitalWrite(latchClockPin,LOW);
  pinState[SR_pinIndex] = state;
  for (int i = 0; i < SHIFT_REG_NUM; i++){
    byte bitmap = 0;
    int base = i*8;
    for(int j = 0; j < 8; j++){
      if(pinState[base+j]) bitSet(bitmap, j);
    }
    shiftOut(dataPin, shiftClockPin, LSBFIRST, bitmap);
  }
  digitalWrite(latchClockPin,HIGH);
}

/*The turnAllOff() function turns all LEDs off*/
void turnAllOff(){
  for (int i = 0; i  < PIN_TOTAL; i++) {
    pinStateWrite(i,LOW);
  }
  Serial.println("All LEDs turned OFF");
}

/*The writeRed function is used to turn ON or OFF a given Red LED as long as the shift register pin indice provided
 *is legitimate. Otherwise, it displays a message. 
*/
void writeRed(int SR_pinIndex, int state){
  bool validPinNum = inArray(redPin,RGB_LED_NUM,SR_pinIndex); /*Searches the redPin array to find the given bit indice*/
  if (validPinNum){
    pinStateWrite(SR_pinIndex, state);/*If the pin is found in the redPin array, drive value (HIGH/LOW) to it. */
  } else {
    Serial.println("Invalid shift register pin indice."); /*Otherwise, display a message saying the bit indice is invalid.*/
  }
}

/*The writeGreen() function is used to turn ON or OFF a single green LED as long as the shift register pin indice
 *provided is legitimate. Otherwise, it displays a message. 
*/
void writeGreen(int SR_pinIndex, int state){
   bool validPinNum = inArray(greenPin,RGB_LED_NUM,SR_pinIndex); /*Searches the redPin array to find the given bit indice*/
  if (validPinNum){
    pinStateWrite(SR_pinIndex, state);/*If the pin is found in the redPin array, drive value (HIGH/LOW) to it. */
  } else {
    Serial.println("Invalid shift register pin indice."); /*Otherwise, display a message saying the bit indice is invalid.*/
  }
}

/*The turnGreenOff() function turns OFF all green LEDs*/
void turnGreenOff(){
  for(int i = 0; i < RGB_LED_NUM; i ++){
    writeGreen(greenPin[i],LOW);
  }
  Serial.println("All green LEDs turned OFF.");
}

/*The turnRedOff() function turns OFF all red LEDs*/
void turnRedOff(){
  for (int i = 0; i < RGB_LED_NUM; i++) {
    writeRed(redPin[i], LOW);
  }
}

/*The function writeRandRed generates an array of 5 random unique red LEDs but only turns on a given number of them*/
void writeRandRed(int numActive_SRPins, int* activeRed_SRPins){
  if (numActive_SRPins > MAX_RAND_ACTIVE){
    Serial.print("ILLEGAL VALUE. numActivePins must be less than ");
    Serial.println(MAX_RAND_ACTIVE);
    return;
  }

  
  /*Turn OFF all red LEDs*/
  turnRedOff();

  /*Store Indices for 5 random red LEDs in activeRedPins array. These red LEDs will be turned on later*/
  int newRandIndex, newPinNum;
  bool unique;
  for (int j = 0; j < MAX_RAND_ACTIVE; j++){
    do{
      newRandIndex = rand()%RGB_LED_NUM;
      newPinNum = redPin[newRandIndex];

      unique = true;
      for (int x = 0; x < j; x++){
        if (activeRed_SRPins[x] == newPinNum) unique = false;
      }
    } while (!unique);
    activeRed_SRPins[j] = newPinNum;
  }
  
  /*Once there are three unique red LEDs stored in the array, turn each of them ON*/
  Serial.println("Turned on random red LEDs at SR Pin indexes:");
  for (int k = 0; k < numActive_SRPins; k++){
    writeRed(activeRed_SRPins[k], HIGH);
    Serial.println(activeRed_SRPins[k]);
  }
}

/*This method is used to turn on the green LEDs in sequence going from lowest bit indice to highest.
 *If any LED in the sequence is red, it is momentarily turned green then back to red as the sequence continues.
 *Parameter targetIndex is the index of the target light in the redPin[] array
 *Parameter speedFactor is used to control how long the lights are ON
*/
void greenSequenceFWD(int* targetSR_pinList, float speedFactor){
  /*It may not be appropriate for the selector and target light to be the same initially*/
  Serial.println("Beginning Forward green sequence ...");
  
  turnGreenOff();/*Turn all Green pins OFF before we start*/

  for (int i = 0 ; i < RGB_LED_NUM ; i ++){
    int greenBit = greenPin[i];
    /*Turn off previous green LED before moving*/
    if (i>0){
      writeGreen(greenPin[i-1],LOW);
    }

    /*If a red pin is ON: 
     *Once we reach it turn it off, turn on the green anode of the same LED, 
     *delay, then turn off the green anode and turn the red anode back on. 
     */

    /*Handling coincidence of red and green lights*/
    if(inArray (targetSR_pinList, MAX_RAND_ACTIVE, redPin[i]) && pinState[redPin[i]]){ 
      writeRed(redPin[i],LOW);
      writeGreen(greenBit, HIGH);
      delay(1000*speedFactor);
      writeGreen(greenBit,LOW);
      writeRed(redPin[i],HIGH);
    } else {
      /*Turn on current green LED*/
      writeGreen(greenBit, HIGH);
      delay(1000*speedFactor);
    }
  }
  writeGreen(RGB_LED_NUM-1, LOW);
}

void greenSequenceRVR(int * target_SRpinList, float speedFactor){
Serial.println("Begin greenChaserRVR() ...");
  
  turnGreenOff();/*Turn all Green pins OFF before we start*/

  for (int i = RGB_LED_NUM-1 ; i >= 0 ; i--){
    int greenBit = greenPin[i];
    /*Turn off previous green LED before moving*/
    if (i<RGB_LED_NUM-1){
      writeGreen(greenPin[i+1],LOW);
    }

    /*Handling coincidence of red and green lights*/
    if(inArray (target_SRpinList, MAX_RAND_ACTIVE, redPin[i]) && pinState[redPin[i]]){ 
      writeRed(redPin[i],LOW);
      writeGreen(greenBit, HIGH);
      delay(1000*speedFactor);
      writeGreen(greenBit,LOW);
      writeRed(redPin[i],HIGH);
    } else {
      /*Turn on current green LED*/
      writeGreen(greenBit, HIGH);
      delay(1000*speedFactor);
    }
  }
  writeGreen(RGB_LED_NUM-1, LOW);
}

/*greenSequence() is a wrapper function that calls greenSequenceFWD() or greenSequenceRVR() based on the direction parameter*/
void greenSequence(int* targetSR_pinList, float speedFactor, int direction){
  if (direction != 1 && direction != -1){
    Serial.println("Invalid direction. Direction must be FWD(1) or RVR(-1).");
    return; 
  } else if (direction == 1){
    greenSequenceFWD(targetSR_pinList, speedFactor);
  } else if (direction == -1){
    greenSequenceRVR(targetSR_pinList, speedFactor);
  }
}
