/*
  <Project name>

  Describe what it does in layman's terms.  Refer to the components
  attached to the various pins.

  The circuit:
   list the components attached to each input
   list the components attached to each output

  Created ...
  By ...
  Modified ...
  By ...

*/

#include <SoftwareSerial.h>
#include <Wire.h>
#include <Servo.h>

Servo servo;

unsigned long previousMillisTimer = 0;
unsigned long previousMillisRPM = 0;
unsigned long stopTime = 900000; // 15 mins
char* recipient = "xxx";
const int relay1 = 5;
const int trigPin1 = 9;
const int echoPin1 = 10;
const int trigPin2 = 11;
const int echoPin2 = 12;
const int interruptPin = 15;

float value = 0;
float rev = 0;
int rpm;
int oldtime = 0;
int time;
boolean firstInit = true;

void setup() {
  Serial1.begin(19200); // Listen for GSM
  Serial2.begin(9600);  // Listen for HC-05
  Serial.begin(9600);

  // Ultrasonic setup
  pinMode(trigPin1, OUTPUT);
  pinMode(echoPin1, INPUT);
  pinMode(trigPin2, OUTPUT);
  pinMode(echoPin2, INPUT);

  // Relay setup
  pinMode(relay1, OUTPUT);

  // Servo setup
  servo.attach(20);

  //
  StartGSMModule();

  // Handle tachometer
  attachInterrupt(interruptPin, isr, RISING);
}

void loop() {
  long unsigned currentMillis = millis();
  if ((currentMillis - previousMillisTimer) >= stopTime) {
    previousMillisTimer = currentMillis;
    // Turn off machine after 15mins
    digitalWrite(relay1, LOW);
  }

  // Handle tachometer response
  if ((currentMillis - previousMillisRPM) >= 1000) {
    previousMillisRPM = currentMillis;
    detachInterrupt(interruptPin);           //detaches the interrupt
    time = millis() - oldtime;    //finds the time
    rpm = (rev / time) * 60000;   //calculates rpm
    oldtime = millis();           //saves the current time
    rev = 0;
    attachInterrupt(interruptPin, isr, RISING);
  }

  // Handle ultrasonic response
  //  int duration = pulseIn(echoPin1, HIGH);
  //  int distance = duration * 0.034 / 2;
  //  if (distance < 5) {
  //    // TODO
  //  }

  // Check for HC-05 response
  if (Serial2.available()) {
    String response = Serial2.readString();
    Serial.println(response);
    response.trim();
    if (response == F("POWERON")) {
      digitalWrite(relay1, LOW);
    }
    else if (response == F("POWEROFF")) {
      digitalWrite(relay1, HIGH);
    }
  }

  // Check for GSM900 response
  if (Serial1.available()) {
    Serial.print(Serial1.readString());
  }
}

void StartGSMModule() {
  Serial.println(F("Setting up SIM900"));

  Serial.print(F("Checking device..."));
  while (1) {
    Serial1.println(F("AT"));

    while (!Serial1.available());
    if (Serial1.find("OK")) {
      Serial.println(F("OK"));
      break;
    }
  }

  Serial.print(F("Setting to text mode..."));
  while (1) {
    Serial1.println(F("AT+CMGF=1"));

    while (!Serial1.available());
    if (Serial1.find("OK")) {
      Serial.println(F("OK"));
      break;
    }
  }

  Serial.print(F("Setting to receiving mode..."));
  while (1) {
    Serial1.println(F("AT+CNMI=2,2,0,0,0"));

    while (!Serial1.available());
    if (Serial1.find("OK")) {
      Serial.println(F("OK"));
      break;
    }
  }

  Serial.print(F("Finding network..."));
  while (1) {
    Serial1.println(F("AT+CPIN?"));

    while (!Serial1.available());
    if (Serial1.find("+CPIN: READY")) {
      Serial.println(F("Network found"));
      break;
    }
  }

  Serial.print(F("Checking signal..."));
  while (1) {
    Serial1.println(F("AT+CSQ"));

    while (!Serial1.available());
    if (Serial1.find("+CSQ")) {
      Serial.println(Serial1.readString());
      break;
    }
  }

  Serial1.println(F("AT+CMGD = 1,4\r"));

  Serial.println(F("GSM ready to be used"));
}

void isr() { //interrupt service routine
  rev++;
}

bool proximity(long &dist) {
  // TODO
}

void NotifyNumber(String number, String message) {
  char num[9];
  char* numPtr = num;

  number.toCharArray(num, 11);

  num[10] = '\0';

  Serial.println("-------------------");
  Serial1.print(F("AT+CMGS =\""));
  Serial1.print(numPtr);
  Serial1.print(F("\"\r\n"));
  delay(1000);
  Serial1.println(message);
  delay(1000);
  Serial1.println((char)26);
  delay(1000);
}
