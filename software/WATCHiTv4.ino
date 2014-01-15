/**
 * WATCHiTv31 (JACKET) Arduino code
 *
 * Copyright (c) 2013, Emanuele Di Santo, Simone Mora
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met: 
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution. 

 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include <Wire.h>
#include <PN532_I2C.h>
#include <PN532.h>
#include <NfcAdapter.h>

PN532_I2C pn532_i2c(Wire);
NfcAdapter nfc = NfcAdapter(pn532_i2c);

#define OFF      0
#define ON       1
#define TOGGLE   2

#define LED_RED     0
#define LED_GREEN   1

static boolean battery_low = false;

void setup() {

  led_setup();
  vibrator_setup();
  
  set_led(LED_RED, ON);

  nfc.begin();
  Serial1.begin(38400);

  while (!Serial1) { ; }
  delay(100);
}

/* this is called every 5 seconds so variable allocation
 * optimizations are not worth it */
void loop() {

  float vcc = readVcc();
  if (vcc >= 3.6) {
    set_led(LED_RED, TOGGLE);
    battery_low = false;
  } else {
    if (battery_low == false) {
      set_led(LED_RED, ON);
      battery_low = true;
    }
  }
  
  //Serial1.println(vcc);

  if (nfc.tagPresent())
  {
    NfcTag tag = nfc.read();
    if (tag.hasNdefMessage())
    {
      NdefMessage message = tag.getNdefMessage();
      NdefRecord record = message.getRecord(0);

      int payloadLength = record.getPayloadLength();
      byte payload[payloadLength];
      record.getPayload(payload);

      String payloadAsString = "";
      for (int c = 0; c < payloadLength; c++) {
        payloadAsString += (char)payload[c];
      }

      Serial1.print(payloadAsString);
      
      set_vibrator(ON);
      delay(1000);
      set_vibrator(OFF);
    }
  }
}

void led_setup() {
  //DDRB |= 0x80;
  DDRB |= 0x01;
}

void vibrator_setup() {
  DDRF |= 0x01;	
  DDRB |= 0x04;
  // once and for all
  PORTB |= 0x04;
}

// leds have inverted logic (on when pulled low)
void set_led(int led, int state) {

  unsigned char ledBit;
  if (led == LED_RED)    ledBit = 0x01;
  if (led == LED_GREEN)  ledBit = 0x80;

  if (state == OFF) {
    PORTB |= ledBit;
  }
  else if (state == ON) {
    PORTB &= ~(ledBit);
  }
  else if (state == TOGGLE) {
    PORTB ^= ledBit;
  }
}

void set_vibrator(int state) {
  if (state == OFF) {
    PORTF &= ~(0x01);
    //PORTB |= 0x04;
  }
  else {
    PORTF |= 0x01;
    //PORTB |= 0x04;
  }
}

float readVcc()
{
  float vcc = analogRead(A4);
  vcc = vcc/1023.0*3.3*2;
  return vcc;
}


