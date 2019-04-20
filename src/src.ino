/*
  Thrash collector

  Created Tejada
  Modified Tejada
*/

#include <NewPing.h>
#include <Servo.h>
#include <Wire.h>

Servo servo;

unsigned long previousMillisTimer = 0;
unsigned long previousMillisRPM = 0;
const unsigned long SECONDS = 1000;
const unsigned long HOUR = 3600 * SECONDS;
unsigned long activeTime = 1 * HOUR;
unsigned long inactiveTime = 2 * HOUR;

// Pins
const int RELAY_PIN = 5;
const int TRIG_PIN_1 = 9;
const int ECHO_PIN_1 = 10;
const int TRIG_PIN_2 = 11;
const int ECHO_PIN_2 = 12;
const int IR_PIN = 13;

// Ultrasonic
long duration;
long distance;
long rightSensor;
long leftSensor;
boolean initialSonarRun = true;

// IR
boolean firstInit = true;
boolean relayOff = false;
boolean irEvaluate = false;
int isObstacle = HIGH;

// Signals for machine timer of inactivity or activity
boolean machineActive = true;
boolean permaOff = false;

void setup() {
  // Listen for GSM
  Serial1.begin(115200);
  // Listen for HC-05
  Serial2.begin(9600);
  // Debugging purpose
  Serial.begin(9600);

  // Ultrasonic setup
  pinMode(TRIG_PIN_1, OUTPUT);
  pinMode(ECHO_PIN_1, INPUT);
  pinMode(TRIG_PIN_2, OUTPUT);
  pinMode(ECHO_PIN_2, INPUT);

  // Relay setup
  pinMode(RELAY_PIN, OUTPUT);

  // IR setup
  pinMode(IR_PIN, INPUT);

  // Servo setup
  servo.attach(7);

  // Default powered on
  digitalWrite(RELAY_PIN, LOW);

  // Defaults to first bin
  servo.write(135);

  Serial.println(F("Started."));
}

void loop() {
  long unsigned currentMillis = millis();

  // Will watch during active time
  if ((currentMillis - previousMillisTimer) >= activeTime && machineActive) {
    previousMillisTimer = currentMillis;
    // Turn off machine after 1 HOUR
    digitalWrite(RELAY_PIN, HIGH);

    // Will start cycle of inactivity
    machineActive = false;

    // Prevent IR detection
    relayOff = true;
  }

  // Will watch during inactive time
  if ((currentMillis - previousMillisTimer) >= inactiveTime && !machineActive) {
    previousMillisTimer = currentMillis;
    // Turn on machine after 2 hours
    digitalWrite(RELAY_PIN, LOW);

    // Will start cycle of activity
    machineActive = true;

    // Restore IR detection
    relayOff = false;
  }

  // Neglects whether machine is inactive or active. Machine will be fully
  // disabled
  if (permaOff) {
    // Force timer to be 0
    currentMillis = 0;
  }

  isObstacle = digitalRead(IR_PIN);
  // Whenever IR pin has detected something
  if (isObstacle == LOW && !relayOff) {
    // Reset clock
    previousMillisRPM = currentMillis;
  }

  // Handle IR response
  if ((currentMillis - previousMillisRPM) >= 20000 && (currentMillis - previousMillisRPM) <= 25000 && !relayOff) {
    // Relay off
    digitalWrite(RELAY_PIN, HIGH);

    // Perma off
    permaOff = true;

    // Prevent IR detection
    relayOff = true;

    machineActive = false;
    
    // Notify
    Serial.print(F("Notifying recipient..."));
    SendTo("9350560351", "WARNING: OBJECT STUCKED");
    Serial.println(F("OK"));
  }

  if (initialSonarRun && machineActive) {
    SonarSensor(TRIG_PIN_1, ECHO_PIN_1);
    rightSensor = distance;
    if (rightSensor <= 24) {
      // Switch to next bin
      servo.write(45);

      initialSonarRun = false;
    }
  }
  else if (!initialSonarRun && machineActive) {
    SonarSensor(TRIG_PIN_2, ECHO_PIN_2);
    leftSensor = distance;
    if (leftSensor <= 24) {
      // Turn RELAY_PIN off
      digitalWrite(RELAY_PIN, HIGH);
      relayOff = true;

      // Disable all activities until HC-05 sends ON
      permaOff = true;

      // Notify
      Serial.print(F("Notifying recipient..."));
      SendTo("9350560351", "WARNING: BINS ARE FULL");
      Serial.println(F("OK"));

      // Then reset to original bin
      servo.write(135);

      initialSonarRun = true;
    }
  }

  Serial.print(leftSensor);
  Serial.print(" - ");
  Serial.print(rightSensor);
  Serial.println();
}

void SonarSensor(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance = (duration / 2) / 29.1;
}

void SendTo(char *number, String message) {
  Serial1.print(F("Sending to"));
  Serial1.println(number);

  *(number + 10) = '\0';

  Serial1.print(F("AT+CMGS =\""));
  Serial1.print(number);
  Serial1.print(F("\"\r\n"));
  delay(1000);
  Serial1.println(message);
  delay(1000);
  Serial1.println((char)26);
  delay(3000);

  Serial.println(F("Sent"));
}

// Check for HC-05/Bluetooth incoming message
void serialEvent2() {
  if (Serial2.available()) {
    char data[20];
    strcpy(data, Serial2.readString().c_str());
    if (strstr(data, "POWERON") != nullptr) {
      Serial.println("STATE ON");

      // Relay on
      digitalWrite(RELAY_PIN, LOW);

      // Remove permaOff
      permaOff = false;

      // Signal to start the 1 Hour of activity
      machineActive = true;

      // Start checking for rotation again
      relayOff = false;

      // After bins are full wait for App to signal an ON then switch to the
      // first bin
      servo.write(135);
    }
    else if (strstr(data, "POWEROFF") != nullptr) {
      Serial.println("STATE OFF");

      // Relay off
      digitalWrite(RELAY_PIN, HIGH);

      // Perma off
      permaOff = true;

      // Prevent IR detection
      relayOff = true;
    }
  }
}
