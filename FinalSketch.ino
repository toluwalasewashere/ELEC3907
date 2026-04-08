/*This represents the latest sketch containing all control signals for React Ring Timing Game*/
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <stdarg.h>

const int dataPin = 23; /*ESP pin 23 will be connected to SR pin 14 (Serial input pin)*/
const int enablePin = 4; /*ESP pin 4 will be connected to SR pin 13 (Output enable pin) */
const int latchClockPin = 5; /*ESP pin 5 will be connected to SR pin 12 (Storage register/Latch clock input)*/
const int shiftClockPin = 18; /*ESP pin 18 will be connected to SR pin 12 (Shift register clock input)*/

const int SHIFT_REG_NUM = 6; /*The total number of shift registers being daisy-chained*/
const int PIN_TOTAL = 8 * SHIFT_REG_NUM; /*Total number of output pins available*/
const int RGB_LED_NUM = 24; /*The total number of RGB LEDs*/
const int MAX_RAND_ACTIVE = 6; /*The maximum number of random red LEDs that can be active at a time*/

//const int SDA = 22; //Data line for LCD I2C communication
//const int SCK = 21; //Clock line for LCD I2C communication
const int freq = 100000; //Frequency at which I2C communication takes place

boolean pinState[PIN_TOTAL];/*Bitmap/Bit array that stores the ON/OFF state for all output bits sent to the shift registers*/

/*There are 12 LEDs with 2 anodes each; 24 anodes total(12 Red, 12 Green), each is driven by SR output pins.
 *The redPin and greenPin arrays map the 24 LED anodes to specific bit positions in the pinState array. 
*/

int greenPin[] = {1,3,5,7,9,11,13,15,17,19,21,23,25,27,29,31,33,35,37,39,41,43,45,47}; /*Bit indices for red anodes of LEDs*/
int redPin[] = {0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46};/*Bit indices for green anodes of LEDs*/

LiquidCrystal_I2C lcd(0x27, 16, 2); //LCD screen having 2 rows of 16 characters at an address in I2C

int screenIndex = 0;
unsigned long lastScreenChange = 0;
const unsigned long screenInterval = 3000;

/*---------------------Wireless Communication--------------------*/

typedef struct __attribute__((packed)) {
  bool accel;
  bool btn1;
  bool btn2;
} Packet;

uint8_t player1Mac[] = {0x88, 0x57, 0x21, 0x70, 0xC1, 0x18};
uint8_t player2Mac[] = {0x88, 0x57, 0x21, 0xDD, 0xEE, 0xFF};

Packet pkt;
Packet pktP1;
Packet pktP2;
         
// Fixed for ESP32 core 3.x
void onReceive(const esp_now_recv_info *info, const uint8_t *data, int len) {
  if (len != sizeof(Packet)) {
    Serial.println("Bad packet size — ignored");
    return;
  }
  memcpy(&pkt, data, sizeof(Packet));

  if (pkt.accel) Serial.println(">> JERK received!");
  if (pkt.btn1)  Serial.println(">> BTN1 pressed!");
  if (pkt.btn2)  Serial.println(">> BTN2 pressed!");
}

/*----------------------Game/Menu Setup----------------------------*/
enum State {
  WAITING,
  TIMING_MENU,
  MEMORY_MENU,
  PONG_MENU,
  TIMING, 
  MEMORY, 
  PONG
};

State currentState = WAITING;

enum WaitingPhase {
  SET_RED,
  WAIT_DELAY,
  CHASER_FWD,
  CHASER_REV,
  CLEAR_ALL
};

WaitingPhase waitingPhase = SET_RED;

int numActive = 1;
int active[MAX_RAND_ACTIVE];
float SF;  
int timingScore;
int level;
bool timeWin;
bool memoryWin;
bool pongWin1;
bool pongWin2;
int maxRally;

unsigned long phaseStartTime = 0;

/*---------------------------------------------------------------*/

