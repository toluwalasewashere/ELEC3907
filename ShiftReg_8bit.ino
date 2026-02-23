#define CLR 5
#define serialData_A 6
#define serialData_B 4
#define shiftCLK 7
#define latchCLK 8


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(CLR, OUTPUT);
  pinMode(serialData_A, OUTPUT);
  pinMode(serialData_B, OUTPUT);
  pinMode(shiftCLK, OUTPUT); //Digital Pin connected to SRCLK on the 74HC595 shift register
  pinMode(latchCLK, OUTPUT); //Digital Pin connected to RCLK on the 74HC595 shift register

  digitalWrite(CLR, LOW);
  digitalWrite(CLR, HIGH);
}

void loop() {
  // put your main code here, to run repeatedly:
  for (int i = 0; i < 256; i++){
    digitalWrite(latchCLK, LOW);//Pulse RCLK to update outputs
    shiftOut(serialData_A, shiftCLK, MSBFIRST, i);
    delay(5000);
    digitalWrite(latchCLK, HIGH);
    Serial.println("Pulsed RCLK - OUTPUTS UPDATED.");
    Serial.println("Shift Register A - Parallel OUTPUT: ");
    Serial.println(i, BIN);
    
    delay(2500);
  }
  digitalWrite(latchCLK, LOW);
  shiftOut(serialData_A,shiftCLK, MSBFIRST, 0);
  digitalWrite(latchCLK,HIGH);
  digitalWrite(latchCLK,LOW);
  
  Serial.println("Shift Register B Start");
  for (int i = 0; i < 256; i++){
    digitalWrite(latchCLK, LOW);
    shiftOut(serialData_B, shiftCLK, MSBFIRST, i);
    delay(1000);
    digitalWrite(latchCLK, HIGH);//Pulse RCLK to update outputs
    Serial.println("Pulsed RCLK - OUTPUTS UPDATED.");
    Serial.println("Shift Register B - Parallel OUTPUT: ");
    Serial.print(i, BIN);
  }
}
