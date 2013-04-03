/**
 * WATCHiTv3 Arduino code
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

#include <stdarg.h>
#include <SoftwareSerial.h>

#define VERSION "060-NOGPS"

/* change accordingly to prototype
 * when uploading the sketch */
#define _PROTOTYPE_1
#define DEBUG 0

// pins
#define LED_BATT	2
#define LED_CACHE	4
#define LED_FIX		6
#define VIBRATOR	A3

/* the following acted like a switch
 * to disable ping messages and save battery */
//#define PING_S		8
//#define PING_D		9

// message types
#define MSG_PING  -1
#define MSG_PERSON 0
#define MSG_MOOD1  1
#define MSG_MOOD2  2
#define MSG_MOOD3  3

// RFID tag identifiers
#define TAG_PERSON_ID	0
#define TAG_MOOD1_ID	1
#define TAG_MOOD2_ID	2
#define TAG_MOOD3_ID	3
#define TAG_TEST_ID	254
#define TAG_FLUSH_ID	255

// RFID tag serials
#define TAG_PERSON1     "01068DCD6324"
#define TAG_PERSON2     "01068DF26018"
#define TAG_MOOD11	"0101A8903109" // :)
#define TAG_MOOD12	"0101A850E71F" // :)
#define TAG_MOOD21	"0101A85CD723" // :|
#define TAG_MOOD22	"0101ACA32926" // :|
#define TAG_MOOD31	"0101A89CE3D7" // :(
#define TAG_MOOD32	"0101AC9FFBC8" // :(


// misc
#define PING_INTERVAL 30000
#define MAX_CACHED_MSGS 15
#define MSG_LENGTH 64

#if defined (_PROTOTYPE_1)
  #define PROTOTYPE "1"
  #define PERSON_STRING "I rescued someone"
  #define MOOD1_STRING	"I'm happy"
  #define MOOD2_STRING	"I'm so and so"
  #define MOOD3_STRING	"I'm sad"
#endif


// serial interfaces to xbee and rfid
static SoftwareSerial xbee_serial (9, 10); // sends
static SoftwareSerial rfid_serial (11, 8); // receives

// sends out a message via xbee
void	send_msg(int );

// print formatted output to xbee
void xbee_printf(const char *, ...);

// misc
int get_tag_id(char *);
void vibrate(int );
long readVcc();


void setup()
{
	// GPS serial interface (receives)
	Serial.begin(9600);
	rfid_serial.begin(9600);
	xbee_serial.begin(19200);
        
        //xbee_serial.println("Hello");
	if(DEBUG) xbee_printf("[DEBUG] WATCHiT v%s online", VERSION);
	
	// be sure to listen from this interface
	rfid_serial.listen();
  
	//Initiate LEDs
	pinMode(LED_BATT,	OUTPUT);
	pinMode(LED_CACHE,	OUTPUT);
	pinMode(LED_FIX,	OUTPUT);
	
	/* the vibration motor is driven by a PNP
	 * transistor, thus has an 'inverse' logic:
	 * it vibrates when the pin is LOW
	 */
	pinMode(VIBRATOR, OUTPUT);
	digitalWrite(VIBRATOR, HIGH);
	
	/* these pins acted as a PING msg switch
	 * pinMode(PING_S, OUTPUT);
	 * pinMode(PING_D, INPUT);
	 * digitalWrite(PING_S, LOW);
	 * digitalWrite(PING_D, HIGH);
	 */
}

