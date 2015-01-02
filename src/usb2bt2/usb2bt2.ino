#define SDEBUG

#include <hidboot.h>
#include <usbhub.h>
#include <stdint.h>
#include <SoftwareSerial.h>
#include "spbits.h"
#include "keymap_table.h"

#define LEDPIN 13

#define RXPIN 6
#define TXPIN 7

SoftwareSerial mySerial(RXPIN, TXPIN);

uint8_t spbits=SPBITS_NONE;
uint8_t nmkeys[6]={0};
uint8_t nnmkeys=0;
uint8_t cskeys1=0;

void panic();
void bridge_serial();

void update_bt()
{
#ifdef SDEBUG
	Serial.print("updat_bt: spbits: 0x");
	Serial.println(spbits, HEX);
	Serial.print("update_bt: nnmkeys: ");
	Serial.println(nnmkeys);
	Serial.print("update_bt: nmkeys: ");
	Serial.print(nmkeys[0]);
	Serial.print(" ");
	Serial.print(nmkeys[1]);
	Serial.print(" ");
	Serial.print(nmkeys[2]);
	Serial.print(" ");
	Serial.print(nmkeys[3]);
	Serial.print(" ");
	Serial.print(nmkeys[4]);
	Serial.print(" ");
	Serial.print(nmkeys[5]);
	Serial.println("");
#endif /* SDEBUG */

	mySerial.print((char)0xfd);
	mySerial.print((char)0x09);
	mySerial.print((char)0x01);
	mySerial.print((char)spbits);
	mySerial.print((char)0x00);
	mySerial.print((char)nmkeys[0]);
	mySerial.print((char)nmkeys[1]);
	mySerial.print((char)nmkeys[2]);
	mySerial.print((char)nmkeys[3]);
	mySerial.print((char)nmkeys[4]);
	mySerial.print((char)nmkeys[5]);
}

void update_consumer()
{
	mySerial.print((char)0xfd);
	mySerial.print((char)0x03);
	mySerial.print((char)0x03);
	mySerial.print((char)nmkeys[0]);
	mySerial.print((char)cskeys1);
}

void update_bt_with_key_mapping()
{
	int e, i;
	uint8_t spbits_local, spbits_orig;
	uint8_t nmkeys0_orig;
	uint8_t is_consumer=0;

	if(nmkeys[0]==0){
		update_consumer();
		update_bt();
		return;
	}

	spbits_local=(spbits>>4)|(spbits&0x0f);
	spbits_orig=spbits;
	nmkeys0_orig=nmkeys[0];

	e=(nmkeys[0]+spbits_local)%KEYMAP_HASH_MAX;
	
	for(i=0; i<KEYMAP_NELEMENTS; i++){
		uint8_t v;

		if((read_keymap_table(e, i, 0)==nmkeys[0])&&(((v=read_keymap_table(e, i, 1))&SPBITS_MODIFIER_MASK)==spbits_local)){
			nmkeys[0]=read_keymap_table(e, i, 2);
			if((v&SPBITS_REPORT_MASK)==SPBITS_REPORT_CONSUMER)
				is_consumer=!0;
			else
				is_consumer=0;
			spbits=read_keymap_table(e, i, 3);
			break;
		}
	}

	if(is_consumer){
		cskeys1=spbits;
		update_consumer();
	}else
		update_bt();
	spbits=spbits_orig;
	nmkeys[0]=nmkeys0_orig;
	cskeys1=0;

	return;
}

class KbdRptParser : public KeyboardReportParser
{
	void PrintKey(uint8_t m, uint8_t key);

protected:
	virtual void OnControlKeysChanged(uint8_t before, uint8_t after);
	virtual void OnKeyDown(uint8_t mod, uint8_t key);
	virtual void OnKeyUp(uint8_t mod, uint8_t key);
};

void KbdRptParser::PrintKey(uint8_t m, uint8_t key)
{
	MODIFIERKEYS mod;
	char c=OemToAscii(m, key);

	*((uint8_t*)&mod)=m;

	Serial.print(mod.bmLeftCtrl==1? "C":" ");
	Serial.print(mod.bmLeftShift==1?"S":" ");
	Serial.print(mod.bmLeftAlt==1?  "A":" ");
	Serial.print(mod.bmLeftGUI==1?  "G":" ");

	Serial.print("<");
	PrintHex<uint8_t>(key, 0x80);
	if(c!=0){
		Serial.print("(ascii:");
		Serial.print(c);
		Serial.print(")");
	}
	Serial.print(">");

	Serial.print(mod.bmRightCtrl==1? "C":" ");
	Serial.print(mod.bmRightShift==1?"S":" ");
	Serial.print(mod.bmRightAlt==1?  "A":" ");
	Serial.print(mod.bmRightGUI==1?  "G":" ");

	Serial.println("");
};

