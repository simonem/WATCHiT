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
#include <stdarg.h>
#include <SoftwareSerial.h>
#include <Adafruit_GFX.h>
#include <Adafruit_LEDBackpack.h>


#define VERSION "010-JACKET-090913"
#define DEBUG_ON 0

#define VIBRATOR 2
#define SPEAKER 3
#define RFID 4

/* baud settings for serial interfaces */
#define BT_DATARATE		19200
#define RFID_DATARATE	9600

/* message strings */
#define STRING_PERSON	"I rescued someone"
#define STRING_MOOD1	"I'm happy"
#define STRING_MOOD2	"I'm so and so"
#define STRING_MOOD3	"I'm sad"
#define STRING_STEP		"step"

/* RFID tag codes */
#define TAG_PERSON1     "01068DCD6324"
#define TAG_PERSON2     "01068DF26018"
#define TAG_MOOD11		"0101A8903109" // :)
#define TAG_MOOD12		"0101A850E71F" // :)
#define TAG_MOOD21		"0101A85CD723" // :|
#define TAG_MOOD22		"0101ACA32926" // :|
#define TAG_MOOD31		"0101A89CE3D7" // :(
#define TAG_MOOD32		"0101AC9FFBC8" // :(
#define TAG_STEP        "5100FDAB4E49"

/* RFID tag IDs */
#define TAG_PERSON_ID 0
#define TAG_MOOD1_ID  1
#define TAG_MOOD2_ID  2
#define TAG_MOOD3_ID  3
#define TAG_STEP_ID   4

/* 8x8 LED matrix */
Adafruit_8x8matrix matrix = Adafruit_8x8matrix();

/* led matrix smileys */
static const uint8_t PROGMEM
	smile_bmp[] =
  { B00111100, B01000010, B10100101, B10000001,
  	B10100101, B10011001, B01000010, B00111100 },
  	neutral_bmp[] =
  { B00111100, B01000010, B10100101, B10000001,
    B10111101, B10000001, B01000010, B00111100 },
    frown_bmp[] =
  { B00111100, B01000010, B10100101, B10000001,
    B10011001, B10100101, B01000010, B00111100 },
    person_bmp[] =
  { B00000000, B00011000, B00011000, B01111110,
  	B01111110, B00011000, B00011000, B00000000 };


/* Bluetooth		pins 0, 1 (hardware serial) */
/* RFID reader		pins 4, 12 (rx, tx). Read only */
static SoftwareSerial rfid_serial (4, 12);


/* prints formatted output to BT */
void bt_printf(const char *, ...);
/* prints a smiley */
void led_print(const uint8_t *);
/* clear the display */
void led_clear();


void setup() {
	
	Serial.begin(BT_DATARATE); // bluetooth

	rfid_serial.begin(RFID_DATARATE);
	rfid_serial.listen();

 	matrix.begin(0x70); // init. led matrix
	matrix.setRotation(2);
	matrix.clear();
	matrix.writeDisplay();

  	pinMode(2, OUTPUT); // VIBRATION
	pinMode(3, OUTPUT); // BUZZ
	pinMode(4, INPUT);	// RFID

  	if (DEBUG_ON) {
  		bt_printf("[DEBUG] WATCHiT %s online", VERSION);
  	}

  	delay(200);

  	matrix.setTextSize(1);
  	matrix.setTextWrap(false);  // we dont want text to wrap so it scrolls nicely
  	matrix.setTextColor(LED_ON);

  	for (int8_t x=0; x>=-72; x--) {
		matrix.clear();
		matrix.setCursor(x, 0);
		matrix.print(" WATCHiT ");
		matrix.writeDisplay();
		delay(100);
	}

	led_clear();
}


