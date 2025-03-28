//import necessary libraries
#include <Servo.h>
#include <Wire.h>
#include <SparkFunMLX90614.h>
#include <SoftwareSerial.h>

//define QTI and LED pins
#define leftQTI 49
#define middleQTI 51
#define rightQTI 53

#define redpin 45
#define greenpin 46
#define bluepin 44

//LCD pin
#define TxPin 14

//constants for playing the song
#define num_sw 19
#define num_im 18

//declare servos
Servo servoLeft;
Servo servoRight;

//declare sensor
IRTherm therm;

//declare and instantiate LCD object
SoftwareSerial LCD = SoftwareSerial(255, TxPin);

//variables used to track hash count and score
int hashCount;
int score;

//variables used for recieving data and determining celebration
int finalReads;
int vals[5] = { -1, -1, -1, -1, -1 };

boolean finished = false;

//arrays used to store notes for songs
int durs_sw[num_sw] = {210, 210, 210, 213, 213, 210, 210, 210, 213, 212, 210, 210, 210, 213, 212, 210, 210, 210, 213};
int octs_sw[num_sw] = {216, 216, 216, 216, 216, 216, 216, 216, 217, 216, 216, 216, 216, 217, 216, 216, 216, 216, 216};
int note_sw[num_sw] = {221, 221, 221, 221, 228, 226, 225, 223, 221, 228, 226, 225, 223, 221, 228, 226, 225, 226, 223};

int durs_im[num_im] = {212, 212, 212, 211, 211, 212, 211, 211, 213, 212, 212, 212, 211, 211, 212, 211, 211, 213};
int octs_im[num_im] = {215, 215, 215, 215, 216, 215, 215, 216, 215, 216, 216, 216, 216, 216, 215, 215, 216, 215};
int note_im[num_im] = {230, 230, 230, 226, 221, 230, 226, 221, 230, 225, 225, 225, 226, 221, 230, 226, 221, 230};

void setup() {
  //attach servos and set up LEDs
  servoLeft.attach(11);
  servoRight.attach(12);

  //set onboard and external LED pins to output
  pinMode(redpin, OUTPUT);
  pinMode(greenpin, OUTPUT);
  pinMode(bluepin, OUTPUT);
  setLED(0, 0, 0);

  pinMode(A0, OUTPUT);
  pinMode(A1, OUTPUT);
  pinMode(A2, OUTPUT);
  setExternal(0, 0, 0);

  //set up the thermal sensor
  Wire.begin();
  therm.begin();
  therm.setUnit(TEMP_F);

  //define variables to keep track of score and hash counts
  hashCount = 0;
  score = 0;

  //start serial monitor and XBee
  Serial.begin(9600);
  Serial2.begin(9600);
  LCD.begin(9600);

  //update LCD screen
  rerender();
}

void loop() {
  //checks if we've hit second hash and runs sensing code
  if(finished){
    updateVals();
    return;
  }

  //get QTI readings
  int state = getState();

  //make decision based on state reading from QTIs
  switch (state) {
    case 0:
      stop();
      break;
    case 1:
      turnRight();
      break;
    case 2:
      forward();
      break;
    case 3:
      turnRight();
      break;
    case 4:
      turnLeft();
      break;
    case 5:
      //not possible
      stop();
      break;
    case 6:
      turnLeft();
      break;
    case 7:
      //hash case handled by seperate function
      handleHash();
      break;
  }
}

void handleHash() {
  //make sure the robot pauses and increment hashCount to track the new hash
  stop();
  hashCount++;

  //if we're only at the hashCount, sense the object and move forward
  if (hashCount == 1) {
    setLED(255, 0, 0);
    senseObject();

    //move forward until no longer on hash
    forward();
    while (getState() == 7)
      delay(1);
  } else if (hashCount == 2) {
    //if second hashCount, mark that we are done moving, sense again
    finished = true;
    setLED(0, 255, 0);
    senseObject();

    //update our array with score and send the information to other boths
    vals[3] = score;
    Serial2.print(char(90 + score));

    setExternal(0, 0, 255);
    delay(250);
    setExternal(0, 0, 0);

    //update LED screen
    rerender();
  }

  delay(500);
  setLED(0, 0, 0);
}

