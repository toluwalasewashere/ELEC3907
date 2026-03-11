/* This sketch is designed to control 12 common cathode RG LEDs using 4 Arduino pins and 4 daisy-chained shift registers (74HC595)
 * Future adaptations may use 5 pins. The extra pin, being included to write to the shift register clear pin. 
 */

const int dataPin = 12; /*Arduino pin 12 will be connected to SR pin 14 (Serial input pin)*/
const int enablePin = 11; /*Arduino pin 11 will be connected to SR pin 13 (Output enable pin) */
const int latchClockPin = 10; /*Arduino pin 10 will be connected to SR pin 12 (Storage register/Latch clock input)*/
const int shiftClockPin = 9; /*Arduino pin 9 will be connected to SR pin 12 (Shift register clock input)*/

const int SHIFT_REG_NUM = 4; /*The total number of shift registers being daisy-chained*/
const int PIN_TOTAL = 8 * SHIFT_REG_NUM; /*Total number of output pins available*/
const int RGB_LED_NUM = 12; /*The total number of RGB LEDs*/

boolean pinState[PIN_TOTAL];/*Bitmap/Bit array that stores the ON/OFF state for all output bits sent to the shift registers*/

/*There are 12 LEDs with 2 anodes each; 24 anodes total(12 Red, 12 Green), each is driven by SR output pins.
 *The redPin and greenPin arrays map the 24 LED anodes to specific bit positions in the pinState array. 
*/

int redPin[] = {1,3,5,7,9,11,13,15,17,19,21,23}; /*Bit indices for red anodes of LEDs*/
int greenPin[] = {0,2,4,6,8,10,12,14,16,18,20,22};/*Bit indices for green anodes of LEDs*/



void setup() {
  Serial.begin(9600);

  pinMode(dataPin, OUTPUT);
  pinMode(enablePin, OUTPUT);
  pinMode(latchClockPin, OUTPUT);
  pinMode(shiftClockPin, OUTPUT);


  enableOutputs(true); 
  /*Turn all LEDs off when setting up*/
  for (int i = 0; i  < PIN_TOTAL; i++) {
    pinState[i] = LOW; /*Apparently, LOW is defined as 0 in the Arduino header files. Thus, it is compatible with the boolean array.*/
    
  }
  
    
}

void loop() {
    
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

/*The pinStateWrite function is supposed to allow control the state of each SR pin linked to an LED Anode.*/
void pinStateWrite(int index, int value){
  digitalWrite(latchClockPin,LOW);
  pinState[index] = value;
  for (int i = 0; i < SHIFT_REG_NUM; i++){
    byte bitmap = 0;
    int base = i*8;
    for(int j = 0; j < 8; j++){
      if(pinState[base+j]) bitSet(bitmap, j);
    }
    shiftOut(dataPin, clockPin, LSBFirst, bitmap);
  }
  digitalWrite(latchClockPin,HIGH);
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
  
  


