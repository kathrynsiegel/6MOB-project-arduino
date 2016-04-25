#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

char msg[32];
int messageLength = 32;

// Initialize RF radio
RF24 radio(9, 10);

const uint64_t pipe = 0xE8E8F0F0E1LL;

// TODO: Incorporate acks, will need to use all channels
const uint64_t pipes[6] = { 0xABCDABCD71LL,
                            0x544d52687CLL,
                            0x544d52687CAA,
                            0x544d52687CBB,
                            0x544d52687CCC,
                            0x544d52687CDD
                          };

void setup(void) {
  Serial.begin(9600);
  radio.begin();
  radio.openReadingPipe(1, pipe);
  radio.startListening();
  Serial.println("Start receiving!");
}

void loop(void) {
  if (radio.available()) {
    bool done = false;
    while (!done) {
      done = radio.read(msg, messageLength);
      delay(50);
    }
    Serial.println(msg);
  }
}