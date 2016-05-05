/*

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  version 2 as published by the Free Software Foundation.
*/
/*
  Hack.lenotta.com
  Modified code of Getting Started RF24 Library
  It will switch a relay on if receive a message with text 1,
  turn it off otherwise.
  Edo
*/

#include <SPI.h>
#include "RF24.h"
#include "DHT.h"
#include <JeeLib.h>
ISR(WDT_vect) {
  Sleepy::watchdogEvent();
}

/*
   Configuration:
   - whether motion should be collected and
       setting up the PIR sensor
   - whether temperature & humidity should be
       collected and setting up the DHT sensor
   - unique sensor ID to identify which sensor
       this data belongs to
   - gateway ID for added security (must match
       gateway of receiver)
*/

// SETUP ----------------------------------------------------
// Configure PIR motion sensor
#define SENSE_MOTION true
int pirPin = 7;
// Configure temperature and humidity sensor
#define SENSE_DHT true
int dhtPin = 6;
// Unique sensorId, must also be configured in system
char sensorId[9] = "SENSOR_3";
// GatewayID, to be used for security
char gatewayId[9] = "GATEWAY1";

// Max payload size for RF radios
size_t messageLength = 32;
size_t sensorIdLength = 8;
size_t gatewayIdLength = 8;
size_t headerSize = sensorIdLength + gatewayIdLength;
// Set up nRF24L01 radio on SPI bus plus pins 9 & 10
RF24 radio(9, 10);
// Radio pipe addresses for the 2 nodes to communicate.
const uint64_t pipe[2] = {0xE8E8F0F0E1LL, 0xF0F0F0F0E1LL};
// PIR setup
int pirState = LOW;
int val = 0;
DHT dht(dhtPin, DHT11);
int relay = 8;


// DELAYS AND DATA --------------------------------------------------
// delay after each reading
long loopInterval = 1000L;
long dhtInterval = 15*loopInterval;
long motionInterval = 5*loopInterval;
long lastMotion = 0;
long lastSend = 0;
long lastDHT = 0;

void setup(void)
{
  Serial.begin(9600);
  pinMode(relay, OUTPUT);
  digitalWrite(relay, HIGH);
  pinMode(pirPin, INPUT);
  Serial.begin(9600);
  Serial.println("\nBooting Arduino...\n\r");

  if (SENSE_MOTION) {
    radio.begin();
    radio.setRetries(15, 15);
    radio.openWritingPipe(pipe[0]);
    radio.openReadingPipe(1,pipe[1]);
    radio.startListening();
    radio.printDetails();
  }
  if (SENSE_DHT) {
    dht.begin();
  }
}

char* writeToBuffer(char* curr, int len, char* msg) {
  // Write gatewayId into message
  for (int i = 0; i < len; i++) {
    if (i < len) {
      *curr++ = msg[i];
    } else {
      *curr++ = ' ';
    }
  }
}

char* writeStringToBuffer(char* curr, int len, String msg) {
  // Write gatewayId into message
  for (int i = 0; i < len; i++) {
    *curr++ = msg.charAt(i);
  }
}

void writeHeader(char* curr) {
  // Write gatewayId into message
  writeToBuffer(curr, gatewayIdLength, gatewayId);
  curr += gatewayIdLength;

  // Write sensorId into message
  writeToBuffer(curr, sensorIdLength, sensorId);
  curr += sensorIdLength;
}

bool readMotion() {
  val = digitalRead(pirPin);
  if (val == HIGH) {
    Serial.println("Motion!");
    lastMotion = millis();
    if (pirState == LOW) {
      pirState = HIGH;
    }
    sendMotionData();
    return true;
  } else {
    if (val == LOW && pirState == HIGH) {
      pirState = LOW;
    }
  }
  return false;
}

void sendMotionData() {
  char message[messageLength];
  char * curr = message;
  writeHeader(curr);
  curr += 16;
  *curr++ = 'M';
  int dataWritten = 0;
  while (dataWritten < 15) {
    *curr++ = ' ';
    dataWritten += 1;
  }

  Serial.println(message);

  bool success = false;
  while (!success) {
    // first 16 bytes is header
    // next 16 bytes is motion information

    radio.stopListening();
    success = radio.write(message, messageLength);
    radio.printDetails();
    if (success) {
      lastSend = millis();
      Serial.println("Sent message");
    } else {
      Serial.println("Failed to send message");
    }
  }
  radio.startListening();
}

bool readDHTData() {
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(f)) {
    Serial.println("Failed to read from DHT sensor!");
    return false;
  }

  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(f);
  Serial.println(" *F");

  long now = millis();
  sendTempData(f,h);

  return true;
}

void sendTempData(float temp, float humidity) {
  char message[messageLength];
  char * curr = message;
  writeHeader(curr);
  curr += 16;
  *curr++ = 'T';
  writeStringToBuffer(curr, 5, String(temp));
  curr += 5;
  *curr++ = 'H';
  writeStringToBuffer(curr, 5, String(humidity));
  curr += 5;
  int dataWritten = 0;
  while (dataWritten < 4) {
    *curr++ = ' ';
    dataWritten += 1;
  }

  Serial.println(message);

  bool success = false;
  while (!success) {
    // first 16 bytes is header
    // next 16 bytes is motion information

    radio.stopListening();
    success = radio.write(message, messageLength);
    radio.printDetails();
    if (success) {
      lastSend = millis();
      Serial.println("Sent message");
    } else {
      Serial.println("Failed to send message");
    }
  }
  radio.startListening();
}

void loop(void) {
  long now = millis();
  if (SENSE_DHT && now - lastDHT > dhtInterval) {
    Serial.println("reading DHT data");
    readDHTData();
    lastDHT = now;
  }
  if (SENSE_MOTION && now - lastMotion > motionInterval) {
    Serial.println("reading motion data");
    readMotion();
    lastMotion = now;
  }
  Sleepy::loseSomeTime(loopInterval);
}