void setup() { 

  Serial.begin(115200); 

/*------------------------LCD I2C Setup---------------------------*/
  Wire.begin(21, 22, freq);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(2, 0);
  lcd.print("Initialized");

  char buffer[17];

/*---------------------------Game Setup--------------------------*/
  randomSeed(analogRead(A0));/*The seed for the random generator comes from a floating analog pin.*/ 
  pinMode(dataPin, OUTPUT); 
  pinMode(enablePin, OUTPUT); 
  pinMode(latchClockPin, OUTPUT); 
  pinMode(shiftClockPin, OUTPUT); 
  enableOutputs(true); 
  turnAllOff();/*Turn all LEDs off when setting up*/ 

/*----------------Wireless Communication Setup--------------------*/
   WiFi.mode(WIFI_STA);
  WiFi.disconnect(true, true);
  delay(200);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  Serial.print("Receiver MAC: ");
  Serial.println(WiFi.macAddress());
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    while (true) delay(1000);
  }
  esp_now_register_recv_cb(onReceive);
  Serial.println("Receiver ready, listening...");
}

void loop() {
  //reset btn and accel, otherwise stays true. 
  bool b1 = pkt.btn1;
  bool b2 = pkt.btn2;
  pkt.btn1 = false;
  pkt.btn2 = false;
  pkt.accel = false;
  handleInput(b1, b2);
  // Only run waiting/menu animation when not inside a game
  if (currentState == WAITING || currentState == TIMING_MENU || currentState == PONG_MENU || currentState == MEMORY_MENU) {
    waitingDisplayStep();
  }
}

void playPong(){
  
}

/*Memory game. An increasing number if red LEDs will light up around the ring. 
*These lights will then turn off after a few seconds. The player must navigate 
*about the ring and select which LEDs were on.
*/
void playMemory(){

  int i = 0;
  bool gameCon = true;
  bool anyAlive = false;
  int strike = 0;
  int overlapTarget = -1;
  int prevHiddenTarget = -1;
  level = 1;
  memoryWin = false;

  bool targetAlive[MAX_RAND_ACTIVE];

  numActive = 1;

  int targetsHit[MAX_RAND_ACTIVE];

  for (int k = 0; k < MAX_RAND_ACTIVE; k++) {
    targetAlive[k] = false;
    targetsHit[k] = -1;
  }

  turnAllOff();

  printLine(0, "       3        ");
  printLine(1, "                ");
  delay(1000);

  printLine(0, "     3   2      ");
  printLine(1, "                ");
  delay(1000);

  printLine(0, "   3   2   1    ");
  printLine(1, "                ");
  delay(1000);

  printLine(0, "       GO       ");
  printLine(1, "                ");
  delay(800);

  writeRandRed(numActive, active);

  delay(3000);

  turnAllOff();

  writeGreen(i, HIGH);

  for (int k = 0; k < MAX_RAND_ACTIVE; k++) {
    targetAlive[k] = (k < numActive);

  }

  printLine(0, "Level:%d", level);
  printLine(1, "Strikes:%d", strike);

  while(gameCon){

    overlapTarget = -1;

    int greenBit = greenPin[i];
    int redBit   = redPin[i];

    for (int k = 0; k < numActive; k++) {
      if (targetAlive[k] && active[k] == redPin[i]) {
        overlapTarget = k;
      break;
      }
    }

    if (overlapTarget != -1) {
      writeRed(redBit, LOW);      
      writeGreen(greenBit, HIGH);     
      prevHiddenTarget = overlapTarget;
    } else {
      writeGreen(greenBit, HIGH);
    } 

    if(pkt.btn2){
      pkt.btn2 = false;

      if(overlapTarget != -1){
        targetAlive[overlapTarget] = false;
        writeGreen(i, LOW);
        writeRed(active[overlapTarget], HIGH);
        targetsHit[overlapTarget] = active[overlapTarget];

        if (prevHiddenTarget == overlapTarget) {
          prevHiddenTarget = -1;
        }
        overlapTarget = -1;
      } else{
        strike++;
        printLine(1, "Strikes:%d", strike);

        turnGreenOff();
        writeAllRed();
        delay(1000);
        turnAllOff();

        //Turn all previousy hit targets back on
        for(int j = 0; j < MAX_RAND_ACTIVE; j++){
          if(targetsHit[j] != -1){
            writeRed(targetsHit[j], HIGH);
          }
        }
      }
    }

    if(pkt.btn1){
      pkt.btn1 = false;
      i++;
      if (i >= RGB_LED_NUM) {
        i = 0;
        writeGreen(greenPin[i], HIGH);
        writeGreen(greenPin[RGB_LED_NUM - 1], LOW);
      }
      else{
        writeGreen(greenPin[i], HIGH);
        writeGreen(greenPin[i - 1], LOW);
      }

      int prevIndex = (i == 0) ? (RGB_LED_NUM - 1) : (i - 1);

      for (int l = 0; l < MAX_RAND_ACTIVE; l++) {
        if (targetsHit[l] == redPin[prevIndex]) {
        writeRed(redPin[prevIndex], HIGH);
        break;
        }
      }
    }

    for (int k = 0; k < numActive; k++) {
      if (targetAlive[k]) {
        anyAlive = true;
        break;
      } else{
        anyAlive = false;
      }
    }
    if (!anyAlive) {
      numActive++;
      level++;
      for (int k = 0; k < MAX_RAND_ACTIVE; k++) {
        targetAlive[k] = false;
        targetsHit[k] = -1;
      }
      printLine(0, "Level:%d", level);
      if (numActive > MAX_RAND_ACTIVE) {
        gameCon = false;
        memoryWin = true;
        turnAllOff();
        break;
      }
      turnAllOff();
      writeRandRed(numActive, active);
      delay(3000);
      turnAllOff();
      i = 0;
      for (int k = 0; k < MAX_RAND_ACTIVE; k++) {
        targetAlive[k] = (k < numActive);
      }
    }
    if (strike >= 3) {
      gameCon = false;
      break;
    }
  } 
}

