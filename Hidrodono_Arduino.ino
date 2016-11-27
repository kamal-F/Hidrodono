#include "Arduino.h"
#include "LiquidCrystal.h"  //this library is included in the Arduino IDE
#include "MenuBackend.h"    //MenuBackend library - copyright by Alexander Brevig

// Contains EEPROM.read() and EEPROM.write()
#include <EEPROM.h>		//EEPROM library - Christopher Andrews, avid A. Mellis

#include <dht.h>	//DHT Temperature & Humidity Sensor li - Rob Tillaart

#include <SimpleTimer.h>	//A timer library for Arduino - mromani@ottotecnica.com


const int buttonPinLeft = 8;      // pin for the Up button
const int buttonPinRight = 9;    // pin for the Down button
const int buttonPinEsc = 10;     // pin for the Esc button
const int buttonPinEnter = 11;   // pin for the Enter button

const int portFertigation = A4;   // port output for fertigation LOW/HIGH
const int portWater = A5;   // port output for water LOW/HIGH

const long detik = 1000;
const long menit = 60000;	// 60 *1000

int lastButtonPushed = 0;

int lastButtonEnterState = LOW;   // the previous reading from the Enter input pin
int lastButtonEscState = LOW;   // the previous reading from the Esc input pin
int lastButtonLeftState = LOW;   // the previous reading from the Left input pin
int lastButtonRightState = LOW;   // the previous reading from the Right input pin

long lastEnterDebounceTime = 0;  // the last time the output pin was toggled
long lastEscDebounceTime = 0;  // the last time the output pin was toggled
long lastLeftDebounceTime = 0;  // the last time the output pin was toggled
long lastRightDebounceTime = 0;  // the last time the output pin was toggled
long debounceDelay = 500;    // the debounce time

unsigned long previousMillis = 0;

int valWaterInterval = 100;
int valWaterLong = 2;
int valWaterAuto = LOW;

int valFertInterval = 0;
int valFertLong = 1;

int valHumidity = 0;
int valTemperature = 0;

int trHumidity; // treshold Humidity
int trTemperature; // treshold Temperature

//pompa
int pumpWater = LOW;
int pumpFertigation = LOW;

//EEPROM
// ID of the settings block, hidroponik 0
#define CONFIG_VERSION "hp0"
// Tell it where to store your config data in EEPROM
#define CONFIG_START 32

// Example settings structure
struct StoreStruct {
	// The variables of your settings
	//eWI, EEPROM Water Interval
	//eWL, EEPROM Water Long
	//eWA, EEPROM Water Auto
	//eFI, EEPROM Fertigation Interval
	//eFL, EEPROM Fertigation Long
	//tH, EEPROM treshold Humidity
	//tT, EEPROM treshold Temperature

	// This is for mere detection if they are your settings
	char version[4];
	int eWI, eWL, eWA, eFI, eFL, tH, tT;
} storage = {
		CONFIG_VERSION,
		// The default values
		100, 2, 0, 0, 1, 60, 27
};


//DHT11
dht DHT;
#define DHT11_PIN 12


// LiquidCrystal display with:
// rs on pin 7
// rw on gnd
// enable on pin 6
// d4, d5, d6, d7 on pins 5, 4, 3, 2
LiquidCrystal lcd(7, 6, 5, 4, 3, 2);


//Menu variables
MenuBackend menu = MenuBackend(menuUsed,menuChanged);
//initialize menuitems

MenuItem menu1Item1 = MenuItem("1. Air          ");
MenuItem menuItem1SubItem1 = MenuItem("1.1 Interval mnt");
MenuItem menuItem1SubItem2 = MenuItem("1.2 Lamanya mnt ");
MenuItem menuItem1SubItem3 = MenuItem("1.3 Otomatis    ");

MenuItem menu1Item2 = MenuItem("2. Fertigasi    ");
MenuItem menuItem2SubItem1 = MenuItem("2.1 Interval mnt");
MenuItem menuItem2SubItem2 = MenuItem("2.2 Lamanya dtk ");

