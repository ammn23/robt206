#include <Servo.h>

//initialization of servo motors
Servo servoBase; 
Servo servoSide;

//variables and pin assignments
const int sensorPin1 = A0;
const int sensorPin2 = A1;
//values of light and maxValues from photoresistors
int lightVal1;
int lightVal2;
int maxfLight;
int maxsLight;
//values of angles of maxValues
int maxfAngle;
int maxsAngle;
int counter = 0;
int lastCounter = 0;
int currentStateCLK;
int lastStateCLK;
String currentDir = "";
unsigned long lastButtonPress = 0;
bool baseSpin = true;
bool buttonPressed = true;

//rotary encoder
const int clkPin = 2;  //the clk attach to pin2
const int dtPin = 3;   //the dt attach to pin3
const int swPin = 4;   //the number of the button

const int swCtrl = 6;

int encoderVal = 0;

void setup() {
  Serial.begin(9600);  // Initialize serial communication
  lightVal1 = analogRead(sensorPin1);
  lightVal2 = analogRead(sensorPin2);
  servoBase.attach(13);
  servoSide.attach(12);
  pinMode(clkPin, INPUT);
  pinMode(dtPin, INPUT);
  pinMode(swPin, INPUT);
  digitalWrite(swPin, HIGH);
  lastStateCLK = digitalRead(clkPin);
}

//checks if the button on rotary encoder is pressed
//If it is, then the autoMode() will run
//if not, it will stary in manualMode()
void loop() {
  int btnState1 = digitalRead(swCtrl);
  if (btnState1 == LOW) {
    if (millis() - lastButtonPress > 50) {
      autoMode();
    }
    lastButtonPress = millis();
  }
  manualMode();
}

//the function of autoMode()
void autoMode() {
  maxfAngle = 0;
  maxfLight = analogRead(sensorPin1);
  servoBase.write(0);
  servoSide.write(0);
  delay(2000);
  //goes through all values to find the angle for maxLight
  for (int i = 0; i < 180; i += 10) {
    servoBase.write(i);
    delay(500);
    lightVal1 = analogRead(sensorPin1);
    lightVal2 = analogRead(sensorPin2);
    if (lightVal1 < maxfLight) {
      maxfLight = lightVal1;
      maxfAngle = i;
    }
    if (lightVal2 < maxfLight) {
      maxfLight = lightVal2;
      maxfAngle = i;
    }
  }
  servoBase.write(maxfAngle); //returns base servo motor on the maxAngle position
  delay(500);

  //it goes to the Side servo motor
  maxfLight = analogRead(sensorPin1); //reads the value of light from photoresistor
  maxfAngle = 0;

  //goes through all values for side servo motor
  for (int i = 0; i < 180; i += 10) {
    servoSide.write(i);
    delay(500);
    lightVal2 = analogRead(sensorPin2);
    if (lightVal2 < maxfLight) {
      maxfLight = lightVal2;
      maxfAngle = i;
    }
    delay(500);
  }
  servoSide.write(maxfAngle);

  delay(3000);
  maxsAngle = maxfAngle;
  maxsLight = analogRead(sensorPin1);
  if (180 - maxfAngle >= maxfAngle) { //checks which trajectory is more convenient to choose
    for (int i = maxfAngle; i > 0; i -= 10) { //starts finding the maxLight in specific range
      servoSide.write(i);
      lightVal2 = analogRead(sensorPin2);
      if (lightVal2 < maxsLight) {
        maxsLight = lightVal2;
        maxsAngle = i;
      }
      delay(500);
    }
    if (maxsLight <= maxfLight) { //checks which light intensity is larger
      for (int i = maxsAngle; i > 0; i -= 5) { //if it found more intensive light in the range,then moves further in the direction
        servoSide.write(i);
        delay(1000);
      }
    } else {
      for (int i = maxfAngle; i < 180; i += 5) { //if not, then it moves in the opposite direction
        servoSide.write(i);
        delay(1000);
      }
    }
  } else { //chose another direction
    for (int i = maxfAngle; i < 180; i += 10) { //goes through the values in the particular range
      servoSide.write(i);
      lightVal2 = analogRead(sensorPin2);
      if (lightVal2 < maxsLight) {
        maxsLight = lightVal2;
        maxsAngle = i;
      }
      delay(500);
    }
    if (maxsLight <= maxfLight) { //checks if it found more intensive light
      for (int i = maxsAngle; i < 180; i += 5) { //if yes, then it continues moving in the same direction
        servoSide.write(i);
        delay(1000);
      }
    } else {
      for (int i = maxfAngle; i > 0; i -= 5) { //if not, moves in the opposite direction
        servoSide.write(i);
        delay(1000);
      }
    }
  }
}

void manualMode() {
  currentStateCLK = digitalRead(clkPin);

  // If last and current state of CLK are different, then pulse occurred
  // React to only 1 state change to avoid double count
  if (currentStateCLK != lastStateCLK && currentStateCLK == 1) {

    // If the dt state is different than the clk state then the encoder is rotating CCW so decrement
    if (baseSpin) {
      if (digitalRead(dtPin) != currentStateCLK) {
        counter -= 10;
        currentDir = "CCW";
      } else {
        // Encoder is rotating cw so increment
        counter += 10;
        currentDir = "CW";
      }
      if (counter >= 0 && counter <= 180) {
        if (lastCounter != counter) {
          if (counter == 0) {
            servoBase.write(5);
          } else if (counter == 180) {
            servoBase.write(179);
          } else {
            servoBase.write(counter);
          }
        }
        lastCounter = counter;
      } else if (counter > 180) counter = 180;
      else if (counter < 0) counter = 0;
    } else {
      if (digitalRead(dtPin) != currentStateCLK) {
        counter -= 10;
        currentDir = "CCW";
      } else {
        // Encoder is rotating CW so increment
        counter += 10;
        currentDir = "CW";
      }
      if (counter >= 0 && counter <= 180) {
        if (lastCounter != counter) {
          if (counter == 0) {
            servoSide.write(5);
          } else if (counter == 180) {
            servoSide.write(179);
          } else {
            servoSide.write(counter);
          }
        }
        lastCounter = counter;
      } else if (counter > 179) counter = 179;
      else if (counter < 0) counter = 0;
    }
  }

  // Remember last CLK state
  lastStateCLK = currentStateCLK;

  // Read the button state
  int btnState = digitalRead(swPin);

  //If we detect LOW signal, button is pressed, so we change the type of servo motor to control
  if (btnState == LOW) {
    //if 50ms have passed since last LOW pulse, it means that the
    //button has been pressed, released and pressed again
    if (millis() - lastButtonPress > 50) {
      if (baseSpin) baseSpin = false; //if base was rotating before pressing the button then side servo motor will be controlled 
      else baseSpin = true; //if side was rotating before pressing the button then base servo motor will be controlled
    }

    // Remember last button press event
    lastButtonPress = millis();
  }

  // Put in a slight delay to help debounce the reading
  delay(1);
}