/*Timing game. Ring of 24 lights has a green circling light and a varying 
*number of red target lights. If the user presses button two while the circling 
*light is overlapping with a target light, then the user scores a point. If the 
*button is pressed and the circling light and target are not overlapping, the
*user receives a strike. Each time the user removes all target lights on the ring,
*an additional light is added as a target on the next rotation of the green light. 
*Each time an additional target is added, The game is over when the user has cleared 
*all 6 levels of difficulty or the user receives 3 strikes. 
*/
void playTiming() {
  int i = 0;
  unsigned long lastStepTime = 0;

  bool gameCon = true;
  int count = 0;
  int strike = 0;
  int cycle = 0;
  timeWin = false;
  

  // Tracks whether each currently-selected target is still alive
  bool targetAlive[MAX_RAND_ACTIVE];

  // Which target is currently under the green light, if any
  int overlapTarget = -1;

  // Which red target was temporarily hidden by the green light on the previous step
  int prevHiddenTarget = -1;

  SF = 0.40;
  numActive = 1;
  lastStepTime = millis();

  for (int k = 0; k < MAX_RAND_ACTIVE; k++) {
    targetAlive[k] = false;
  }

  turnAllOff();

  printLine(0, "       3        ");
  printLine(1, "                ");
  delay(1000);

  printLine(0, "     3   2      ");
  printLine(1, "                ");
  delay(1000);

  printLine(0, "   3   2   1    ");
  printLine(1, "                ");
  delay(1000);

  printLine(0, "       GO       ");
  printLine(1, "                ");
  delay(800);

  turnAllOff();
  writeRandRed(numActive, active);

  for (int k = 0; k < MAX_RAND_ACTIVE; k++) {
    targetAlive[k] = (k < numActive);
  }

  delay(1000);
  lastStepTime = millis();

  printLine(0, "Score:%d", count);
  printLine(1, "Strikes:%d", strike);

  while (gameCon) {

    // -------- button handling --------
    if (pkt.btn2) {
      pkt.btn2 = false;

      if (overlapTarget != -1) {
        // successful hit
        targetAlive[overlapTarget] = false;
        writeRed(active[overlapTarget], LOW);
        count++;
        printLine(0, "Score:%d", count);

        // prevent the same target from being restored next step
        if (prevHiddenTarget == overlapTarget) {
          prevHiddenTarget = -1;
        }
        overlapTarget = -1;
      } else {
        // miss
        strike++;

        printLine(1, "Strikes:%d", strike);

        turnGreenOff();
        writeAllRed();
        delay(1000);

        turnAllOff();

        // redraw only surviving targets
        for (int k = 0; k < numActive; k++) {
          if (targetAlive[k]) {
            writeRed(active[k], HIGH);
          }
        }

        lastStepTime = millis();   // freeze chaser during penalty
      }
    }

    if (strike >= 3) {
      gameCon = false;
      break;
    }

    // -------- timed movement --------
    if (millis() - lastStepTime >= (unsigned long)(1000.0 * SF)) {
      lastStepTime = millis();

      // Turn off previous green
      if (i > 0) {
        writeGreen(greenPin[i - 1], LOW);
      } else {
        writeGreen(greenPin[RGB_LED_NUM - 1], LOW);
      }

      // Restore previously hidden red if it still exists
      if (prevHiddenTarget != -1 && targetAlive[prevHiddenTarget]) {
        writeRed(active[prevHiddenTarget], HIGH);
      }
      prevHiddenTarget = -1;
      overlapTarget = -1;

      // Completed one full lap
      if (i >= RGB_LED_NUM) {
        i = 0;
        cycle++;

        bool anyAlive = false;
        for (int k = 0; k < numActive; k++) {
          if (targetAlive[k]) {
            anyAlive = true;
            break;
          }
        }

        // level up only after a full cycle and only if all current targets are cleared
        if (!anyAlive) {
          numActive++;

          if (numActive > MAX_RAND_ACTIVE) {
            gameCon = false;
            timeWin = true;
            turnAllOff();
            break;
          }

          SF *= 0.75;
          turnAllOff();
          writeRandRed(numActive, active);

          for (int k = 0; k < MAX_RAND_ACTIVE; k++) {
            targetAlive[k] = (k < numActive);
          }

          delay(1000);
          lastStepTime = millis();
        }
      }

      int greenBit = greenPin[i];
      int redBit   = redPin[i];

      // Check whether this position contains a live target
      for (int k = 0; k < numActive; k++) {
        if (targetAlive[k] && active[k] == redBit) {
          overlapTarget = k;
          break;
        }
      }

      if (overlapTarget != -1) {
        writeRed(redBit, LOW);          // temporarily hide red
        writeGreen(greenBit, HIGH);     // show green over it
        prevHiddenTarget = overlapTarget;
      } else {
        writeGreen(greenBit, HIGH);
      }

      i++;
    }
  }

  turnAllOff();
  timingScore = count;
}

