#include <Servo.h>
#include <Wire.h> // I2C library, required for MLX90614
#include <SparkFunMLX90614.h> //Search for this library within Arduino IDE. Tools --> Manage Libraries --> Search for SparkFun MLX90614 in the search bar

//declare servos
Servo servoLeft;
Servo servoRight;

IRTherm therm; // Create an IRTherm object to interact with throughout

//define QTI and LED pins
#define leftQTI 49 
#define middleQTI 51
#define rightQTI 53

#define redpin 45
#define greenpin 46
#define bluepin 44

//variable used to track hash count
int hashCount;
int score;

void setup() {
  //attach servos and set up LEDs
  servoLeft.attach(11);
  servoRight.attach(12);

  pinMode(redpin, OUTPUT);
  pinMode(greenpin, OUTPUT);
  pinMode(bluepin, OUTPUT);
  setLED(0,0,0);

  pinMode(A0, OUTPUT);
  pinMode(A1, OUTPUT);
  pinMode(A2, OUTPUT);
  setExternal(0,0,0);

  Wire.begin();
  therm.begin();
  therm.setUnit(TEMP_F);

  hashCount = 0;
  score = 0;

  Serial.begin(9600); //start the serial monitor so we can view the output
  Serial2.begin(9600);
}

void loop() {
  int state = getState();
  Serial.println(state);

  if (Serial2.available()) {
    char incoming = Serial2.read();
    setExternal(0,255,255);
    delay(250);
    setExternal(0,0,0);
  }

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
      handleHash();
      break;
  }

  
}

void handleHash(){
  stop();
  hashCount++;
  
  if(hashCount==1){
    setLED(255,0,0);
    senseObject();
  }else if(hashCount==2){
    setLED(0,255,0);
    senseObject();

    if (Serial2.available())
      Serial2.print(char(90+score));
    setExternal(0,0,255);
    delay(250);
    setExternal(0,0,0);

    servoLeft.detach();
    servoRight.detach();
  }
  delay(500);
  setLED(0,0,0);
  //go forward until off hash
  forward();
  while(getState()==7)
    delay(1);
}

void senseObject(){
  if (therm.read()&&therm.object()<63.5){
    score = hashCount;
    setExternal(0,255,0);
  }else{
    setExternal(255,0,0);
  }
  delay(250);
  setExternal(0,0,0);
}

//Defines funtion 'rcTime' to read value from QTI sensor
// From Ch. 6 Activity 2 of Robotics with the BOE Shield for Arduino
boolean qtiState(int pin){
  pinMode(pin, OUTPUT);    // Sets pin as OUTPUT
  digitalWrite(pin, HIGH); // Pin HIGH
  delay(1);                // Waits for 1 millisecond
  pinMode(pin, INPUT);     // Sets pin as INPUT
  digitalWrite(pin, LOW);  // Pin LOW
  long time = micros();    // Tracks starting time
  while(digitalRead(pin)); // Loops while voltage is high
  time = micros() - time;  // Calculate decay time
  return time>150;             // Return decay time
}

//gets current state based on QTI readings
int getState(){
  boolean qti1 = qtiState(leftQTI); 
  boolean qti2 = qtiState(middleQTI);
  boolean qti3 = qtiState(rightQTI);

  return 4*qti1 + 2*qti2 + qti3;
}


//directional functions
void turnRight(){
  servoLeft.writeMicroseconds(1700);
  servoRight.writeMicroseconds(1510);
}

void turnLeft(){
  servoLeft.writeMicroseconds(1600);
  servoRight.writeMicroseconds(1370);
}

void forward(){
  servoLeft.writeMicroseconds(1660);
  servoRight.writeMicroseconds(1450);
}

void stop(){
  servoLeft.writeMicroseconds(1500);
  servoRight.writeMicroseconds(1500);
}


//function to set leds
void setLED(int r, int g, int b){
  analogWrite(redpin, 255-r);
  analogWrite(greenpin, 255-g);
  analogWrite(bluepin, 255-b);
}

void setExternal(int r, int g, int b){
  analogWrite(A0, 255-r);
  analogWrite(A1, 255-g);
  analogWrite(A2, 255-b);
}