/*
My IR Remote codes

PLAY: FFA25D
EQ:	  FF22DD
CH+:  FFE21D
CH-:  FF629D
VOL+: FFC23D
VOL-: FF02FD
0:    FFE01F
PREV: FFA857
NEXT: FF906F
1:    FF6897
2:    FF9867
3:    FFB04F
4:    FF30CF
5:    FF18E7
6:    FF7A85
7:    FF10EF
8:    FF38C7
9:    FF5AA5
PICK SONG: FF42BD
CH SET: FF52AD
*/

#include <IRremote.h>
#include <Wire.h>
#include <LiquidCrystal.h>

LiquidCrystal lcd(12, 11, 10, 2, 8, 7);
int RECV_PIN = 14;
IRrecv irrecv(RECV_PIN);
decode_results results;

byte second, minute, hour, dayOfWeek, dayOfMonth, month, year; // Values to read from DS3231 RTC Module
byte alarmHour = 5;
byte alarmMinute = 0;
bool alarmOn = true;
bool alarmActive = false;
long lastTimeReading = 0;

const int backlightLCD = 9;
const int btnAlarmHour = 3;
const int btnAlarmMinute = 4;
const int btnAlarmToggle = 16;

int ledMosfet = 6;

long alarmTurnedOnAt = 0;
long alarmDuration = 14400000; //4 hours.
long alarmStepTime = 0;

long pushedButtonAt = 0;
long backlightDuration = 3000;
bool backlightOnForButton = false;

int brightness = 0;
int brightnessStep = 51;
int smallBrightnessStep = 10;
bool lightOn = false;
int backlightBrightnessForNight = 25;
int backlightBrightnessForDay = 255;
bool isNight = false;

int nightLightBrightness = 0;


// Variables for debouncing the buttons
int stateBtnAlarmHour = 0;
int stateBtnAlarmMinute = 0;
int stateBtnAlarmToggle = 0;
int lastStateBtnAlarmHour = 0;
int lastStateBtnAlarmMinute = 0;
int lastStateBtnAlarmToggle = 0;
int readingBtnAlarmHour = 0;
int readingBtnAlarmMinute = 0;
int readingBtnAlarmToggle = 0;
long lastDebounceTimeBtnAlarmHour = 0;
long lastDebounceTimeBtnAlarmMinute = 0;
long lastDebounceTimeBtnAlarmToggle = 0;
const long debounceDelay = 50;

#define DS3231_I2C_ADDRESS 0x68
// Convert normal decimal numbers to binary coded decimal
byte decToBcd(byte val)
{
	return((val / 10 * 16) + (val % 10));
}
// Convert binary coded decimal to normal decimal numbers
byte bcdToDec(byte val)
{
	return((val / 16 * 10) + (val % 16));
}

//Defining a special character to use as brightness icon
byte sunCharacter[8] = {
	0b00000,
	0b00100,
	0b10101,
	0b01110,
	0b11011,
	0b01110,
	0b10101,
	0b00100
};

void setup()
{
	Wire.begin();
	irrecv.enableIRIn();
	//Serial.begin(9600);
	pinMode(backlightLCD, OUTPUT);
	pinMode(ledMosfet, OUTPUT);
	pinMode(btnAlarmHour, INPUT);
	pinMode(btnAlarmMinute, INPUT);
	pinMode(btnAlarmToggle, INPUT);

	pinMode(A2, OUTPUT);

	lcd.createChar(0, sunCharacter);
	lcd.begin(16, 2);
	/* set the initial time here:
	   DS3231 seconds, minutes, hours, day(1 for sunday, 2 for monday...), date, month, year
	   Set the time by uncommenting the following line after editing the values and load the sketch on your arduino. Right after that, comment out the line and load the sketch again. */

	   // setDS3231time(00,59,23,1,31,12,16);
}