MenuItem menu1Item3 = MenuItem("3. Treshold sens");
MenuItem menuItem3SubItem1 = MenuItem("3.1 Kelembapan  ");
MenuItem menuItem3SubItem2 = MenuItem("3.2 Temperatur  ");

MenuItem menu1Item4 = MenuItem("4. Info         ");


//menu loop at void loop
unsigned long currentMillis2 = millis();
unsigned long previousMillis2 = 0;
const long simdelay = 2000;

int label1 = 1; //label, 1 = enabled, 0 = disabled
int label2 = 1;
int label3 = 1;
int label4 = 1;

int isRootMenu = 1;

// the timer object, dalam detik, harus dikali 1000
SimpleTimer timer;

int timerFertigation;
int timerWater;
int tlongFertigation;
int tlongWater;

void setup()
{
	//PIN mapping
	pinMode(buttonPinLeft, INPUT);
	pinMode(buttonPinRight, INPUT);
	pinMode(buttonPinEnter, INPUT);
	pinMode(buttonPinEsc, INPUT);

	pinMode(portFertigation, OUTPUT);
	pinMode(portWater, OUTPUT);

	lcd.begin(16, 2);

	//configure menu
	menu.getRoot().add(menu1Item1);
	menu1Item1.addRight(menu1Item2).addRight(menu1Item3).addRight(menu1Item4);
	menu1Item1.add(menuItem1SubItem1).addRight(menuItem1SubItem2).addRight(menuItem1SubItem3);
	menu1Item2.add(menuItem2SubItem1).addRight(menuItem2SubItem2);
	menu1Item3.add(menuItem3SubItem1).addRight(menuItem3SubItem2);

	lcd.setCursor(0,0);
	lcd.print("Hidrodono v.0.1");

	//baca dari ROM
	loadConfig();

	//masukan ke variabel
	insertConfig();

	//Serial.begin(115200);

	//tampil awal
	menu.toRoot();
}

// The loop function is called in an endless loop
void loop()
{
	readButtons();  //I splitted button reading and navigation in two procedures because
	navigateMenus();  //in some situations I want to use the button for other purpose (eg. to change some settings)

	if(isRootMenu==1)menuRoot();

	timer.run();

	//Serial.println(timerFertigation.getNumTimers());

}


void menuChanged(MenuChangeEvent changed){
	MenuItem newMenuItem=changed.to; //get the destination menu

	if(newMenuItem.getName() != "MenuRoot"){

		isRootMenu = 0;

		lcd.setCursor(0,0);
		lcd.print("[Setup]         ");

		lcd.setCursor(0,1); //set the start position for lcd printing to the second row
		lcd.print(newMenuItem.getName());
	}else{
		isRootMenu = 1;
	}
	//Serial.println(newMenuItem.getName());
}

void menuUsed(MenuUseEvent used){

	if(used.item == menuItem1SubItem1){
		menuInterval(valWaterInterval,"Interval menit  ", storage.eWI);
	}else if(used.item == menuItem1SubItem2){
		menuInterval(valWaterLong,"Lamanya menit   ", storage.eWL);
	}else if(used.item == menuItem1SubItem3){
		menuWaterAuto();
	}else if(used.item == menuItem2SubItem1){
		menuInterval(valFertInterval,"F Interval mnt  ", storage.eFI);
	}else if(used.item == menuItem2SubItem2){
		menuInterval(valFertLong,"F Lamanya detik ", storage.eFL);
	}else if(used.item == menuItem3SubItem1){
		menuInterval(trHumidity,"T kelembapan    ", storage.tH);
	}else if(used.item == menuItem3SubItem2){
		menuInterval(trTemperature,"T temperatur C  ", storage.tT);
	}else if(used.item == menu1Item4){
		menuInfo();
	}
}

