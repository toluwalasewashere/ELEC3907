
/* This sketch is designed to control 12 common cathode RG LEDs using 4 Arduino pins and 4 daisy-chained shift registers (74HC595)
 * Future adaptations may use 5 pins. The extra pin, being included to write to the shift register clear pin. 
 */

const int dataPin = 12; /*Arduino pin 12 will be connected to SR pin 14 (Serial input pin)*/
const int enablePin = 11; /*Arduino pin 11 will be connected to SR pin 13 (Output enable pin) */
const int latchClockPin = 10; /*Arduino pin 10 will be connected to SR pin 12 (Storage register/Latch clock input)*/
const int shiftClockPin = 9; /*Arduino pin 9 will be connected to SR pin 11 (Shift register clock input)*/

const int SHIFT_REG_NUM = 6; /*The total number of shift registers being daisy-chained*/
const int PIN_TOTAL = 8 * SHIFT_REG_NUM; /*Total number of output pins available*/
const int RGB_LED_NUM = 24; /*The total number of RGB LEDs*/
const int MAX_RAND_ACTIVE = 5; /*The maximum number of random red LEDs that can be active at a time*/

boolean pinState[PIN_TOTAL];/*Bitmap/Bit array that stores the ON/OFF state for all output bits sent to the shift registers*/

/*There are 12 LEDs with 2 anodes each; 24 anodes total(12 Red, 12 Green), each is driven by SR output pins.
 *The redPin and greenPin arrays map the 24 LED anodes to specific bit positions in the pinState array. 
*/

int redPin[] = {0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46,}; /*Bit indices for red anodes of LEDs*/
int greenPin[] = {1,3,5,7,9,11,13,15,17,19,21,23,25,27,29,31,33,35,37,39,41,43,45,47};/*Bit indices for green anodes of LEDs*/



void setup() {
  Serial.begin(9600);
  randomSeed(analogRead(A0));/*The seed for the random generator comes from a floating analog pin.*/ 

  pinMode(dataPin, OUTPUT);
  pinMode(enablePin, OUTPUT);
  pinMode(latchClockPin, OUTPUT);
  pinMode(shiftClockPin, OUTPUT);


  enableOutputs(true); 
  turnAllOff();/*Turn all LEDs off when setting up*/
  
  
    
}

void loop() {
  Serial.println("Starting Up...");
  generalSequence(0.05);
  delay(5000);
  turnAllOff();

  Serial.println("Starting Game Mode 1 Lighting Test...");
  float SF = 0.500;
  int active[MAX_RAND_ACTIVE];
  for (int numActive = 1; numActive < 7; numActive ++){
    Serial.print("--------- Active Random LEDs: ");
    Serial.print(numActive);
    Serial.println(" ---------");

    writeRandRed(numActive,active);
    delay(5000);
    greenSequence(active,SF,1);
    greenSequence(active,SF, -1);
    turnAllOff();
  }
}

/*The enableOutputs function turns all shift register outputs ON or OFF dependent on the boolean outputState 
 *If outputState is true, all shift register outputs are turned ON. Otherwise, they are turned OFF.
*/
void enableOutputs(boolean outputState){
  if (outputState){
    digitalWrite(enablePin,LOW); /*The 74HC595 has active low output enable. So by driving the enable pin LOW, we enable all outputs.*/
  } else {
    digitalWrite(enablePin, HIGH);
  }
}


