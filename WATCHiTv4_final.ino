//"This work is licensed under the Attribution-NonCommercial-ShareAlike 3.0 Unported License.
// To view a copy of the license, visit http://http://creativecommons.org/licenses/by-nc-sa/3.0/ "
//
//  WATCHiTv4_final
//  WATCHiT
//
//  
//  Copyright (c) 2014 NTNU. All rights reserved.
//

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