void KbdRptParser::OnKeyDown(uint8_t mod, uint8_t key)
{
#ifdef SDEBUG
	Serial.print("DN ");
	PrintKey(mod, key);
#endif /* SDEBUG */

	if(key==0)
		return;

	while(nnmkeys>0)
		nmkeys[--nnmkeys]=0;
	nmkeys[nnmkeys++]=key;

	update_bt_with_key_mapping();
}

void KbdRptParser::OnKeyUp(uint8_t mod, uint8_t key)
{
#ifdef SDEBUG
	Serial.print("UP ");
	PrintKey(mod, key);
#endif /* SDEBUG */

	if(key==0)
		return;

	while(nnmkeys>0)
		nmkeys[--nnmkeys]=0;

	update_bt_with_key_mapping();
}

void KbdRptParser::OnControlKeysChanged(uint8_t before, uint8_t after)
{
	MODIFIERKEYS beforeMod, afterMod;

	*((uint8_t*)&beforeMod)=before;
	*((uint8_t*)&afterMod)=after;

#ifdef SDEBUG
	if(beforeMod.bmLeftCtrl!=afterMod.bmLeftCtrl)
		Serial.println("LeftCtrl changed");
	if(beforeMod.bmLeftShift!=afterMod.bmLeftShift)
		Serial.println("LeftShift changed");
	if(beforeMod.bmLeftAlt!=afterMod.bmLeftAlt)
		Serial.println("LeftAlt changed");
	if(beforeMod.bmLeftGUI!=afterMod.bmLeftGUI)
		Serial.println("LeftGUI changed");

	if(beforeMod.bmRightCtrl!=afterMod.bmRightCtrl)
		Serial.println("RightCtrl changed");
	if(beforeMod.bmRightShift!=afterMod.bmRightShift)
		Serial.println("RightShift changed");
	if(beforeMod.bmRightAlt!=afterMod.bmRightAlt)
		Serial.println("RightAlt changed");
	if(beforeMod.bmRightGUI!=afterMod.bmRightGUI)
		Serial.println("RightGUI changed");
#endif /* SDEBUG */

	spbits=	\
		 (afterMod.bmLeftCtrl?  SPBITS_LCTRL:0)	\
		|(afterMod.bmLeftShift? SPBITS_LSHIFT:0)	\
		|(afterMod.bmLeftAlt?   SPBITS_LSHIFT:0)	\
		|(afterMod.bmLeftGUI?   SPBITS_LGUI:0)	\
		|(afterMod.bmRightCtrl? SPBITS_RCTRL:0)	\
		|(afterMod.bmRightShift?SPBITS_RSHIFT:0)	\
		|(afterMod.bmRightAlt?  SPBITS_RALT:0)	\
		|(afterMod.bmRightGUI?  SPBITS_RGUI:0);

	update_bt_with_key_mapping();
}

USB Usb;
USBHub Hub1(&Usb);
USBHub Hub2(&Usb);
USBHub Hub3(&Usb);
USBHub Hub4(&Usb);
HIDBoot<HID_PROTOCOL_KEYBOARD> HidKeyboard(&Usb);
KbdRptParser Prs;

void setup()
{
	pinMode(LEDPIN, OUTPUT);

#ifdef SDEBUG
	Serial.begin(115200);
	while(!Serial)
		;
#endif /* SDEBUG */
	mySerial.begin(2400);

	if(Usb.Init()==-1){
#ifdef SDEBUG
		Serial.println("error: OSC did not start");
#endif /* SDEBUG */
		panic();
	}
#ifdef SDEBUG
	Serial.println("Usb started");
#endif /* SDEBUG */
	delay(200);

	HidKeyboard.SetReportParser(0, (HIDReportParser*)&Prs);

#ifdef SDEBUG
	Serial.println("End of setup()");
#endif /* SDEBUG */
}

void loop()
{
	Usb.Task();
#ifdef SDEBUG
	bridge_serial();
#endif /* SDEBUG */
}

void panic()
{
	for(;;){
		digitalWrite(LEDPIN, HIGH);
		delay(200);
		digitalWrite(LEDPIN, LOW);
		delay(200);
	}
}

void bridge_serial()
{
	if(mySerial.available())
		Serial.write(mySerial.read());
	if(Serial.available())
		mySerial.write(Serial.read());
}
