/*
  Thrash collector

  Created Tejada
  Modified Tejada
*/

#include <Servo.h>
#include <Wire.h>

Servo servo;

unsigned long previousMillisTimer = 0;
unsigned long previousMillisRPM = 0;
const unsigned long seconds = 1000;
const unsigned long hour = 3600 * seconds;
unsigned long activeTime = 1 * hour;
unsigned long inactiveTime = 2 * hour;

// Pins
const int relay = 5;
const int trigPin1 = 9;
const int echoPin1 = 10;
const int trigPin2 = 11;
const int echoPin2 = 12;
const int irPin = 13;

// Ultrasonic
long duration;
int distance;
boolean ultrasonic_0 = true;

// Tachometer
boolean firstInit = true;
boolean relayOff = false;
int tryCount = 1;

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

  Serial.println("Initializing.");
  
  pinMode(trigPin1, OUTPUT);
  pinMode(echoPin1, INPUT);
  pinMode(trigPin2, OUTPUT);
  pinMode(echoPin2, INPUT);

  // Relay setup
  pinMode(relay, OUTPUT);

  // IR setup
  pinMode(irPin, INPUT);

  // Servo setup
  servo.attach(7);

  // Default powered on
  digitalWrite(relay, LOW);

  Serial.println("Done.");
}

void loop() {
  long unsigned currentMillis = millis();

  // Will watch during active time
  if ((currentMillis - previousMillisTimer) >= activeTime && machineActive) {
    previousMillisTimer = currentMillis;
    // Turn off machine after 1 hour
    digitalWrite(relay, HIGH);

    // Will start cycle of inactivity
    machineActive = false;

    // Prevent IR detection
    relayOff = true;
  }

  // Will watch during inactive time
  if ((currentMillis - previousMillisTimer) >= inactiveTime && !machineActive) {
    previousMillisTimer = currentMillis;
    // Turn on machine after 2 hours
    digitalWrite(relay, LOW);

    // Will start cycle of activity
    machineActive = true;

    // Restore IR detection
    relayOff = false;
  }

  // Neglects whether machine is inactive or active. Machine will be fully disabled
  if(permaOff) {
    // Force timer to be 0
    currentMillis = 0;
  }

  // Whenever IR pin has detected something
  if (digitalRead(irPin) == LOW && !relayOff) {
    // Reset clock
    previousMillisRPM = currentMillis;
    Serial.println("detected");
  }

  // Handle IR response
  if ((currentMillis - previousMillisRPM) >= 2500 && !relayOff) {
    relayOff = true;

    // Turn off relay immediately
    digitalWrite(relay, HIGH);

    // Increment tries
    if (tryCount++ < 2) {
      SendTo("9366092093",
             "WARNING: OBJECT STUCKED\n\nTry again? Reply yes to retry or no "
             "to stop");

      // Set to recieving mode.
      Serial.print(F("Setting to receiving mode..."));
      while (1) {
        Serial1.println(F("AT+CNMI=2,2,0,0,0"));

        while (!Serial1.available())
          ;
        if (Serial1.find("OK")) {
          Serial.println(F("OK"));
          break;
        }
      }

      // And wait reply of user.
      while (1) {
        while (!Serial1.available())
          ;
        if (Serial1.find("yes")) {
          Serial.println(F("yes"));
          digitalWrite(relay, HIGH);

          // Start checking for rotation again
          relayOff = false;

          break;
        } else if (Serial1.find("no")) {
          Serial.println(F("no"));
          break;
        }
      }
    } else {
      digitalWrite(relay, HIGH);
      SendTo("9162277397", "WARNING: OBJECT STUCKED");
      Serial.println("OFF");
    }
  }

  // Check for HC-05/Bluetooth incoming message
  if (Serial2.available()) {
    String response = Serial2.readString();
    Serial.println(response);
    response.trim();
    if (response == F("POWERON")) {
      // Relay on
      digitalWrite(relay, LOW);

      // Remove permaOff
      permaOff = false;

      // Signal to start the 1 Hour of activity
      machineActive = true;

      // Start checking for rotation again
      relayOff = false;

      // After bins are full wait for App to signal an ON then switch to the first bin
      servo.write(45);
    } else if (response == F("POWEROFF")) {
      // Relay off
      digitalWrite(relay, HIGH);

      // Perma off
      permaOff = true;

      // Prevent IR detection
      relayOff = true;
    }
  }

  // Servo will only work if machine is in active state
  if (ultrasonic_0 && machineActive) {
    // First ultrasonic
    firstUltrasonic();
  } else if(!ultrasonic_0 && machineActive) {
    // Second ultrasonic
    secondUltrasonic();
  }
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

  Serial.println("Sent");
}

void firstUltrasonic() {
  // Clears the trigPin
  digitalWrite(trigPin1, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin1, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin1, LOW);
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin1, HIGH);
  // Calculating the distance
  distance = duration * 0.034 / 2;
  // Prints the distance on the Serial Monitor
  Serial.print("Distance: ");
  Serial.println(distance);

  if (distance <= 24) {
    // Switch to second ultrasonic
    ultrasonic_0 = false;

    // Reset this values for second ultrasonic
    duration = 0;
    distance = 0;

    // Change to second bin
    servo.write(135);
  }
}

void secondUltrasonic() {
  digitalWrite(trigPin2, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin2, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin2, LOW);
  duration = pulseIn(echoPin2, HIGH);
  distance = duration * 0.034 / 2;
  Serial.print("Distance: ");
  Serial.println(distance);

  if (distance <= 24) {
    // Switch to first ultrasonic
    ultrasonic_0 = true;

    // Turn relay off
    digitalWrite(relay, HIGH);
    relayOff = true;

    // Disable all activities until HC-05 sends ON
    permaOff = true;
    
    Serial.print("Notifying recipient...");
    SendTo("9162277397", "WARNING: BINS ARE FULL");
    Serial.println("OK");
  }
}