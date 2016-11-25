
/*
* Arduino Pro Micro
* Board: Arduino Micro
* Port: /dev/ttyACMx
* Programmer: USBasp
*
*
*/



#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <EEPROM.h>


// Beállított idő jelzője EEPROM 0x00h címen
// 0 nincs beállítva, 0xffh beállítva
const byte RTC_ok = 0;

// RTC modul címe
const byte RTC_addr = 0x68;
//RTC modul EEPROM címe
const byte RTC_EEPROM_addr = 0x57;


// AD átalakítás feszültsége. Nem 5 Volt, mert a védő diódán esik pár tized volt,
const int VDD = 469;

// I2C LCD
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

// LCD LED kapcsoló gombja.
const int LCD_BE_GOMB = 4;

// Egygombos ki-be kapcsoló állapotjelzője.
boolean LCD_be = false;

// Relé kimenet.
const int RELE = 5;

// Relé állapotjezője.
boolean RELE_be = false;

// Relé számláló
unsigned long int RELE_szamlalo = 0;

// LM35 analóg pinje.
const int lm35 = A0;

// Hőmérsékletek tömbje.
int meresek[10];

// Átlaghőmérséklet.
float atlaghomerseklet;

// Hiszterézis
float hist = 0.5;

// Hiszterézis DIP kapcsoló pinek
const int HISTPIN1 = 6;
const int HISTPIN2 = 7;

// Bekapcsoláai hőmérséklet
float be_homerseklet = 19.0;

// Mérések száma az átlaghőmérséklet számításához.
byte meresszam = 0;

// Hőmérséklet mérés közötti idő msec-ben. 6000 msec, percenként 10 mérés.
const unsigned long int mereskoz = 6000;

// Relé bekapcsolások közötti minimum idő msec-ben.
// Határesetben a relé ki-be kapcsolgatna, minimum időt kell biztosítani.
// Most ez próbából 1perc (60000msec).
const unsigned long int bekapcsolaskoz = 60000;

// Mérés aktuális időpontja.
unsigned long int cpuido = millis();

// Előző mérés időpontja.
unsigned long int elozo_meres = millis();

// A relé előző bekapcsolásának időpontja.
unsigned long int elozo_bekapcsolas = millis();






// *********************************************************
// setup()
void setup() {
  pinMode(LCD_BE_GOMB, INPUT);
  Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("---Thermostat---");
  lcd.setCursor(0, 1);
  lcd.print("<> triton-dev <>");
  delay(2000);
  lcd.clear();
  for (int i = 0; i < 10; i++) {
    lcd.backlight();
    meres();
    delay(600);
  }
  lcd.noBacklight();
  // RTC beállítása
  if (EEPROM.read(0) == 0) {
    setRTC(00, 49, 21, 5, 24, 11, 16);
    EEPROM.write(0, 255);
  }

} // end setup
// *********************************************************


// *********************************************************
// loop()
void loop() {

  // Hiszterézis pinek olvasásása, hiszterézis beállítása
  byte Hyst = hister_olvas();
  lcd.setCursor(14, 2);
  lcd.print("H");
  lcd.print(Hyst);

  // LCD háttérfény bekapcsolása
  if (digitalRead(LCD_BE_GOMB) == HIGH && !LCD_be) {
    LCD_be = true;
    lcd.backlight();
    delay(250); // Várakozás, hogy ne kapcsolgasson ki-be az LCD LED-je.
  }

  // LCD háttérfény kikapcsolása
  if (digitalRead(LCD_BE_GOMB) == HIGH && LCD_be) {
    LCD_be = false;
    lcd.noBacklight();
    delay(250); // Várakozás, hogy ne kapcsolgasson ki-be az LCD LED-je.
  }

  // Mérés, ha az előző mérés óta mereskoz idő eltelt.
  cpuido = millis();
  if (cpuido - elozo_meres >= mereskoz) {
    elozo_meres = cpuido;
    meres();
  }

  // Relé bekapcsolása, ha az átlaghőmérséklet hiszterézissel alacsonyabb,
  // mint a minimum hőmérséklet és még a relé nincs bekapcsolva.
  if ((int)atlaghomerseklet * 100 != (int)be_homerseklet * 100) {
    if ((atlaghomerseklet - hist < be_homerseklet) && !RELE_be) {
      RELE_be = true;
      RELE_szamlalo++;
      digitalWrite(RELE, HIGH);
    }

    // Relé elengedése, ha az átlaghőmérséklet  eléri, vagy meghaladja
    //  a bekapcsolási hőmérsékletet.
    if ((atlaghomerseklet > be_homerseklet) && (cpuido - elozo_bekapcsolas >= bekapcsolaskoz)) {
      elozo_bekapcsolas = millis();
      RELE_be = false;
      digitalWrite(RELE, LOW);
    }
  }

  if (RELE_be) {
    lcd.setCursor(0, 1);
    lcd.print("I");
  }
  else {
    lcd.setCursor(0, 1);
    lcd.print("0");
  }
  lcd.setCursor(2, 1);
  lcd.print(RELE_szamlalo);
  // Soros portra irja az állapotot.
  Serial.print(atlaghomerseklet);
  Serial.print(" ");
  Serial.println(RELE_be);

  RTCido();

} //end loop
// *********************************************************


