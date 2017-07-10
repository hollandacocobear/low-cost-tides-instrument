#include <OneWire.h>
#include <SPI.h>              // KOMUNIKASI SD CARD
#include <SD.h>               // SD CARD
#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>
#include <DallasTemperature.h>  // DS18B20 library. By : Miles Burton, Tim Newsome, and Guil Barros
#include <TinyGPS++.h>
#include <math.h>

//inisialisasi LCD 16X2 I2C
#define I2C_ADDR    0x27 // <<----- Add your address here.  Find it from I2C Scanner
#define BACKLIGHT_PIN     3
#define En_pin  2
#define Rw_pin  1
#define Rs_pin  0
#define D4_pin  4
#define D5_pin  5
#define D6_pin  6
#define D7_pin  7
LiquidCrystal_I2C lcd(I2C_ADDR, En_pin, Rw_pin, Rs_pin, D4_pin, D5_pin, D6_pin, D7_pin);

//D18B20
#define ONE_WIRE_BUS 4
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress insideThermometer;

//inisialisasi RTC
RTC_DS1307 rtc;
DateTime nows;

//GPS
TinyGPSPlus gps;

//GLOBAL VARIABEL UNTUK SD CARD
File file;
char str[13];
String filename, nama, tanggal;

//GLOBAL VARIABLE
byte a, i, j;
int waktu;
float tekanan, pressure, depth, suhu, flat, flon, X, GR;
const int numAvg = 10;
long readings = 0;
char tgl[8];
unsigned long start, TIME; // KONSTANTA WAKTU PENERIMAAN DATA KOORDINAT DARI GPS
unsigned int index = 0;
char next_char;

void setup() {
  // sensors initialization
  Serial.begin(9600);
  sensors.begin();      // DS18B20
  lcd.begin(16, 2);
  lcd.setBacklightPin(BACKLIGHT_PIN, POSITIVE);
  lcd.setBacklight(HIGH);
  /*
    lcd.setCursor(0, 0);
    lcd.print(F(" LOW COST TIDES"));
    lcd.setCursor(0, 1);
    lcd.print(F(" MONITORING SYS"));
    delay(500);
    lcd.clear();
    pinMode(10, OUTPUT);
    digitalWrite(10, HIGH);
  */
  //INISIALISASI RTC
  if (! rtc.begin()) {
    lcd.setCursor(0, 0);
    lcd.print(F("RTC ERROR!!!")); //Please run the SetTime
    lcd.setCursor(0, 1);
    lcd.print(F("CONTACT CS"));
    while (1);
  }

  //INIT SD CARD
  lcd.setCursor(0, 0);
  lcd.print(F("- SD CARD INIT -"));
  lcd.setCursor(0, 1);

  if (!SD.begin(10)) {
    lcd.print(F("SD CARD ERROR!!!"));
    return;
  }
  lcd.print(F("SD CARD OK!"));
  delay(500);
  lcd.clear();

  //INISIALISASI GPS - WAKTU TUNGGU MENCARI SATELIT
  lcd.print(F("--- GPS INIT ---"));
  gps.encode(Serial.read());
  if (millis() > 5000 && gps.charsProcessed() < 10)
  {
    for (i = 0; i <= 9; i++)
    {
      lcd.setCursor(0, 1);
      lcd.print(F("NO GPS DETECTED"));
    }
  }
  lcd.setCursor(2, 1);
  lcd.print(F("GPS DETECTED"));
  delay(500);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("TIME INTERVAL"));
  menu(); //pengambilan nilai waktu penyimpanan data

}