/*The pinStateWrite function is supposed to allow control the state of each SR pin linked to an LED Anode.
 *Parameter value is either HIGH/'1' or LOW/'0'.
*/
void pinStateWrite(int index, int value){
  digitalWrite(latchClockPin,LOW);
  pinState[index] = value;
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

void turnAllOff(){
  for (int i = 0; i  < PIN_TOTAL; i++) {
    pinStateWrite(i,LOW);
  }
}

void turnGreenOff(){
  for(int i = 0; i < RGB_LED_NUM; i ++){
    writeGreen(greenPin[i],LOW);
  }
}

void turnRedOff(){
  for (int i = 0; i < RGB_LED_NUM; i++) {
    writeRed(redPin[i], LOW);
  }
}

/*The writeRed function is used to turn ON or OFF a given RED LED as long as the bit indice provided
 *is legitimate. Otherwise, it displays a message. 
*/
void writeRed(int pinNum, int value){
  bool validPinNum = inArray(redPin,RGB_LED_NUM,pinNum); /*Searches the redPin array to find the given bit indice*/
  if (validPinNum){
    pinStateWrite(pinNum, value);/*If the pin is found in the redPin array, drive value (HIGH/LOW) to it. */
  } else {
    Serial.println("Invalid bit indice."); /*Otherwise, display a message saying the bit indice is invalid.*/
  }
}


/*The writeGreen() function is used to turn ON or OFF a single green LED*/
void writeGreen(int pinNum, int value){
   bool validPinNum = inArray(greenPin,RGB_LED_NUM,pinNum); /*Searches the redPin array to find the given bit indice*/
  if (validPinNum){
    pinStateWrite(pinNum, value);/*If the pin is found in the redPin array, drive value (HIGH/LOW) to it. */
  } else {
    Serial.println("Invalid bit indice."); /*Otherwise, display a message saying the bit indice is invalid.*/
  }
}


/*The function writeRandRed generates an array of 5 random unique red LEDs but only turns on a given number of them*/
void writeRandRed(int numActive_SRPins, int * activeRed_SRPins){
  Serial.println("\n---- writeRandRed() NEW CALL ----");
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
void greenSequenceFWD(int * targetSR_PinList, float speedFactor){
  /*It may not be appropriate for the selector and target light to be the same initially*/
  Serial.println("Forward Green Sequence beginning...");
  
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
    if(inArray (targetSR_PinList, MAX_RAND_ACTIVE, redPin[i]) && pinState[redPin[i]]){ 
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

/*This function turns the green LEDs on in sequence going from highest bit indice to lowest bit indice.*/
void greenSequenceRVR(int * targetSR_PinList, float speedFactor){
Serial.println("Reverse green sequence beginning...");
  
  turnGreenOff();/*Turn all Green pins OFF before we start*/

  for (int i = RGB_LED_NUM-1 ; i >= 0 ; i--){
    int greenBit = greenPin[i];
    /*Turn off previous green LED before moving*/
    if (i<RGB_LED_NUM-1){
      writeGreen(greenPin[i+1],LOW);
    }

    /*Handling coincidence of red and green lights*/
    if(inArray (targetSR_PinList, MAX_RAND_ACTIVE, redPin[i]) && pinState[redPin[i]]){ 
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

/*Wrapper function for greenSequenceFWD() and greenSequenceRVR()*/
void greenSequence(int* targetSR_PinList,float speedFactor, int direction){
  if (direction == 1){
    greenSequenceFWD(targetSR_PinList, speedFactor);
  } else if (direction == -1){
    greenSequenceRVR(targetSR_PinList, speedFactor);
  } else {
    Serial.println("Invalid Direction. Direction must be 1 (FORWARD) or -1 (REVERSE/BACKWARDS)");
  }
}


/*The function inArray searches through an array to find an element. It returns a  otherwise.*/
bool inArray (int* arr, int size, int element){
  for (int i = 0; i < size; i++){
    if (arr[i] == element){ 
      return true;
    }
  }
  return false;
}

void generalSequence(float speedFactor){
  for (int i = 0; i < PIN_TOTAL; i++){
    pinStateWrite(i,HIGH);
    Serial.print("Pin ");
    Serial.print(i);
    Serial.print(" - STATE: ");
    Serial.println(pinState[i]);

    if(i > 0){
      pinStateWrite(i-1, LOW);
    }
    delay(1000*speedFactor);
  }
}

/*--------------------------------SHIFT REGISTER OPERATION EXPLAINED--------------------------------*/
/* The 74HC595 operates using two internal registers.
 * 1. Shift Register - Temporarily holds data as it's being received.
 * 2. Storage Register (Latch) - Holds the final data and controls the output pins (Q_A to Q_H)
 *
 * Data is transmitted into the shift register serially (bit-by-bit) through the serial input pin.
 * At each rising edge (transition from LOW to HIGH) in the shift register clock signal, all bits
 * in the shift register shift one position left (from LSB to MSB) and the current LSB is updated 
 * with the current logic level at the serial input pin. This process continues until all 8 bits are
 * loaded into the shift register. Once all 8 bits are loaded, a pulse is sent to the latch clock pin
 * and all data is copied from the shift register to the storage register. 
 * The outputs Q_A to Q_H are updated based on the values in the storage register.
*/

/*--------------------------------DAISY-CHAINING EXPLAINED--------------------------------*/
/* A daisy chain is a wiring scheme where multiple devices are wired together in sequence or in a ring
 * in a manner such that the output of one connects to the input of the next. 
 * This project required control of many
 * The Q_H' output pin of the 74HC595 allows the IC to be daisy-chained. The pin is serial output. 
 * It outputs the last bit (Q_H) of the shift register. By wiring Q_H' from one chip to the serial input
 * of the next multiple shift registers can be chained together. Each additional shift register adds an extra
 * 8 outputs, all of them controlled using the same 4 Arduino pins - dataPin, latchClockPin, enablePin, 
 * and shiftClockPin. 
*/
  
  