// *********************************************************
// Funkciók:

// Mérés
void meres() {
  // Mérés.
  int ertek = analogRead(lm35);
  meresek[meresszam] = ertek;
  lcd.setCursor(0, 0);
  lcd.print(meresszam);
  lcd.write(byte(B01111110));
  lcd.print(ertek * VDD / 1023.0, 1);
  meresszam++;
  // 10 mérés átlagának számítása.
  if (meresszam == 10) {
    meresszam = 0;
    int atlagol = 0;
    for (int i = 0; i < 10; i++) {
      atlagol += meresek[i];
    }
    atlaghomerseklet = atlagol / 10.0 * VDD / 1024;
    lcd.print(" ");
    lcd.print(atlaghomerseklet, 1);
    lcd.print(" ");
    lcd.print(be_homerseklet, 1);
  }
} // Mérés vége.

// Hiszterézis beolvasása a HISTPINx alapján
//
byte hister_olvas() {
  byte h = 0;
  if (digitalRead(HISTPIN1)) {
    h += 2;
  }
  if (digitalRead(HISTPIN2)) {
    h += 1;
  }
  return h;
} // Hiszterézis olvasás vége.

// Konverziók óra kezeléshez
byte dec2bcd (byte val) {
  return ( (val / 10 * 16) + (val % 10));
}
byte bcd2dec (byte val) {
  return ( (val / 16 * 10) + (val % 16) );
}
// RTC beállítása
void setRTC(byte second, byte minute, byte hour, byte dayOfWeek, byte
            dayOfMonth, byte month, byte year)
{
  // DS3231 óra és dátum beállítása
  Wire.beginTransmission(RTC_addr);
  Wire.write(0); // Másodperc regiszter címe
  Wire.write(dec2bcd(second)); // másodperc
  Wire.write(dec2bcd(minute)); // perc
  Wire.write(dec2bcd(hour)); // óra
  Wire.write(dec2bcd(dayOfWeek)); // nap száma (1=Vasárnap, 7=Szombat)
  Wire.write(dec2bcd(dayOfMonth)); // nap (1 to 31)
  Wire.write(dec2bcd(month)); // hónap
  Wire.write(dec2bcd(year)); // év (0 to 99)
  Wire.endTransmission();
}
// RTC olvasása
void getRTC(byte *second, byte *minute, byte *hour, byte *dayOfWeek, byte *dayOfMonth, byte *month, byte *year) {
  Wire.beginTransmission(RTC_addr);
  Wire.write(0); // DS3231 másodperc címe
  Wire.endTransmission();
  Wire.requestFrom(RTC_addr, 7);
  // 7 bájt olvasása DS3231-ről 00h címtől kezdve
  *second = bcd2dec(Wire.read() & 0x7f);
  *minute = bcd2dec(Wire.read());
  *hour = bcd2dec(Wire.read() & 0x3f);
  *dayOfWeek = bcd2dec(Wire.read());
  *dayOfMonth = bcd2dec(Wire.read());
  *month = bcd2dec(Wire.read());
  *year = bcd2dec(Wire.read());
}
// RTC kijelzése
void RTCido()
{
  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
  // adat olvasása RTC-ről
  getRTC (&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
  //
  Serial.print(hour, DEC);
  Serial.print(":");
  if (minute < 10)
  {
    Serial.print("0");
  }
  Serial.print(minute, DEC);
  Serial.print(":");
  if (second < 10)
  {
    Serial.print("0");
  }
  Serial.print(second, DEC);
  Serial.print(" 20");
  Serial.print(year, DEC);
  Serial.print(".");
  Serial.print(month, DEC);
  Serial.print(".");
  Serial.print(dayOfMonth, DEC);
  Serial.print(", ");
  switch (dayOfWeek) {
    case 1:
      Serial.print("V");
      break;
    case 2:
      Serial.print("H");
      break;
    case 3:
      Serial.print("K");
      break;
    case 4:
      Serial.print("Sze");
      break;
    case 5:
      Serial.print("Cs");
      break;
    case 6:
      Serial.print("P");
      break;
    case 7:
      Serial.print("Szo");
      break;
  }
  Serial.print(" ");
}