void loop() {
  // put your main code here, to run repeatedly:
  unsigned long startTime = 0;

  startTime = millis();
  lcd.clear();
  lcd.setCursor(0, 0);

  //2017/01/01 15:04
  nows = rtc.now();
  lcd.print(nows.year());
  lcd.write('/');
  lcd2digits(nows.month());
  lcd.write('/');
  lcd2digits(nows.day());
  lcd.write(' ');
  lcd2digits(nows.hour());
  lcd.write(':');
  lcd2digits(nows.minute());

  //PENGAMBILAN DATA GPS
  displayInfo();

  //ambil data tekanan
  for (i = 0; i < numAvg; i++) {
    readings += analogRead(A0);
    delay(50);
  }

  //Serial.println(readings); //0 = 54 250 = 998

  // konversi nilai digital menjadi nilai tekanan
  tekanan = ((((float)readings / (float)numAvg) - 54.0) / 944.00) * 250.00;
  sensors.requestTemperatures();


  //penyesuaian nilai tekanan setelah dilakukan kalibrasi
  pressure = tekanan * 0.99957647;
  float dbar = pressure * 0.1;
  //tekanan = ((((float)readings / (float)numAvg)) / 1023.00) * 250.00;
  suhu = sensors.getTempCByIndex(0);

  //fungsi kedalaman
  X = sin(flat / 57.29578);
  X = X * X;
  GR = 9.780318 * (1.0 + (5.2788 * pow(10, -3) + 2.36 * pow(10, -5) * X) * X) + 1.092 * pow(10, -6) * dbar;
  depth = (((-1.82 * pow(10, -15) * dbar + 2.279 * pow(10, -10)) * dbar - 2.2512 * pow(10, -5)) * dbar + 9.72659) * dbar;
  depth = depth / GR;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(flat, 4);
  lcd.print("|");
  lcd.print(flon, 4);
  lcd.print("|");
  lcd.setCursor(0, 1);
  lcd.print(pressure, 2);
  lcd.print("|");

  lcd.print(depth, 2);
  lcd.print("|");
  lcd.print(suhu, 2);


  sprintf(tgl, "%d/", nows.year());
  if (SD.exists(tgl))  {
  }
  else SD.mkdir(tgl);

  //PENGECEKAN FOLDER BULAN
  sprintf(tgl, "%d/%d/", nows.year(), nows.month());
  if (SD.exists(tgl)) {
  }
  else SD.mkdir(tgl);

  //SAVE DATA
  filename = String(nows.year()) + '/' + String(nows.month()) + '/' + String(nows.day()) + ".txt";
  filename.toCharArray(str, 13);
  file = SD.open(str, FILE_WRITE);

  file.print(nows.year());
  file.print('/');
  save2digits(nows.month());
  file.print('/');
  save2digits(nows.day());
  file.print(' ');
  save2digits(nows.hour());
  file.print(':');
  save2digits(nows.minute());
  file.print(':');
  save2digits(nows.second());
  file.print('|');
  file.print(flon, 6);
  file.print('|');
  file.print(flat, 6);
  file.print('|');
  file.print(tekanan);
  file.print('|');
  file.print(pressure);
  file.print('|');
  file.print(depth);
  file.print('|');
  file.print(suhu);

  readings = 0;
  delay(1000);
  lcd.clear();
  TIME = millis() - startTime;
  lcd.print(TIME);
  file.print('|');
  file.println(TIME);
  file.close();
  delay(waktu * 1000 - TIME);

}

void lcd2digits(int number) {
  if (number >= 0 && number < 10) {
    lcd.write('0');
  }
  lcd.print(number);
}

void save2digits(int number)
{ if (number >= 0 && number < 10) {
    file.print('0');
  }
  file.print(number);
}

void menu() {

  file = SD.open("config.txt");
  lcd.setCursor(0, 1);
  if (file) {
    while (file.available())
    {
      next_char = file.read();
      tgl[index++] = next_char;

    }
    file.close();
  }
  else
  { lcd.print("error");
  }

  waktu = atoi(tgl);

  lcd.print(waktu);
  lcd.print(F(" second"));
  delay(500);
}

void displayInfo()
{
  for (start = millis(); millis() - start < 1000;) {
    while (Serial.available()) {
      char g = Serial.read();
      gps.encode(g);
    }
  }

  if (gps.location.isUpdated())
  {
    flat = gps.location.lat();
    flon = gps.location.lng();
  }
  else
  { lcd.print("error");
  }
}