void senseObject() {
  //update score if the object is found to be cold and update LED accordingly
  if (therm.read() && therm.object() < 63.5) {
    score = hashCount;
    setExternal(0, 255, 0);
  } else {
    setExternal(255, 0, 0);
  }
  delay(250);
  setExternal(0, 0, 0);
}

//Defines funtion 'rcTime' to read value from QTI sensor
// From Ch. 6 Activity 2 of Robotics with the BOE Shield for Arduino
boolean qtiState(int pin) {
  pinMode(pin, OUTPUT);     // Sets pin as OUTPUT
  digitalWrite(pin, HIGH);  // Pin HIGH
  delay(1);                 // Waits for 1 millisecond
  pinMode(pin, INPUT);      // Sets pin as INPUT
  digitalWrite(pin, LOW);   // Pin LOW
  long time = micros();     // Tracks starting time
  while (digitalRead(pin));                      // Loops while voltage is high
  time = micros() - time;  // Calculate decay time
  return time > 150;       // Return decay time
}

//gets current state based on QTI readings
int getState() {
  boolean qti1 = qtiState(leftQTI);
  boolean qti2 = qtiState(middleQTI);
  boolean qti3 = qtiState(rightQTI);

  return 4 * qti1 + 2 * qti2 + qti3;
}

void updateVals(){
  //first read and update any new vals
  readVals();

  //increment number of reads to keep track of how long we've been sensing
  finalReads++;
  delay(100);

  //determine which celebration, if any, to do
  boolean complete = true;
  boolean same = true;
  int sum = 0;
  for (int i=0;i<5;i++){
    //if any value is -1, the array is not complete
    if (vals[i] == -1)
      complete = false;
    //if any value is different from the first, they are not all the same
    if(vals[i]!=vals[0])
      same = false;
    //keep track of the total, assume 1 if no data
    sum += vals[i] == -1 ? 1 : vals[i];
  }

  if (complete || finalReads > 100) {
    //based on variables we've tracked, determine celebration and send it out
    if(same){
      Serial2.print(char(21));
      delay(2000);
      sing();
    }else if(sum<5){
      Serial2.print(char(22));
      delay(2000);
      lightShow();
    }else{
      Serial2.print(char(23));
      delay(2000);
      dance();
    }

    //detach servos and enter infinite loop to end program
    servoLeft.detach();
    servoRight.detach();
    while(true)
      delay(1);
  }
}

void readVals() {
  //keeps track of whether any values change
  boolean changed = false;

  //read from buffer till empty
  while (Serial2.available()) {
    char incoming = Serial2.read();

    setExternal(0, 255, 255);
    delay(200);
    setExternal(0, 0, 0);
    delay(200);

    //process any read characters to update array
    changed = changed | vals[incoming / 10 - 6] != incoming % 10;
    vals[incoming / 10 - 6] = incoming % 10;
  }

  //only rerender screen if any changes were noted
  if (changed)
    rerender();
}

