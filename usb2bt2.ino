#include <hidboot.h>
#include <usbhub.h>
#include <stdint.h>
#include <SoftwareSerial.h>

#define LEDPIN 13

#define RXPIN 6
#define TXPIN 7

SoftwareSerial mySerial(RXPIN, TXPIN);

uint8_t spbits=0;
uint8_t nmbits[6]={0};
uint8_t nmbits_elem=0;

void panic();
void bridge_serial();

void update_bt()
{
	Serial.print("updat_bt: spbits: 0x");
	Serial.println(spbits, HEX);
	Serial.print("update_bt: nmbits: ");
	Serial.print(nmbits[0]);
	Serial.print(" ");
	Serial.print(nmbits[1]);
	Serial.print(" ");
	Serial.print(nmbits[2]);
	Serial.print(" ");
	Serial.print(nmbits[3]);
	Serial.print(" ");
	Serial.print(nmbits[4]);
	Serial.print(" ");
	Serial.print(nmbits[5]);
	Serial.println("");

	mySerial.print((char)0xfd);
	mySerial.print((char)0x09);
	mySerial.print((char)0x01);
	mySerial.print((char)spbits);
	mySerial.print((char)0x00);
	mySerial.print((char)nmbits[0]);
	mySerial.print((char)nmbits[1]);
	mySerial.print((char)nmbits[2]);
	mySerial.print((char)nmbits[3]);
	mySerial.print((char)nmbits[4]);
	mySerial.print((char)nmbits[5]);
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
	Serial.print("DN ");
	PrintKey(mod, key);

	if(nmbits_elem==5)
		Serial.println("warning: more than 6 keys are pushed at the same time");
	else
		nmbits[nmbits_elem++]=key;

	update_bt();
}

void KbdRptParser::OnKeyUp(uint8_t mod, uint8_t key)
{
	int i, j;

	Serial.print("UP ");
	PrintKey(mod, key);

	j=0;
	for(i=0; i<=nmbits_elem; i++)
		if(nmbits[i]!=key)
			nmbits[j++]=nmbits[i];
	if(i==j)
		Serial.println("warning: unpushed key is released");
	else
		nmbits[nmbits_elem--]=0;

	update_bt();
}

void KbdRptParser::OnControlKeysChanged(uint8_t before, uint8_t after)
{
	MODIFIERKEYS beforeMod, afterMod;

	*((uint8_t*)&beforeMod)=before;
	*((uint8_t*)&afterMod)=after;

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
	Serial.println("Usb started");
	delay(200);

	HidKeyboard.SetReportParser(0, (HIDReportParser*)&Prs);

	Serial.println("End of setup()");
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