void loop()
{
	char tag[12];
	char eeprom_buf[64];
	int buf, pos = 0, tag_id = 0;
	boolean reading = false;
	static unsigned long last_ping;
	static boolean valid_fix = false;
  
	// clear the buffers
	tag[0] = '\0';
	eeprom_buf[0] = '\0';

  
	// read RFID tag
	while (rfid_serial.available())
	{
		buf = rfid_serial.read();
		if (buf == 2) reading = true; // tag begin
		if (buf == 3) reading = false;// tag end
		if ((reading == true) && buf!=2 && buf!=10 && buf!=13)
		tag[pos++] = buf;
		delay(1);
	}

	/* if we read a total of 12 bytes then we read
	 * the whole tag and we can process the command */
	if (pos == 12)
	{
		tag_id = get_tag_id(tag);
		if(DEBUG) xbee_printf("[DEBUG] Recognized tag %d : %s", tag_id, tag);
		switch (tag_id)
		{
			case TAG_PERSON_ID:	send_msg(MSG_PERSON); break;
			case TAG_MOOD1_ID:	send_msg(MSG_MOOD1); break;
			case TAG_MOOD2_ID:	send_msg(MSG_MOOD2); break;
			case TAG_MOOD3_ID:	send_msg(MSG_MOOD3); break;
		}
		vibrate(tag_id);
	}	
	// check if the battery is depleting
	if (readVcc() < 3560)
		digitalWrite(LED_BATT, HIGH);
}

// sends a message via xbee
void send_msg(int msg_type)
{

	char buf[64];
	const char *msg;

		switch (msg_type)
		{	   
			case (MSG_PERSON)	: msg = PERSON_STRING; break;
			case (MSG_MOOD1)	: msg = MOOD1_STRING; break;
			case (MSG_MOOD2)	: msg = MOOD2_STRING; break;
			case (MSG_MOOD3)	: msg = MOOD3_STRING; break;
		}

		// format our msg string
		sprintf(buf, msg);

		// print formatted output to xbee
		xbee_printf(msg);

	}






// prints a formatted string to xbee (max 64 chars)
void xbee_printf(const char *fmt, ...)
{
	char buf[64];
	buf[0] = '\0';
  
	va_list args;
	va_start (args, fmt );
	vsnprintf(buf+strlen(buf), 64, fmt, args);
	va_end (args);
  
	xbee_serial.println(buf);
}

/* returns -1 if tag is not recognized,
 * or tag id otherwise */
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
	return -1;
}

/* vibrates the vibration module
 * the module has inverse logic (PNP) */
void vibrate(int event)
{
	switch (event)
	{
		case TAG_PERSON_ID: {
			for (int i = 0; i < 3; i++)
			{
				digitalWrite(VIBRATOR, LOW);
				delay(300);
				digitalWrite(VIBRATOR, HIGH);
				delay(300);
			}
		} break;
		case TAG_MOOD1_ID: {
			for (int i = 0; i < 2; i++)
			{
				digitalWrite(VIBRATOR, LOW);
				delay(300);
				digitalWrite(VIBRATOR, HIGH);
				delay(300);
			}
		} break;
		case TAG_MOOD2_ID: {
			digitalWrite(VIBRATOR, LOW);
			delay(300);
			digitalWrite(VIBRATOR, HIGH);
			delay(300);
			digitalWrite(VIBRATOR, LOW);
			delay(900);
			digitalWrite(VIBRATOR, HIGH);
		} break;
		case TAG_MOOD3_ID: {
			digitalWrite(VIBRATOR, LOW);
			delay(900);
			digitalWrite(VIBRATOR, HIGH);
			delay(300);
			digitalWrite(VIBRATOR, LOW);
			delay(900);
			digitalWrite(VIBRATOR, HIGH);
		} break;
	}
}

/* taken somewhere on the net
 * returns Vcc in millivolts */
long readVcc()
{
	long result = 0;
	/* read 1.1V reference against AVcc set the reference
	 * to Vcc and the measurement to the internal 1.1V reference */
	#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
	ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
	#elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
	ADMUX = _BV(MUX5) | _BV(MUX0);
	#elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
	ADMUX = _BV(MUX3) | _BV(MUX2);
	#else
	ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
	#endif
	delay(2); // wait for Vref to settle
	ADCSRA |= _BV(ADSC); // start conversion
	while (bit_is_set(ADCSRA,ADSC)); // measuring
	uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH	 
	uint8_t high = ADCH; // unlocks both
	result = (high<<8) | low;
	// calculate Vcc (in mV); 1125300 = 1.1*1023*1000
	result = 1125300L / result; 
	return result;
}
