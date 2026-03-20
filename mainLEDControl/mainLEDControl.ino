
/* This sketch is designed to control 12 common cathode RG LEDs using 4 Arduino pins and 4 daisy-chained shift registers (74HC595)
 * Future adaptations may use 5 pins. The extra pin, being included to write to the shift register clear pin. 
 */
 #include "LEDControls.h"







void setup() {
  Serial.begin(9600);
  
  


  enableOutputs(true); 
  turnAllOff();/*Turn all LEDs off when setting up*/
  
  
    
}

void loop() {
  Serial.println("Starting Up...");
  delay(5000);
  float SF = 0.03;
  int active[5];
  for (int numActive = 1; numActive < 7; numActive ++){
    Serial.print("--------- Active Random LEDs: ");
    Serial.print(numActive);
    Serial.println(" ---------");

    writeRandRed(numActive,active);
    delay(5000);
    greenSequenceFWD(active,SF);
    greenSequenceRVR(active,SF);
    turnAllOff();
  }
}





















/*greenChaser() but lights travel in the oppposite direction*/




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
  
  


