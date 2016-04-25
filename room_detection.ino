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

/*
 * 
 */

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "DHT.h"

/*
 * Configuration:
 * - whether motion should be collected and
 *     setting up the PIR sensor
 * - whether temperature & humidity should be
 *     collected and setting up the DHT sensor
 * - unique sensor ID to identify which sensor
 *     this data belongs to
 * - gateway ID for added security (must match
 *     gateway of receiver)
 */
// Configure PIR motion sensor
#define SENSE_MOTION true
int pirPin = 7;
// Configure temperature and humidity sensor
#define SENSE_TEMPERATURE true
int dhtPin = 6;
// Unique sensorId, must also be configured in system
char sensorId[9] = "SENSOR_1";
// GatewayID, to be used for security
char gatewayId[9] = "GATEWAY1";

int loopInterval = 500;

int relay = 8;
// Max payload size for RF radios
size_t messageLength = 32;

size_t sensorIdLength = 8;
size_t gatewayIdLength = 8;

size_t headerSize = sensorIdLength + gatewayIdLength;

long lastMotion = 0;
long lastSend = 0;
long sendInterval = 60 * 1000L; // 1 minute (60000 ms)

// Set up nRF24L01 radio on SPI bus plus pins 9 & 10 
RF24 radio(9,10);

// Radio pipe addresses for the 2 nodes to communicate.
//const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };
const uint64_t pipe = 0xE8E8F0F0E1LL;

// PIR setup
int pirState = LOW;
int val = 0;

DHT dht(dhtPin, DHT11);

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
    radio.setRetries(15,15);
    radio.openWritingPipe(pipe);
    radio.startListening();
    radio.printDetails();
  }

  if (SENSE_TEMPERATURE) {
    dht.begin();
  }
}

char* writeToBuffer(char* curr, int len, char* msg) {
    // Write gatewayId into message
    for (int i=0; i<len; i++) {
      if (i < len) {
        *curr++ = msg[i];
      } else {
        *curr++ = ' ';
      }
    }
}

char* writeStringToBuffer(char* curr, int len, char type, String msg) {
    // Write gatewayId into message
    for (int i=0; i<len; i++) {
      if (i == 0) {
        *curr++ = type;
      } else if (i - 1 < msg.length()) {
        *curr++ = msg.charAt(i - 1);
      } else {
        *curr++ = ' ';
      }
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

bool readTemperature(char *message) {
  char *curr = message;
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

  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(f);
  Serial.println(" *F");
  
  writeHeader(curr);
  curr += headerSize;

  String temperature = String(f);
  writeStringToBuffer(curr, 8, 'T', temperature);
  curr += 8;

  String humidity = String(h);
  writeStringToBuffer(curr, 8, 'H', humidity);
  curr += 8;
  
  return true;
}

bool readMotion(char* message){
  char * curr = message; 

  writeHeader(curr);
  curr += 16;

  val = digitalRead(pirPin);
  if (val == HIGH) {
    Serial.println("Motion!");
    lastMotion = millis();

    if (pirState == LOW) {
      Serial.println("(new motion)");
      pirState = HIGH;
    }
    writeStringToBuffer(curr, 16, 'M', String(1));
    curr += 16;

    return true;

  } else {
    if (val == LOW && pirState == HIGH) {
      Serial.println("Motion ended!");
      pirState = LOW;
    }

    writeStringToBuffer(curr, 16, 'M', String(0));
    curr += 16;
  }
  
  return false;
}

void readAndSendMotionData(){
  char motionMessage[messageLength];
  bool motionSensed = readMotion(motionMessage);

  long now = millis();
  // Only send if it's been > 1 min since the last send
//  if (motionSensed && now - lastSend > sendInterval) {
  if (motionSensed) {
    // First, stop listening so we can talk
    radio.stopListening();
  
    // Send the final one back.
    bool success = radio.write(motionMessage, messageLength );
    radio.printDetails();
    
    if (success) {
      lastSend = millis();
      Serial.print("Sent motion message: ");
      Serial.println(motionMessage);
      Serial.println();
    } else {
      Serial.println("Failed to send message");
    }
    
    // Now, resume listening so we catch the next packets.
    radio.startListening();
  }
}

void readAndSendTempData(){
  char tempMessage[messageLength];
  bool tempSuccess = readTemperature(tempMessage);

  // Only send if it's been > 1 min since the last send
  if (tempSuccess) {
    // First, stop listening so we can talk
    radio.stopListening();
  
    // Send the final one back.
    bool success = radio.write(tempMessage, messageLength);

    if (success) {
      lastSend = millis();
      Serial.print("Sent temp message: ");
      Serial.println(tempMessage);
      Serial.println();
    } else {
      Serial.println("Failed to send message");
    }
    
    // Now, resume listening so we catch the next packets.
    radio.startListening();
  }
}


void loop(void) {
  if (SENSE_MOTION) {
    readAndSendMotionData();
  }
  if (SENSE_TEMPERATURE) {
    readAndSendTempData();
  }
  delay(random(500,3000));
}