void dance(){
  servoLeft.writeMicroseconds(1500);
  servoRight.writeMicroseconds(1500);

  // forward-forward
  servoLeft.writeMicroseconds(1700);
  servoRight.writeMicroseconds(1300);
  delay(500);
  servoLeft.writeMicroseconds(1500);
  servoRight.writeMicroseconds(1500);
  delay(1000);

  servoLeft.writeMicroseconds(1700);
  servoRight.writeMicroseconds(1300);
  delay(500);
  servoLeft.writeMicroseconds(1500);
  servoRight.writeMicroseconds(1500);
  delay(1000);

  // back
  servoLeft.writeMicroseconds(1300);
  servoRight.writeMicroseconds(1700);
  delay(500);
  servoLeft.writeMicroseconds(1500);
  servoRight.writeMicroseconds(1500);
  delay(1000);

  // back
  servoLeft.writeMicroseconds(1300);
  servoRight.writeMicroseconds(1700);
  delay(500);
  servoLeft.writeMicroseconds(1500);
  servoRight.writeMicroseconds(1500);
  delay(1000);

  // left left
  servoLeft.writeMicroseconds(1300);
  servoRight.writeMicroseconds(-1700);
  delay(100);
  servoLeft.writeMicroseconds(1500);
  servoRight.writeMicroseconds(1500);
  delay(200);

  servoLeft.writeMicroseconds(1300);
  servoRight.writeMicroseconds(-1700);
  delay(100);
  servoLeft.writeMicroseconds(1500);
  servoRight.writeMicroseconds(1500);
  delay(200);

  // // right right
  servoLeft.writeMicroseconds(1500);
  servoRight.writeMicroseconds(1700);
  delay(100);
  servoLeft.writeMicroseconds(1500);
  servoRight.writeMicroseconds(1500);
  delay(200);

  servoLeft.writeMicroseconds(1500);
  servoRight.writeMicroseconds(1700);
  delay(100);
  servoLeft.writeMicroseconds(1500);
  servoRight.writeMicroseconds(1500);
  delay(200);
}

void sing(){
  //first song, star wars main theme
  for(long k = 0; k < num_sw; k++){
    LCD.write(durs_sw[k]);
    LCD.write(octs_sw[k]);
    LCD.write(note_sw[k]);
    int len = 214 - durs_sw[k];
    float del = 2000 / pow(2, len);
    delay(int(del*1.1));
  }

  //second song, imperial march
  for(long k = 0; k < num_im; k++){
    LCD.write(durs_im[k]);
    LCD.write(octs_im[k]);
    LCD.write(note_im[k]);
    int len = 214 - durs_im[k];
    float del = 2000 / pow(2, len);
    delay(int(del*1.1));
  }
}

void lightShow(){
  int counter = 3;
  //exteral led flickers white and then blue and on-board does the opposite 
  for (int k = 1; k<8; k++){
    setExternal(255,255,255);
    setLED(0,0,255);
    delay(5000/counter); 

    setExternal(0,0,255);
    setExternal(255,255,255);
    delay(5000/counter);

    counter = 2*counter; 
  }

  for (int j = 1; j<9; j++){
    //turn external led purple + board led yellow
    setExternal(255,0,255);
    setLED(255,255,0);
    delay(125);

    //turn both leds off 
    setExternal(0,0,0);
    setLED(0,0,0);
    delay(125);
  }
}

//directional functions
void turnRight() {
  servoLeft.writeMicroseconds(1700);
  servoRight.writeMicroseconds(1510);
}

void turnLeft() {
  servoLeft.writeMicroseconds(1600);
  servoRight.writeMicroseconds(1370);
}

void forward() {
  servoLeft.writeMicroseconds(1660);
  servoRight.writeMicroseconds(1450);
}

void stop() {
  servoLeft.writeMicroseconds(1500);
  servoRight.writeMicroseconds(1500);
}

//update LED screen based on read values
void rerender() {
  LCD.write(12);  // clear
  delay(10);
  LCD.write(22);  // no cursor no blink
  delay(10);

  LCD.print("|");
  for (int n : vals) {
    delay(10);
    if (n > -1)
      LCD.print(n);
    else
      LCD.print(" ");
    delay(10);
    LCD.print("|");
  }
}

//functions to set LEDs
void setLED(int r, int g, int b) {
  analogWrite(redpin, 255 - r);
  analogWrite(greenpin, 255 - g);
  analogWrite(bluepin, 255 - b);
}

void setExternal(int r, int g, int b) {
  analogWrite(A0, 255 - r);
  analogWrite(A1, 255 - g);
  analogWrite(A2, 255 - b);
}