/*Display activity in WAITING and game menus*/
void waitingDisplayStep() {

  updateState();  

  switch (waitingPhase) {

    case SET_RED:
      writeRandRed(numActive, active);

      phaseStartTime = millis();
      waitingPhase = WAIT_DELAY;
      break;

    case WAIT_DELAY:
      if (millis() - phaseStartTime >= 3000) {
        waitingPhase = CHASER_FWD;
      }
      break;

    case CHASER_FWD:
      if (greenChaserStep(active, 0.5)) {   
        waitingPhase = CHASER_REV;
      }
      break;

    case CHASER_REV:
      if (greenChaserRVRStep(active, 0.5)) {
        waitingPhase = CLEAR_ALL;
      }
      break;

    case CLEAR_ALL:
      turnAllOff();

      numActive++;
      if (numActive > MAX_RAND_ACTIVE) numActive = 1;

      waitingPhase = SET_RED;
      break;
  }
}

/*Remove unwanted space on LCD and write to it*/
void printLine(int row, const char* format, ...) {
  char raw[17];
  char buffer[17];

  va_list args;
  va_start(args, format);
  vsnprintf(raw, sizeof(raw), format, args);
  va_end(args);

  snprintf(buffer, sizeof(buffer), "%-16s", raw);

  lcd.setCursor(0, row);
  lcd.print(buffer);
}