void readButtons(){  //read buttons status
	int reading;
	int buttonEnterState=LOW;             // the current reading from the Enter input pin
	int buttonEscState=LOW;             // the current reading from the input pin
	int buttonLeftState=LOW;             // the current reading from the input pin
	int buttonRightState=LOW;             // the current reading from the input pin

	//Enter button
	// read the state of the switch into a local variable:
	reading = digitalRead(buttonPinEnter);

	// check to see if you just pressed the enter button
	// (i.e. the input went from LOW to HIGH),  and you've waited
	// long enough since the last press to ignore any noise:

	// If the switch changed, due to noise or pressing:
	if (reading != lastButtonEnterState) {
		// reset the debouncing timer
		lastEnterDebounceTime = millis();
	}

	if ((millis() - lastEnterDebounceTime) > debounceDelay) {
		// whatever the reading is at, it's been there for longer
		// than the debounce delay, so take it as the actual current state:
		buttonEnterState=reading;
		lastEnterDebounceTime=millis();
	}

	// save the reading.  Next time through the loop,
	// it'll be the lastButtonState:
	lastButtonEnterState = reading;


	//Esc button
	// read the state of the switch into a local variable:
	reading = digitalRead(buttonPinEsc);

	// check to see if you just pressed the Down button
	// (i.e. the input went from LOW to HIGH),  and you've waited
	// long enough since the last press to ignore any noise:

	// If the switch changed, due to noise or pressing:
	if (reading != lastButtonEscState) {
		// reset the debouncing timer
		lastEscDebounceTime = millis();
	}

	if ((millis() - lastEscDebounceTime) > debounceDelay) {
		// whatever the reading is at, it's been there for longer
		// than the debounce delay, so take it as the actual current state:
		buttonEscState = reading;
		lastEscDebounceTime=millis();
	}

	// save the reading.  Next time through the loop,
	// it'll be the lastButtonState:
	lastButtonEscState = reading;


	//Down button
	// read the state of the switch into a local variable:
	reading = digitalRead(buttonPinRight);

	// check to see if you just pressed the Down button
	// (i.e. the input went from LOW to HIGH),  and you've waited
	// long enough since the last press to ignore any noise:

	// If the switch changed, due to noise or pressing:
	if (reading != lastButtonRightState) {
		// reset the debouncing timer
		lastRightDebounceTime = millis();
	}

	if ((millis() - lastRightDebounceTime) > debounceDelay) {
		// whatever the reading is at, it's been there for longer
		// than the debounce delay, so take it as the actual current state:
		buttonRightState = reading;
		lastRightDebounceTime =millis();
	}

	// save the reading.  Next time through the loop,
	// it'll be the lastButtonState:
	lastButtonRightState = reading;


	//Up button
	// read the state of the switch into a local variable:
	reading = digitalRead(buttonPinLeft);

	// check to see if you just pressed the Down button
	// (i.e. the input went from LOW to HIGH),  and you've waited
	// long enough since the last press to ignore any noise:

	// If the switch changed, due to noise or pressing:
	if (reading != lastButtonLeftState) {
		// reset the debouncing timer
		lastLeftDebounceTime = millis();
	}

	if ((millis() - lastLeftDebounceTime) > debounceDelay) {
		// whatever the reading is at, it's been there for longer
		// than the debounce delay, so take it as the actual current state:
		buttonLeftState = reading;
		lastLeftDebounceTime=millis();;
	}

	// save the reading.  Next time through the loop,
	// it'll be the lastButtonState:
	lastButtonLeftState = reading;

	//records which button has been pressed
	if (buttonEnterState==HIGH){
		lastButtonPushed=buttonPinEnter;

	}else if(buttonEscState==HIGH){
		lastButtonPushed=buttonPinEsc;

	}else if(buttonRightState==HIGH){
		lastButtonPushed=buttonPinRight;

	}else if(buttonLeftState==HIGH){
		lastButtonPushed=buttonPinLeft;

	}else{
		lastButtonPushed=0;
	}
}

void navigateMenus() {
	MenuItem currentMenu=menu.getCurrent();

	switch (lastButtonPushed){
	case buttonPinEnter:
		if(!(currentMenu.moveDown())){  //if the current menu has a child and has been pressed enter then menu navigate to item below
			menu.use();
		}else{  //otherwise, if menu has no child and has been pressed enter the current menu is used
			menu.moveDown();
		}
		break;
	case buttonPinEsc:
		menu.toRoot();  //back to main
		break;
	case buttonPinRight:
		menu.moveRight();
		break;
	case buttonPinLeft:
		menu.moveLeft();
		break;
	}

	lastButtonPushed=0; //reset the lastButtonPushed variable
}

