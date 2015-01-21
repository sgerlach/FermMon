// Main Temperature Controller
// Sample the temps and either turn on or turn off
// relays for power to cooler/heater
// write data back out serial ports to
// mysql database (or maybe ES....)


#include <OneWire.h>
#include <DallasTemperature.h>
#include <FastIO.h>
#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>


//Ferment Schedule and temps
#define INIT_TEMP 68.0
#define MAX_TEMP 72.0
#define FERM_DAYS 14
#define COOLER_PIN 6
#define HEATER_PIN 7


///Pre setup stuff
// Pin for temp sensors
#define ONE_WIRE_BUS 10
#define TEMP_PRECISION 10

//One Wire device instance
OneWire oneWire(ONE_WIRE_BUS);

//Pass the OneWire Ref to DallasTemp
DallasTemperature sensors(&oneWire);

//Setup the LCD
#define I2C_ADDR    0x3F  // Define I2C Address where the SainSmart LCD is
#define BACKLIGHT_PIN     3
#define En_pin  2
#define Rw_pin  1
#define Rs_pin  0
#define D4_pin  4
#define D5_pin  5
#define D6_pin  6
#define D7_pin  7
LiquidCrystal_I2C lcd(I2C_ADDR,En_pin,Rw_pin,Rs_pin,D4_pin,D5_pin,D6_pin,D7_pin);

//Array for the device addresses
DeviceAddress beerProbe = { 0x28, 0x5B, 0x0A, 0xFB, 0x05, 0x00, 0x00, 0xA5 };
DeviceAddress chamberProbe = { 0x28, 0xDE, 0x70, 0xFA, 0x05, 0x00, 0x00, 0xFB };

//Arduino Pins for output
int coolRelay = COOLER_PIN;
int heatRelay = HEATER_PIN;
int led = 13;


//Globals
float beerTemp = 68.0;
float chamberTemp = 68.0;
float initTemp = INIT_TEMP;
float maxTemp = MAX_TEMP;
int fermDays = FERM_DAYS;
boolean coolerOn = false;
boolean heaterOn = false;

void setup(void) {
  //Start searil ports
  Serial.begin(9600);
  //Serial.println("Setup complete. Serial open and libs setup");

  //Start up sensors
  sensors.begin();
  sensors.setResolution(beerProbe, TEMP_PRECISION);
  sensors.setResolution(chamberProbe, TEMP_PRECISION);
  
  //set up the lcd
  lcd.begin(20,4);
  lcd.setBacklightPin(BACKLIGHT_PIN,POSITIVE);
  lcd.setBacklight(HIGH);
  setupLCD();
  
  // Set the pins for output
  pinMode(coolRelay, OUTPUT);
  pinMode(heatRelay, OUTPUT);
  pinMode(led, OUTPUT);

}

void loop(void) {
  delay(5000);
  //Serial.println("Requesting Temps");
  sensors.requestTemperatures();
  beerTemp = beerTemp + (readTemps("Beer", beerProbe) / 4.0) - (beerTemp / 4.0);
  chamberTemp = chamberTemp + (readTemps("Chamber", chamberProbe) / 4.0) - (chamberTemp / 4.0) ;
  checkCooler();
  checkHeater();
  writeDataLCD(initTemp, beerTemp, chamberTemp, coolerOn, heaterOn);
  writeDataSerial();
  
  //Serial.println("DONE");

  

}

float readTemps(String device, DeviceAddress deviceAddress) {
 float tempC = sensors.getTempC(deviceAddress);
 float tempF = 0.0;
 if (tempC == -167) {
   writeMessageLCD("Error getting temps");
 } else {
   tempF = DallasTemperature::toFahrenheit(tempC);
   //Serial.print(device +" Temp F:");
   //Serial.print(tempF);
   //Serial.print("\r\n");
 }
 return tempF;
}