void loop() {

	int buf;
	int pos = 0;
	char tag[12];
	int tag_id = 0;
	boolean reading = false;
	
	tag[0] = '\0'; // clear the buffer
	
	/* read RFID tag */
	while (rfid_serial.available())
	{
		buf = rfid_serial.read();
		if (buf == 2) reading = true;  // tag begin
		if (buf == 3) reading = false; // tag end
		if ((reading == true) && buf!=2 && buf!=10 && buf!=13)
		tag[pos++] = buf;
		delay(1); //sane delay
	}

	/* if we read a total of 12 bytes then we read
	 * the whole tag and we can process the command */
	if (pos == 12)
	{
		tag_id = get_tag_id(tag);
	    if (DEBUG_ON) {
	    	bt_printf("[DEBUG] recognized tag: %s", tag);
	    }

	    switch (tag_id)
	    {
	    	case TAG_PERSON_ID:
	    	{
	    		bt_printf(STRING_PERSON);
	    		led_print(person_bmp);
	    		
	    		doFeedback(300);
	    		delay(300);
	    		doFeedback(300);
	    		delay(300);
	    		doFeedback(300);
	    		
	    		led_clear();

	    	} break;

	    	case TAG_MOOD1_ID: 
	    	{
	    		bt_printf(STRING_MOOD1);
	    		led_print(smile_bmp);
	    		
	    		doFeedback(300);
	    		delay(300);
	    		doFeedback(300);
	    		
	    		led_clear();
	    		
	    	} break;

	    	case TAG_MOOD2_ID: 
	    	{
	    		bt_printf(STRING_MOOD2);
	    		led_print(neutral_bmp);
	    		
	    		doFeedback(300);
	    		delay(300);
	    		doFeedback(900);
	    		
	    		led_clear();
	    		
	    	} break;

	    	case TAG_MOOD3_ID:
	    	{
	    		bt_printf(STRING_MOOD3);
	    		led_print(frown_bmp);
	    		
	    		doFeedback(900);
	    		delay(300);
	    		doFeedback(900);
	    		
	    		led_clear();
	    		
	    	} break;

	    	case TAG_STEP_ID:
	    	{
	    		bt_printf(STRING_STEP);
	    		
	    	} break;
	    }
	}

	// a sane delay
	delay(100);
}

/* prints a formatted string to BT
 * max 64 chars, lines are terminated
 */
void bt_printf(const char *fmt, ...)
{
	char buf[64];
	buf[0] = '\0';
  
	va_list args;
	va_start (args, fmt );
	vsnprintf(buf+strlen(buf), 64, fmt, args);
	va_end (args);
  
	Serial.println(buf);
}

/* prints a smiley on the LED matrix */
void led_print(const uint8_t *smiley)
{
	matrix.clear();
  	matrix.drawBitmap(0, 0, smiley, 8, 8, LED_ON);
  	matrix.writeDisplay();
}

/* clears the LED matrix diplay */
void led_clear()
{
	matrix.clear();
	matrix.writeDisplay();
}

/* identifies a tag
 * returns -1 if tag is not recognized,
 * or tag id otherwise
 */
int get_tag_id(char *tag)
{
	if (!strcmp(tag, TAG_PERSON1) || !strcmp(tag, TAG_PERSON2))
		return TAG_PERSON_ID;
	else if (!strcmp(tag, TAG_MOOD11) || !strcmp(tag, TAG_MOOD12))
		return TAG_MOOD1_ID;
	else if (!strcmp(tag, TAG_MOOD21) || !strcmp(tag, TAG_MOOD22))
		return TAG_MOOD2_ID;
	else if (!strcmp(tag, TAG_MOOD31) || !strcmp(tag, TAG_MOOD32))
		return TAG_MOOD3_ID;
	else if (!strcmp(tag, TAG_STEP))
		return TAG_STEP_ID;
	return -1;
}

void doFeedback(int ms)
{
	digitalWrite(VIBRATOR, HIGH);
	tone(3, 440, ms);
	delay(ms);
	digitalWrite(VIBRATOR, LOW);
	noTone(3); // not really necessary
}