void menuInterval(int &valx, String description, int &strg){
	int temp = 0;

	temp = valx;

	lcd.clear();

	do {
		readButtons();
		//Serial.println(lastButtonPushed);

		if(lastButtonPushed==buttonPinLeft){
			temp--;
		}else if(lastButtonPushed==buttonPinRight){
			temp++;
		}else if(lastButtonPushed==buttonPinEnter){
			valx = temp;

			//save to ROM
			strg = valx;
			saveConfig();

			// load interval timeout baru
			//baca dari ROM
			loadConfig();

			//masukan ke variabel
			insertConfig();

			lastButtonPushed = buttonPinEsc;
			menu.toRoot();
			break;
		}

		lcd.setCursor(0,0);
		lcd.print(description);
		lcd.setCursor(0,1);
		lcd.print(temp);
		lcd.print("    ");

	} while (lastButtonPushed!=buttonPinEsc);

}

void menuWaterAuto(){
	int temp = LOW;

	temp = valWaterAuto;

	lcd.clear();

	do {
		readButtons();
		//Serial.println(lastButtonPushed);

		if(lastButtonPushed==buttonPinLeft){
			temp = LOW;
		}else if(lastButtonPushed==buttonPinRight){
			temp = HIGH;
		}else if(lastButtonPushed==buttonPinEnter){
			valWaterAuto = temp;

			// save to ROM
			storage.eWA = temp;
			saveConfig();

			lastButtonPushed = buttonPinEsc;
			menu.toRoot();
			break;
		}

		lcd.setCursor(0,0);
		lcd.print("Air otomatis");
		lcd.setCursor(0,1);
		if(temp==LOW){
			lcd.print("tidak");
		}else{
			lcd.print("ya");
		}
		lcd.print("    ");

	} while (lastButtonPushed!=buttonPinEsc);
}

void menuInfo(){
	int lama = 1500;

	lcd.clear();

	lcd.setCursor(0,0);
	lcd.print("**INFO**        ");

	delay(lama);

	lcd.setCursor(0,0);
	lcd.print("Interval air    ");
	lcd.setCursor(0,1);
	lcd.print(valWaterInterval);
	lcd.print(" menit   ");

	delay(lama);

	lcd.setCursor(0,0);
	lcd.print("Lamanya air     ");
	lcd.setCursor(0,1);
	lcd.print(valWaterLong);
	lcd.print(" menit   ");

	delay(lama);

	lcd.setCursor(0,0);
	lcd.print("Otomatis air    ");
	lcd.setCursor(0,1);
	if(valWaterAuto==LOW){
		lcd.print("tidak");
	}else{
		lcd.print("ya");
	}
	lcd.print("     ");

	delay(lama);

	lcd.setCursor(0,0);
	lcd.print("Interval Fertiga");
	lcd.setCursor(0,1);
	lcd.print(valFertInterval);
	lcd.print(" menit  ");

	delay(lama);

	lcd.setCursor(0,0);
	lcd.print("Lamanya Fertigas");
	lcd.setCursor(0,1);
	lcd.print(valFertLong);
	lcd.print(" detik  ");

	delay(lama);

	lcd.setCursor(0,0);
	lcd.print("T kelembapan    ");
	lcd.setCursor(0,1);
	lcd.print(trHumidity);
	lcd.print("     ");

	delay(lama);

	lcd.setCursor(0,0);
	lcd.print("T temperatur    ");
	lcd.setCursor(0,1);
	lcd.print(trTemperature);
	lcd.print(" C   ");

	delay(lama);


	menu.toRoot();
}