/*Transition conditions for menus*/
void handleInput(bool btn1, bool btn2) {
  State prevState = currentState;

  switch (currentState) {
    case WAITING:
      if (btn2) {
        currentState = TIMING_MENU;
      }
      break;

    case TIMING_MENU:
      if (btn1) {
        currentState = MEMORY_MENU;
      } 
      else if (btn2) {
        currentState = TIMING;
      }
      break;

    case MEMORY_MENU:
      if (btn1) {
        currentState = PONG_MENU;
      } 
      else if (btn2) {
        currentState = MEMORY;
      }
      break;

    case PONG_MENU:
      if (btn1) {
        currentState = TIMING_MENU;
      }
      // else if (btnValue == 2) currentState = PONG;
      break;

    case TIMING:
      break;

    case MEMORY:
      break;

    case PONG:
      break;
  }

  if (prevState != currentState) {
    screenIndex = 0;
    lastScreenChange = millis();
    turnAllOff();

    if (currentState == TIMING) {
      playTiming();

      if(timeWin){
        writeAllGreen();
        printLine(0, "    You Win!    ");
        printLine(1, " Max  Score: 21");
        delay(5000);
        timeWin = false;
        turnAllOff();
      } 
      else{
      writeAllRed();
      printLine(0, "   Game Over   ");
      printLine(1, "Score: %d", timingScore);
      delay(3000);
      turnAllOff();
      }

      currentState = WAITING;
      screenIndex = 0;
      lastScreenChange = millis();
    }

    if (currentState == MEMORY) {
      playMemory();

      if(memoryWin){
        writeAllGreen();
        printLine(0, "    You Win!    ");
        printLine(1, "Level 6 Complete");
        delay(5000);
        memoryWin = false;
        turnAllOff();
      } 
      else{
      writeAllRed();
      printLine(0, "   Game Over   ");
      printLine(1, "Level: %d", level);
      delay(3000);
      turnAllOff();
      }

      currentState = WAITING;
      screenIndex = 0;
      lastScreenChange = millis();
    }

    if (currentState == PONG) {
      playPong();

      if(pongWin1){
        writeAllGreen();
        printLine(0, " Player 1 Wins! ");
        printLine(1, "Max Rally: %d", maxRally);
        delay(5000);
        memoryWin = false;
        turnAllOff();
      } 
      else if(pongWin2){
        writeAllGreen();
        printLine(0, " Player 2 Wins! ");
        printLine(1, "Max Rally: %d", maxRally);
        delay(5000);
        memoryWin = false;
        turnAllOff();
      }
      currentState = WAITING;
      screenIndex = 0;
      lastScreenChange = millis();
    }
  }

  updateState();
}

/*State Transition enum Menu of updating menus*/
void updateState() {
  if (millis() - lastScreenChange > screenInterval) {
    screenIndex++;
    lastScreenChange = millis();
  }

  switch (currentState) {
    case WAITING:
      showWaitingScreens();
      break;

    case TIMING_MENU:
      showTimingMenu();
      break;

    case MEMORY_MENU:
      showMemoryMenu();
      break;

    case PONG_MENU:
      showPongMenu();
      break;

    case TIMING:
      playTiming();
      break;

    case MEMORY:
      playMemory();
      break;
    
    case PONG:
      playPong();
      break;

  }
}
/*Waiting screen menu*/
void showWaitingScreens() {
  switch (screenIndex % 3) {

    case 0:
      printLine(0, " Welcome to our");
      printLine(1, "ELEC3907 Project");
      break;

    case 1:
      printLine(0, "   React Ring");
      printLine(1, "  Timing  Game");
      break;

    case 2:
      printLine(0, " Press Button 2");
      printLine(1, "  to  Continue");
      break;
  }
}

/*Timing game menu select*/
void showTimingMenu() {
  switch (screenIndex % 2) {

    case 0:
      printLine(0, "Timing Game");
      printLine(1, "Btn1:Next");
      break;

    case 1:
      printLine(0, "Timing Game");
      printLine(1, "Btn2:Start");
      break;
  }
}

/*Pong game menu select*/
void showMemoryMenu() {
  switch (screenIndex % 2) {

    case 0:
      printLine(0, "Memory Game");
      printLine(1, "Btn1:Next");
      break;

    case 1:
      printLine(0, "Memory Game");
      printLine(1, "Btn2:Start");
      break;
  }
}

