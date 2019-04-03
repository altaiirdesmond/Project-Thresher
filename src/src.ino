/*
  Thrash collector

  Created Tejada
  Modified Tejada
*/

#include <Wire.h>
#include <Servo.h>

Servo servo;

unsigned long previousMillisTimer = 0;
unsigned long previousMillisRPM = 0;
unsigned long stopTime = 1800000; // 30 mins
const int relay1 = 5;
const int trigPin1 = 9;
const int echoPin1 = 10;
//const int trigPin2 = 11;
//const int echoPin2 = 12;
const int irPin = 13;

// Ultrasonic
long duration;
int distance;

// Tachometer
boolean firstInit = true;
boolean relayOff = false;
int tryCount = 1;

void setup() {
	Serial1.begin(115200); // Listen for GSM
	Serial2.begin(9600);  // Listen for HC-05
	Serial.begin(9600);

	// Ultrasonic setup
	pinMode(trigPin1, OUTPUT);
	pinMode(echoPin1, INPUT);
	//pinMode(trigPin2, OUTPUT);
	//pinMode(echoPin2, INPUT);

	// Relay setup
	pinMode(relay1, OUTPUT);

	//
	pinMode(irPin, INPUT);

	// Servo setup
	servo.attach(20);

	// Default powered on
	digitalWrite(relay1, LOW);
}

void loop() {
	long unsigned currentMillis = millis();
	if ((currentMillis - previousMillisTimer) >= 10000) {
		previousMillisTimer = currentMillis;
		// Turn off machine after 15mins
		digitalWrite(relay1, HIGH);
	}

	if (digitalRead(irPin) == LOW && !relayOff) {
		// Reset clock
		previousMillisRPM = currentMillis;
	}

	// Handle IR response
	if ((currentMillis - previousMillisRPM) >= 2500 && !relayOff) {
		relayOff = true;

		// Turn off relay immediately
		digitalWrite(relay1, HIGH);
		
		// Increment tries
		if (tryCount++ < 2) {
			SendTo("9366092093", "WARNING: OBJECT STUCKED\n\nTry again? Reply yes to retry or no to stop");
			
			// Set to recieving mode.
			Serial.print(F("Setting to receiving mode..."));
			while (1) {
				Serial1.println(F("AT+CNMI=2,2,0,0,0"));

				while (!Serial1.available());
				if (Serial1.find("OK")) {
					Serial.println(F("OK"));
					break;
				}
			}

			// And wait reply of user.
			while (1) {
				while (!Serial1.available());
				if (Serial1.find("yes")) {
					Serial.println(F("yes"));
					digitalWrite(relay1, HIGH);

          // Start checking for rotation again
					relayOff = false;
         
					break;
				}
				else if (Serial1.find("no")) {
					Serial.println(F("no"));
					break;
				}
			}
		}
		else { 
			// 
			digitalWrite(relay1, HIGH);
			SendTo("9366092093", "WARNING: OBJECT STUCKED");
			Serial.println("OFF");
		}
	}

	// Check for HC-05 response
	if (Serial2.available()) {
		String response = Serial2.readString();
		Serial.println(response);
		response.trim();
		if (response == F("POWERON")) {
			digitalWrite(relay1, LOW);

      // Start checking for rotation again
      relayOff = false;
		}
		else if (response == F("POWEROFF")) {
			digitalWrite(relay1, HIGH);
		}
	}

	// Ultrasonic
	CheckDistance();
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

void CheckDistance() {
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

	if (distance <= 4) {
		SendTo("9366092093", "WARNING BIN FULL");
	}
}