void loop()
{
	ListenForButtonPress();

	if (millis() - lastTimeReading > 1000) //Once every second
	{
		ReadTime();

		if (alarmOn)
		{
			if (second == 0)
			{
				CheckAlarm();
				if (minute == 0)
				{
					CheckDayOrNight();
				}
			}
		}
		//DisplayTimeAndDateOnSerialMonitor();
		RefreshLCD();
		lastTimeReading = millis();
	}

	if (alarmActive)
	{
		if (millis() - alarmTurnedOnAt < alarmDuration) //For the duration of alarm
		{
			TurnOnBacklight(AppropriateBacklightBrightness());
			if (millis() - alarmStepTime > 10000) //Once in 10 seconds, will  reach full brightness at 42 minutes
			{
				ChangeBrightness(1);
				alarmStepTime = millis();
			}
		}
		else //Once at the end of alarm duration
		{
			brightness = 0;
			SetBrightness(brightness);
			alarmActive = false;
			TurnOffBacklight();
		}
	}

	if (backlightOnForButton)
	{
		if (millis() - pushedButtonAt < backlightDuration) //For the backlight duration
		{
			TurnOnBacklight(AppropriateBacklightBrightness());
		}
		else //Once at the end of backlight duration
		{
			TurnOffBacklight();
			backlightOnForButton = false;
		}
	}

	if (irrecv.decode(&results)) {		//If a button is pressed on the IR remote
		if (results.value == 0xFFA25D) //PLAY button
		{
			LightOn();
			PushedAnyButton();
		}
		if (results.value == 0xFF22DD) //EQ button
		{
			LightOff();
			PushedAnyButton();
		}
		if (results.value == 0xFFE21D) //CH+ button
		{
			ChangeBrightness(brightnessStep);
			PushedAnyButton();
		}
		if (results.value == 0xFF629D) //CH- button
		{
			ChangeBrightness(-brightnessStep);
			PushedAnyButton();
		}
		if (results.value == 0xFFC23D) //VOL+ button
		{
			ChangeBrightness(smallBrightnessStep);
			PushedAnyButton();
		}
		if (results.value == 0xFF02FD) // VOL- button
		{
			ChangeBrightness(-smallBrightnessStep);
			PushedAnyButton();
		}
		if (results.value == 0xFF52AD) //CH SET button
		{
			ChangeBrightness(-1);
			PushedAnyButton();
		}
		if (results.value == 0xFF5AA5) //9 button
		{
			ChangeBrightness(1);
			PushedAnyButton();
		}
		if (results.value == 0xFFA857) //PREV button
		{
			if (backlightOnForButton)
			{
				IncreaseAlarmHour();
				PushedAnyButton();
			}
			else
			{
				PushedAnyButton();
			}
		}
		if (results.value == 0xFF906F) //NEXT button
		{
			if (backlightOnForButton)
			{
				IncreaseAlarmMinute();
				PushedAnyButton();
			}
			else
			{
				PushedAnyButton();
			}
		}
		if (results.value == 0xFFE01F) //0 button
		{
			if (backlightOnForButton)
			{
				ToggleAlarm();
				PushedAnyButton();
			}
			else
			{
				PushedAnyButton();
			}
		}

		if (results.value == 0xFF42BD) //PICK SONG button
		{
			if (backlightOnForButton)
			{
				ToggleLedMosfet();
				PushedAnyButton();
			}
			else
			{
				PushedAnyButton();
			}
		}

		irrecv.resume();
	}
	//delay(100);
}

//TODO
void ToggleLedMosfet() {
	if (ledMosfet == 6)
	{
		ledMosfet = 5;
		analogWrite(6, 0);
		brightness = 0;
	}
	else
	{
		ledMosfet = 6;
		analogWrite(5, 0);
		brightness = 0;
	}
}