/*Pong game menu select*/
void showPongMenu() {
  switch (screenIndex % 2) {

    case 0:
      printLine(0, "Pong Game");
      printLine(1, "Btn1:Next");
      break;

    case 1:
      printLine(0, "Pong Game");
      printLine(1, "Btn2:Start");
      break;
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

/*Function which turns all lights Red*/
void writeAllRed(){
  for(int j = 0; j < RGB_LED_NUM; j++){
    writeRed(2*j, HIGH);
  }
}

/*Function which turns all lights Green*/
void writeAllGreen(){
  for(int j = 0; j < RGB_LED_NUM; j++){
    writeGreen(2*j+1, HIGH);
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

/*Checks if any pins in the passed array are currently on*/
bool isOn(int * activeRedPins){
  for(int i = 0; i < MAX_RAND_ACTIVE; i++){
    if(pinState[activeRedPins[i]]){
      return true;
    }
  }
  return false;
}


/*The function writeRandRed generates an array of 5 random unique red LEDs but only turns on a given number of them*/
void writeRandRed(int numActivePins, int * activeRedPins){
  Serial.println("\n---- writeRandRed() NEW CALL ----");
  if (numActivePins > MAX_RAND_ACTIVE){
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
        if (activeRedPins[x] == newPinNum) unique = false;
      }
    } while (!unique);
    activeRedPins[j] = newPinNum;
  }
  
  /*Once there are three unique red LEDs stored in the array, turn each of them ON*/
  for (int k = 0; k < numActivePins; k++){
    writeRed(activeRedPins[k], HIGH);
    Serial.println(activeRedPins[k]);
  }
}

/*This method is used to turn on the green LEDs in sequence going from lowest bit indice to highest.
 *If any LED in the sequence is red, it is momentarily turned green then back to red as the sequence continues.
 *Parameter targetIndex is the index of the target light in the redPin[] array
 *Parameter speedFactor is used to control how long the lights are ON
*/
bool greenChaserStep(int *targetIndexList, float speedFactor) {

  static int i = 0;
  static unsigned long lastStepTime = 0;

  if (millis() - lastStepTime >= 1000 * speedFactor) {
    lastStepTime = millis();

    static int prevRed = -1;

    // turn off previous green
    if (i > 0) {
      writeGreen(greenPin[i - 1], LOW);
    }

    // restore previous red
    if (prevRed != -1) {
      writeRed(prevRed, HIGH);
      prevRed = -1;
    }

    int greenBit = greenPin[i];

    // handle current LED
    if (inArray(targetIndexList, MAX_RAND_ACTIVE, redPin[i]) && pinState[redPin[i]]) {
      prevRed = redPin[i];
      writeRed(redPin[i], LOW);
      writeGreen(greenBit, HIGH);
    } else {
      writeGreen(greenBit, HIGH);
    }

    i++;

    if (i >= RGB_LED_NUM) {
      writeGreen(greenPin[RGB_LED_NUM - 1], LOW);
      i = 0;
      return true;  // DONE
    }
  }

  return false;  // still running
}

/*greenChaser() but lights travel in the oppposite direction*/
bool greenChaserRVRStep(int *targetIndexList, float speedFactor) {

  static int i = RGB_LED_NUM - 1;
  static unsigned long lastStepTime = 0;

  if (millis() - lastStepTime >= 1000 * speedFactor) {
    lastStepTime = millis();

    static int prevRed = -1;

    if (i < RGB_LED_NUM - 1) {
      writeGreen(greenPin[i + 1], LOW);
    }

    if (prevRed != -1) {
      writeRed(prevRed, HIGH);
      prevRed = -1;
    }

    int greenBit = greenPin[i];

    if (inArray(targetIndexList, MAX_RAND_ACTIVE, redPin[i]) && pinState[redPin[i]]) {
      prevRed = redPin[i];
      writeRed(redPin[i], LOW);
      writeGreen(greenBit, HIGH);
    } else {
      writeGreen(greenBit, HIGH);
    }

    i--;

    if (i < 0) {
      writeGreen(greenPin[0], LOW);
      i = RGB_LED_NUM - 1;
      return true;  // DONE
    }
  }

  return false;
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