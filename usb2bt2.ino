//#define SDEBUG

#include <hidboot.h>
#include <usbhub.h>
#include <stdint.h>
#include <SoftwareSerial.h>

#define LEDPIN 13

#define RXPIN 6
#define TXPIN 7

SoftwareSerial mySerial(RXPIN, TXPIN);

uint8_t spbits=0;
uint8_t nmkeys[6]={0};
uint8_t nmkeys_elem=0;

void panic();
void bridge_serial();

void update_bt()
{
#ifdef SDEBUG
	Serial.print("updat_bt: spbits: 0x");
	Serial.println(spbits, HEX);
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

	if(nmkeys_elem>5)
		Serial.println("warning: more than 6 keys are pushed at the same time; ignoring the last key");
	else
		nmkeys[nmkeys_elem++]=key;

	update_bt();
}

void KbdRptParser::OnKeyUp(uint8_t mod, uint8_t key)
{
	int i, j;

#ifdef SDEBUG
	Serial.print("UP ");
	PrintKey(mod, key);
#endif /* SDEBUG */

	j=0;
	for(i=0; i<=nmkeys_elem; i++)
		if(nmkeys[i]!=key)
			nmkeys[j++]=nmkeys[i];
	if(i==j)
		Serial.println("warning: unpushed key is released");
	else
		nmkeys[nmkeys_elem--]=0;

	update_bt();
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
		 (afterMod.bmLeftCtrl?  1<<0:0)	\
		|(afterMod.bmLeftShift? 1<<1:0)	\
		|(afterMod.bmLeftAlt?   1<<2:0)	\
		|(afterMod.bmLeftGUI?   1<<3:0)	\
		|(afterMod.bmRightCtrl? 1<<4:0)	\
		|(afterMod.bmRightShift?1<<5:0)	\
		|(afterMod.bmRightAlt?  1<<6:0)	\
		|(afterMod.bmRightGUI?  1<<7:0);

	update_bt();
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

	Serial.begin(115200);
	while(!Serial)
		;
	mySerial.begin(2400);

	if(Usb.Init()==-1){
		Serial.println("error: OSC did not start");
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