void ListenForButtonPress() //https://www.arduino.cc/en/Tutorial/Debounce
{
	readingBtnAlarmHour = digitalRead(btnAlarmHour);
	readingBtnAlarmMinute = digitalRead(btnAlarmMinute);
	readingBtnAlarmToggle = digitalRead(btnAlarmToggle);

	if (readingBtnAlarmHour != lastStateBtnAlarmHour)
	{
		lastDebounceTimeBtnAlarmHour = millis();
	}
	if (readingBtnAlarmMinute != lastStateBtnAlarmMinute)
	{
		lastDebounceTimeBtnAlarmMinute = millis();
	}
	if (readingBtnAlarmToggle != lastStateBtnAlarmToggle)
	{
		lastDebounceTimeBtnAlarmToggle = millis();
	}

	if (millis() - lastDebounceTimeBtnAlarmHour > debounceDelay)
	{
		if (readingBtnAlarmHour != stateBtnAlarmHour)
		{
			stateBtnAlarmHour = readingBtnAlarmHour;
			if (stateBtnAlarmHour == LOW)
			{
				if (backlightOnForButton)
				{
					IncreaseAlarmHour();
					PushedAnyButton();
				}
				else
				{
					PushedAnyButton();
				}
			}
		}
	}
	if (millis() - lastDebounceTimeBtnAlarmMinute > debounceDelay)
	{
		if (readingBtnAlarmMinute != stateBtnAlarmMinute)
		{
			stateBtnAlarmMinute = readingBtnAlarmMinute;
			if (stateBtnAlarmMinute == LOW)
			{
				if (backlightOnForButton)
				{
					IncreaseAlarmMinute();
					PushedAnyButton();
				}
				else
				{
					PushedAnyButton();
				}
			}
		}
	}
	if (millis() - lastDebounceTimeBtnAlarmToggle > debounceDelay)
	{
		if (readingBtnAlarmToggle != stateBtnAlarmToggle)
		{
			stateBtnAlarmToggle = readingBtnAlarmToggle;
			if (stateBtnAlarmToggle == LOW)
			{
				if (backlightOnForButton)
				{
					ToggleAlarm();
					PushedAnyButton();
				}
				else
				{
					PushedAnyButton();
				}
			}
		}
	}
	lastStateBtnAlarmHour = readingBtnAlarmHour;
	lastStateBtnAlarmMinute = readingBtnAlarmMinute;
	lastStateBtnAlarmToggle = readingBtnAlarmToggle;
}

void ReadTime()
{
	// retrieve data from DS3231
	readDS3231time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
}

//void DisplayTimeAndDateOnMonitor()
//{
//	Serial.print(hour, DEC);
//	Serial.print(":");
//	if (minute < 10)
//	{
//		Serial.print("0");
//	}
//	Serial.print(minute, DEC);
//	Serial.print(":");
//	if (second < 10)
//	{
//		Serial.print("0");
//	}
//	Serial.print(second, DEC);
//	Serial.print(" ");
//	Serial.print(dayOfMonth, DEC);
//	Serial.print("/");
//	Serial.print(month, DEC);
//	Serial.print("/");
//	Serial.print(year, DEC);
//	Serial.print(" Day of week: ");
//	switch (dayOfWeek) {
//	case 1:
//		Serial.println("Sunday");
//		break;
//	case 2:
//		Serial.println("Monday");
//		break;
//	case 3:
//		Serial.println("Tuesday");
//		break;
//	case 4:
//		Serial.println("Wednesday");
//		break;
//	case 5:
//		Serial.println("Thursday");
//		break;
//	case 6:
//		Serial.println("Friday");
//		break;
//	case 7:
//		Serial.println("Saturday");
//		break;
//	}
//}

void RefreshLCD()
{
	lcd.clear();
	PrintOnLCD(hour);
	lcd.print(":");
	PrintOnLCD(minute);
	lcd.print(":");
	PrintOnLCD(second);
	lcd.print("  ");
	//if (isNight)
	//{
	//	lcd.print("N");
	//}
	//else
	//{
	//	lcd.print("D");
	//}
	if (alarmActive)
	{
		lcd.print("!");
	}
	else
	{
		lcd.print(" ");
	}
	if (ledMosfet == 6)
	{
		lcd.write(byte(0));
	}
	else
	{
		lcd.print(" ");
	}
	lcd.write(byte(0));
	lcd.print(brightness);

	lcd.setCursor(0, 1);
	lcd.print("Alarm:");
	PrintOnLCD(alarmHour);
	lcd.print(":");
	PrintOnLCD(alarmMinute);
	if (alarmOn)
	{
		lcd.print(" (on)");
	}
	else
	{
		lcd.print("(off)");
	}
}

void PrintOnLCD(byte value)
{
	if (value < 10)
	{
		lcd.print(0);
	}
	lcd.print(value);
}

