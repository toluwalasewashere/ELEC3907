/* This sketch is designed to control 12 common cathode RG LEDs using 4 Arduino pins and 4 daisy-chained shift registers (74HC595)
 * Future adaptations may use 5 pins. The extra pin, being included to write to the shift register clear pin. 
 */

const int dataPin = 12; /*Arduino pin 12 will be connected to SR pin 14 (Serial input pin)*/
const int enablePin = 11; /*Arduino pin 11 will be connected to SR pin 13 (Output enable pin) */
const int latchClockPin = 10; /*Arduino pin 10 will be connected to SR pin 12 (Storage register clock input) - IDK why its called latchClockPin. I am just following naming conventions - */
const int shiftClockPin = 9; /*Arduino pin 9 will be connected to SR pin 12 (Shift register clock input)*/

const int SHIFT_REG_NUM = 4; /*The total number of shift registers being daisy-chained*/
const int PIN_TOTAL = 8 * SHIFT_REG_NUM; /*Total number of output pins available*/
const int RGB_LED_NUM = 12; /*The total number of RGB LEDs*/

boolean pinState[PIN_TOTAL];

/*There are 24 total LED Anodes(12 Red, 12 Green) being driven by SR output pins.
 *For the sake of organization, each pin driving an LED anode has been given an "index".
 *The arrays below contain the indexes of SR output pins driving red and green anodes respectively. 
*/

int redPin[] = {1,3,5,7,9,11,13,15,17,19,21,23}; 
int greenPin[] = {0,2,4,6,8,10,12,14,16,18,20,22};

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
  
  
}