void checkCooler(void) {
  // If temp is < .5 F above target and relay is on, turn off
  // Log that we turned off
  float diff = beerTemp-initTemp;
  //Serial.println("Cooler: Beer Temp - initTemp = " + String(diff));
  if ( beerTemp - initTemp < .5  && coolerOn ) {
    writeMessageLCD("Temp Correct");
    digitalWrite(coolRelay, LOW);
    coolerOn = !coolerOn;
  }

  // If temp is > .6 above target and realy is off, turn on
  // Log that we turned on
  if (beerTemp - initTemp > .6 && !coolerOn) {
    writeMessageLCD("Temp high");
    digitalWrite(coolRelay, HIGH);
    coolerOn = !coolerOn;
  }
}

void checkHeater(void) {
  // If temp is < .5 F below target and relay is on, turn off
  // Log that we turned off
  float diff = beerTemp-initTemp;
  //Serial.println("Heater: Beer Temp - initTemp = " + String(diff));
  if ( initTemp - beerTemp < .5  && heaterOn ) {
    writeMessageLCD("Temp Correct");
    digitalWrite(heatRelay, LOW);
    heaterOn = !heaterOn;
  }

  // If temp is > .6 above target and realy is off, turn on
  // Log that we turned on
  if (initTemp - beerTemp  > .6 && !heaterOn) {
    writeMessageLCD("Temp Low");
    digitalWrite(heatRelay, HIGH);
    heaterOn = ! heaterOn;
  }
}

void writeDataSerial(void) {
  // Writing out to serial connection should be JSON data
  // {date: datestamp, beerTemp: [data in F], chamberTemp: [data in F], coolerState: [0/1], heaterState: [0/1]}
  String message = "{";
  message += "\"beerTemp\": \"" + String(beerTemp) + "\", ";
  message += "\"chamberTemp\": \"" + String(chamberTemp) + "\", ";
  message += "\"coolerState\": \"" + String(coolerOn) + "\", ";
  message += "\"heaterState\": \"" + String(heaterOn) + "\"";
  message +=  "}";
  Serial.println(message);
}

void setupLCD(void) {
  // Going to write the following to the LCD
  // Set For: 68.0F
  // Beer:68.0F Cool:On/Off
  // Cham:68.0F Heat:On/Off
  // Msg: Ohh my god. Can't wait for beer
  lcd.setCursor(0,0);
  lcd.print("Set For:");
  lcd.setCursor(0,1);
  lcd.print("Beer:");
  lcd.setCursor(12,1);
  lcd.print("Cool:");
  lcd.setCursor(0,2);
  lcd.print("Cham:");
  lcd.setCursor(12, 2);
  lcd.print("Heat:");
  lcd.setCursor(0,3);
  lcd.print("Msg:");
}

void writeDataLCD(float setTemp, float beerTemp, float chamTemp, bool coolerState, bool heaterState) {
  // Going to write the following to the LCD
  // Set For: 68.0F
  // Beer:68.0F Cool:On/Off
  // Cham:68.0F Heat:On/Off
  // Msg: Ohh my god. Can't wait for beer
  String message = "";
  lcd.setCursor(8,0);
  message = String(setTemp) + "F";
  lcd.print(message);
  lcd.setCursor(5,1);
  message = String(beerTemp)+ "F";
  lcd.print(message);
  lcd.setCursor(17,1);
  message = String(coolerState);
  lcd.print(message);
  lcd.setCursor(5,2);
  message = String(chamTemp) + "F";
  lcd.print(message);
  lcd.setCursor(17,2);
  message = String(heaterState);
  lcd.print(message);
}

void writeMessageLCD(String message) {
 int i;
 int msgLen = sizeof(message);
 String msgPadding = "";
 for (i = 1; i < (14 - sizeof(message)); i++) {
   msgPadding += " " ;
 }
 message += msgPadding;
 lcd.setCursor(4,3);
 lcd.print(message);
}
