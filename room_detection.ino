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
#include "nRF24L01.h"
#include "RF24.h"

int relay = 8;
size_t messageLength = sizeof("hello");

// Set up nRF24L01 radio on SPI bus plus pins 9 & 10 
RF24 radio(9,10);

// Radio pipe addresses for the 2 nodes to communicate.
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

// PIR setup
int pirPin = 2;
int pirState = LOW;
int val = 0;

void setup(void)
{
  Serial.begin(57600);
  pinMode(relay, OUTPUT);
  digitalWrite(relay, HIGH);
  
  pinMode(pirPin, INPUT);
  
  Serial.begin(9600);
  Serial.println("\nBooting Arduino...\n\r");

  radio.begin();
//  radio.setAutoAck(1);                    // Ensure autoACK is enabled
  radio.setRetries(15,15);
  radio.openWritingPipe(pipes[1]);
  radio.openReadingPipe(1,pipes[0]);
  radio.startListening();
  radio.printDetails();
}

bool readState(char* message){
  char * curr = message; 

  val = digitalRead(pirPin);
  if (val == HIGH) {
    // TODO motion detected
    Serial.println("Motion!");
    if (pirState == LOW) {
      Serial.println("(new motion)");
      pirState = HIGH;
    }
    char str[8] = "hello";
    for (int i=0; i<6; i++) {
      *curr++ = str[i];
    }
    return true;
  } else {
    if (val == LOW && pirState == HIGH) {
      Serial.println("Motion ended!");
      pirState = LOW;
    }  
  }
  
  return false;
}

void readAndSendData(){
  unsigned short action, id, length, callback;
  char message[10];
  bool motionSensed = readState(message);

  if (motionSensed) {
    // First, stop listening so we can talk
    radio.stopListening();
  
    // Send the final one back.
    radio.write( message, messageLength );
    Serial.print("Sent message: ");
    Serial.println(message);
    Serial.println();
  
    // Now, resume listening so we catch the next packets.
    radio.startListening();
  }
}


void loop(void)
{
  readAndSendData();
  delay(200);
}
