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
NewPing sonar[2] = {   // Sensor object array.
  NewPing(9, 10, 200), // Each sensor's trigger pin, echo pin, and max distance to ping. 
  NewPing(11, 12, 200)
};
boolean ultrasonic_1_init = true;

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
	}

	// Servo will only work if machine is in active state
	for (uint8_t i = 0; i < 2; i++) { // Loop through each sensor and display results.
		delay(50); // Wait 50ms between pings (about 20 pings/sec). 29ms should be the shortest delay between pings.
		Serial.print(i);
		Serial.print("=");
		Serial.print(sonar[i].ping_cm());
		Serial.print("cm ");

		if (sonar[0].ping_cm() <= 21 && ultrasonic_1_init && machineActive) {
			servo.write(135);

			ultrasonic_1_init = false;
		}
		else if (sonar[1].ping_cm() <= 21 && !ultrasonic_1_init && machineActive) {
			servo.write(45);

			ultrasonic_1_init = true;

			// Turn RELAY_PIN off
			digitalWrite(RELAY_PIN, HIGH);
			relayOff = true;

			// Disable all activities until HC-05 sends ON
			permaOff = true;

			// Notify
			Serial.print(F("Notifying recipient..."));
			SendTo("9162277397", "WARNING: BINS ARE FULL");
			Serial.println(F("OK"));
		}
	}

	Serial.println();
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

void firstUltrasonic() {
	// Clears the trigPin
	digitalWrite(TRIG_PIN_1, LOW);
	delayMicroseconds(2);
	// Sets the trigPin on HIGH state for 10 micro SECONDS
	digitalWrite(TRIG_PIN_1, HIGH);
	delayMicroseconds(10);
	digitalWrite(TRIG_PIN_1, LOW);
	// Reads the echoPin, returns the sound wave travel time in microseconds
	long duration = pulseIn(ECHO_PIN_1, HIGH);
	// Calculating the distance
	float distance = duration * 0.034 / 2;
	// Prints the distance on the Serial Monitor
	Serial.print(F("Distance: "));
	Serial.println(distance);

	if (distance <= 21) {
		// Switch to second ultrasonic
		ultrasonic_1_init = false;

		// Reset this values for second ultrasonic
		duration = 0;
		distance = 0;

		// Change to second bin
		servo.write(45);
	}
}

void secondUltrasonic() {
	digitalWrite(TRIG_PIN_2, LOW);
	delayMicroseconds(2);
	digitalWrite(TRIG_PIN_2, HIGH);
	delayMicroseconds(10);
	digitalWrite(TRIG_PIN_2, LOW);
	long duration = pulseIn(ECHO_PIN_2, HIGH);
	float distance = duration * 0.034 / 2;
	Serial.print(F("Distance: "));
	Serial.println(distance);

	if (distance <= 21) {
		servo.write(135);

		// Switch to first ultrasonic
		ultrasonic_1_init = true;

		// Turn RELAY_PIN off
		digitalWrite(RELAY_PIN, HIGH);
		relayOff = true;

		// Disable all activities until HC-05 sends ON
		permaOff = true;

		Serial.print(F("Notifying recipient..."));
		SendTo("9162277397", "WARNING: BINS ARE FULL");
		Serial.println(F("OK"));
	}
}