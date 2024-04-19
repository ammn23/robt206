#include <Servo.h>
#include <SPI.h>
#include <Ethernet.h>

// MAC address and IP address
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(192, 168, 137, 177);

EthernetServer server(80);
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
bool aMode = false;
int rotStage = 0;
//rotary encoder
const int clkPin = 2;  //the clk attach to pin2
const int dtPin = 3;   //the dt attach to pin3
const int swPin = 9;   //the number of the button
unsigned long lastManualUpdate = 0;

const int swCtrl = 5;

int encoderVal = 0;
int fBound;
int sBound;
int baseAngle;
int sideAngle;

void setup() {
  Serial.begin(9600);  // Initialize serial communication

  while (!Serial) {
    ;
  }
  Serial.println("Ethernet WebServer Example");

  // start the server
  Ethernet.begin(mac, ip);

  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    while (true) {
      delay(1);  // do not point run without Ethernet
    }
  }
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
  }

  // start the server
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());

  //solar panel init-n
  lightVal1 = analogRead(sensorPin1);
  lightVal2 = analogRead(sensorPin2);
  servoBase.attach(7);
  servoSide.attach(8);
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
  rotStage = 1;
  aMode = true;
  weboutput();
  //goes through all values to find the angle for maxLight
  for (int i = 0; i < 180; i += 10) {
    Serial.print("autoin-----------------------------");
    servoBase.write(i);
    delay(500);
    lightVal1 = analogRead(sensorPin1);
    lightVal2 = analogRead(sensorPin2);
    if (lightVal1 < maxfLight) {
      maxfLight = lightVal1;
      maxfAngle = i;
      weboutput();
    }
    if (lightVal2 < maxfLight) {
      maxfLight = lightVal2;
      maxfAngle = i + 180;
      weboutput();
    }
  }
  servoBase.write(maxfAngle);  //returns base servo motor on the maxAngle position
  baseAngle = maxfAngle;
  delay(500);

  //it goes to the Side servo motor
  maxfLight = analogRead(sensorPin1);  //reads the value of light from photoresistor
  maxfAngle = 0;
  rotStage = 2;

  //goes through all values for side servo motor
  for (int i = 0; i < 180; i += 10) {
    servoSide.write(i);
    delay(500);
    lightVal2 = analogRead(sensorPin2);
    if (lightVal2 < maxfLight) {
      maxfLight = lightVal2;
      maxfAngle = i;
      weboutput();
    }
    delay(500);
  }
  servoSide.write(maxfAngle);
  sideAngle = maxfAngle;

  delay(3000);
  rotStage = 3;
  maxsAngle = maxfAngle;
  maxsLight = analogRead(sensorPin1);
  if (180 - maxfAngle >= maxfAngle) {  //checks which trajectory is more convenient to choose

    for (int i = maxfAngle; i > 0; i -= 10) {  //starts finding the maxLight in specific range
      servoSide.write(i);
      lightVal2 = analogRead(sensorPin2);
      if (lightVal2 < maxsLight) {
        maxsLight = lightVal2;
        maxsAngle = i;
      }
      delay(500);
    }
    if (maxsLight <= maxfLight) {  //checks which light intensity is larger
      //from maxslight to zero, write range which should be followed
      fBound = maxsAngle;
      sBound = 0;
      weboutput();
      for (int i = maxsAngle; i > 0; i -= 5) {  //if it found more intensive light in the range,then moves further in the direction
        servoSide.write(i);
        delay(1000);
      }
    } else {
      //from maxflight to 180, write range which should be followed
      fBound = maxfAngle;
      sBound = 180;
      weboutput();
      for (int i = maxfAngle; i < 180; i += 5) {  //if not, then it moves in the opposite direction
        servoSide.write(i);
        delay(1000);
      }
    }
  } else {                                       //chose another direction
    for (int i = maxfAngle; i < 180; i += 10) {  //goes through the values in the particular range
      servoSide.write(i);
      lightVal2 = analogRead(sensorPin2);
      if (lightVal2 < maxsLight) {
        maxsLight = lightVal2;
        maxsAngle = i;
      }
      delay(500);
    }
    if (maxsLight <= maxfLight) {  //checks if it found more intensive light
      //from maxslight to 180, write range which should be followed
      fBound = maxsAngle;
      sBound = 180;
      weboutput();
      for (int i = maxsAngle; i < 180; i += 5) {  //if yes, then it continues moving in the same direction
        servoSide.write(i);
        delay(1000);
      }
    } else {
      //from maxfangle to zero, write range which should be followed
      fBound = maxfAngle;
      sBound = 0;
      weboutput();
      for (int i = maxfAngle; i > 0; i -= 5) {  //if not, moves in the opposite direction
        servoSide.write(i);
        delay(1000);
      }
    }
  }
}

void manualMode() {
  currentStateCLK = digitalRead(clkPin);
  lightVal2 = analogRead(sensorPin2);
  if (millis() - lastManualUpdate >= 10000 || lastManualUpdate == 0) {
    aMode = false;
    lastManualUpdate = millis();
    weboutput();
  }
  // If last and current state of CLK are different, then pulse occurred
  // React to only 1 state change to avoid double count
  if (currentStateCLK != lastStateCLK && currentStateCLK == 1) {

    // If the dt state is different than the clk state then the encoder is rotating CCW so decrement
    if (baseSpin) {

      if (digitalRead(dtPin) != currentStateCLK) {
        counter -= 10;
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
      } else {
        // Encoder is rotating CW so increment
        counter += 10;
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
      if (baseSpin) baseSpin = false;  //if base was rotating before pressing the button then side servo motor will be controlled
      else baseSpin = true;            //if side was rotating before pressing the button then base servo motor will be controlled
    }

    // Remember last button press event
    lastButtonPress = millis();
  }

  // Put in a slight delay to help debounce the reading
  delay(1);
}


void weboutput() {
  EthernetClient client = server.available();
  if (client) {
    Serial.println("new client");
    // an HTTP request ends with a blank line
    bool currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the HTTP request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard HTTP response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println("Refresh: 5");         // refresh the page automatically every 5 sec
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
          if (!aMode) {
            client.print("Current Mode: Manual ");
            client.println("<br />");
            client.print("Last Checked Light: ");
            client.print(lightVal2);
          } else {
            if (rotStage != 3) {
              client.print("Current Mode: Auto ");
              client.println("<br />");
              client.print("Current Stage: ");
              client.print(rotStage);
              client.println("<br />");
              client.print("Maximum Light Found: ");
              client.print(maxfLight);
              client.println("<br />");
              client.print("Angle of Maximum Light: ");
              client.print(maxfAngle);
              client.println("<br />");
            } else {
              client.print("Current Mode: Auto ");
              client.println("<br />");
              client.print("Current Stage: 3");
              client.println("<br />");

              client.print("  Base Angle: ");
              client.print(baseAngle);
              client.println("<br />");
              client.print("  Starting Side Angle: ");
              client.print(sideAngle);
              client.println("<br />");
              client.print("The Range of Following: ");
              client.print(fBound);
              client.print(" - ");
              client.print(sBound);
              client.println("<br />");
            }
          }
          //else{}
          client.println("</html>");
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    delay(1);
    client.stop();
    Serial.println("client disconnected");
  }
}