void CheckAlarm()
{
	if (hour == alarmHour)
	{
		if (minute == alarmMinute)
		{
			Alarm();
		}
	}
}

void Alarm()
{
	alarmActive = true;
	alarmTurnedOnAt = millis();
	alarmStepTime = millis();
}

void IncreaseAlarmHour()
{
	if (alarmHour == 23)
	{
		alarmHour = 0;
	}
	else
	{
		alarmHour++;
	}
	RefreshLCD();
}

void IncreaseAlarmMinute()
{
	if (alarmMinute == 59)
	{
		alarmMinute = 0;
	}
	else
	{
		alarmMinute++;
	}
	RefreshLCD();
}

void ToggleAlarm()
{
	alarmOn = !alarmOn;
	RefreshLCD();
}

void BacklightForButtonPress()
{
	backlightOnForButton = true;
	pushedButtonAt = millis();
}

void ChangeBrightness(int step) {
	brightness += step;
	if (brightness > 255)
	{
		brightness = 255;
	}
	if (brightness < 0)
	{
		brightness = 0;
	}
	SetBrightness(brightness);
}

void SetBrightness(int brightnessValue)
{
	analogWrite(ledMosfet, brightnessValue);
	if (brightnessValue == 0)
	{
		lightOn = false;
	}
	else
	{
		lightOn = true;
	}
}

void LightOn() {
	if (brightness != 255)
	{
		brightness = 255;
		SetBrightness(brightness);
	}
	else
	{
		return;
	}
}

void LightOff() {
	if (brightness != 0)
	{
		brightness = 0;
		SetBrightness(brightness);
	}
	else
	{
		return;
	}
}

void PushedAnyButton()
{
	BacklightForButtonPress();
	if (alarmActive)
	{
		alarmActive = false;
		brightness = 0;
		SetBrightness(brightness);
	}
}

void TurnOnBacklight(int backlightBrightness) {
	analogWrite(backlightLCD, backlightBrightness);
}

void TurnOffBacklight() {
	analogWrite(backlightLCD, 0);
}

int AppropriateBacklightBrightness() {
	if (isNight)
	{
		return backlightBrightnessForNight;
	}
	else
	{
		return backlightBrightnessForDay;
	}
}

void CheckDayOrNight() {
	if ((hour > 21) || (hour >= 0 && hour < 9))
	{
		isNight = true;
	}
	else
	{
		isNight = false;
	}
}

void setDS3231time(byte second, byte minute, byte hour, byte dayOfWeek, byte
	dayOfMonth, byte month, byte year)
{
	// sets time and date data to DS3231
	Wire.beginTransmission(DS3231_I2C_ADDRESS);
	Wire.write(0); // set next input to start at the seconds register
	Wire.write(decToBcd(second)); // set seconds
	Wire.write(decToBcd(minute)); // set minutes
	Wire.write(decToBcd(hour)); // set hours
	Wire.write(decToBcd(dayOfWeek)); // set day of week (1=Sunday, 7=Saturday)
	Wire.write(decToBcd(dayOfMonth)); // set date (1 to 31)
	Wire.write(decToBcd(month)); // set month
	Wire.write(decToBcd(year)); // set year (0 to 99)
	Wire.endTransmission();
}
void readDS3231time(byte *second,
	byte *minute,
	byte *hour,
	byte *dayOfWeek,
	byte *dayOfMonth,
	byte *month,
	byte *year)
{
	Wire.beginTransmission(DS3231_I2C_ADDRESS);
	Wire.write(0); // set DS3231 register pointer to 00h
	Wire.endTransmission();
	Wire.requestFrom(DS3231_I2C_ADDRESS, 7);
	// request seven bytes of data from DS3231 starting from register 00h
	*second = bcdToDec(Wire.read() & 0x7f);
	*minute = bcdToDec(Wire.read());
	*hour = bcdToDec(Wire.read() & 0x3f);
	*dayOfWeek = bcdToDec(Wire.read());
	*dayOfMonth = bcdToDec(Wire.read());
	*month = bcdToDec(Wire.read());
	*year = bcdToDec(Wire.read());
}