void menuRoot(){
	if ((currentMillis2 - previousMillis2 >= simdelay) && label1 ) {
		lcd.setCursor(0,0);
		lcd.print("Politeknik Pos  ");
		lcd.setCursor(0,1);
		lcd.print("Indonesia       ");

		label1 = 0;
	}

	if ((currentMillis2 - previousMillis2 >= simdelay *2) && label2) {
		lcd.setCursor(0,0);
		lcd.print("Tim Hidroponik  ");
		lcd.setCursor(0,1);
		lcd.print("                ");

		label2 = 0;
	}

	if ((currentMillis2 - previousMillis2 >= simdelay *3) && label3) {
		//baca sensor dt11
		DHT.read11(DHT11_PIN);
		valHumidity = DHT.humidity;
		valTemperature =  DHT.temperature;

		if(valWaterAuto == HIGH){
			// disable pump
			timer.disable(timerWater);

			// baca dari sensor, jika kelempbapan kurang dari treshold
			// artinya kekeringan maka aktifkan pompa air

			if(valHumidity <= trHumidity){
				digitalWrite(portWater, HIGH);
			}else{
				digitalWrite(portWater, LOW);
			}
		} else{
			// enable pump
			timer.enable(timerWater);
		}


		lcd.setCursor(0,0);
		lcd.print("Kelembapan: ");
		lcd.print(valHumidity);
		lcd.print("    ");
		lcd.setCursor(0,1);
		lcd.print("Temperatur: ");
		lcd.print(valTemperature);
		lcd.print("    ");

		label3 = 0;
	}

	if ((currentMillis2 - previousMillis2 >= simdelay *4) && label4) {
		previousMillis2 = currentMillis2;

		lcd.setCursor(0,0);
		lcd.print("D IV TI - LPPM  ");
		lcd.setCursor(0,1);
		lcd.print("2016            ");

		//restart label
		label1 = 1;
		label2 = 1;
		label3 = 1;
		label4 = 1;
	}

	currentMillis2 = millis();

	//exit dari menu root, pengalihan ke mode setup
}

void loadConfig() {
	// To make sure there are settings, and they are YOURS!
	// If nothing is found it will use the default settings.
	if (EEPROM.read(CONFIG_START + 0) == CONFIG_VERSION[0] &&
			EEPROM.read(CONFIG_START + 1) == CONFIG_VERSION[1] &&
			EEPROM.read(CONFIG_START + 2) == CONFIG_VERSION[2])
		for (unsigned int t=0; t<sizeof(storage); t++)
			*((char*)&storage + t) = EEPROM.read(CONFIG_START + t);
}

void saveConfig() {
	for (unsigned int t=0; t<sizeof(storage); t++)
		EEPROM.write(CONFIG_START + t, *((char*)&storage + t));
}

// a function to be executed periodically
void doPumpFer() {
	//hidupkan pompa fertigasi
	digitalWrite(portFertigation, HIGH);

	//timeout pompa fertigasi, dalam detik
	tlongFertigation = timer.setTimeout(valFertLong * detik, doPumpFerOff);

}

void doPumpWater(){
	//hidupkan pompa air
	digitalWrite(portWater, HIGH);

	//timeout pompa water, dalam menit
	tlongWater = timer.setTimeout(valWaterLong * menit, doPumpWaterOff);

}

void doPumpFerOff(){
	//matikan pompa fertigasi
	digitalWrite(portFertigation, LOW);
}

void doPumpWaterOff(){
	//matikan pompa air
	digitalWrite(portWater, LOW);
}

// insert config and SimpleTimer
void insertConfig(){

	valWaterInterval = storage.eWI;
	valWaterLong = storage.eWL;
	valWaterAuto = storage.eWA;

	valFertInterval = storage.eFI;
	valFertLong = storage.eFL;

	trHumidity = storage.tH;
	trTemperature = storage.tT;

	//SimpleTimer

	//timerFertigation dalam menit
	timerFertigation = timer.setInterval(valFertInterval * menit, doPumpFer);

	//timerWater dalam menit
	timerWater = timer.setInterval(valWaterInterval * menit, doPumpWater);

	//long fertigation, dalam detik
	tlongFertigation = timer.setTimeout(valFertLong * detik, doPumpFerOff);

	//long water, dalam menit
	tlongWater = timer.setTimeout(valWaterLong * menit, doPumpWaterOff);

}
