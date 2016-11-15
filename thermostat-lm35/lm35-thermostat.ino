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
} // end setup
// *********************************************************


// *********************************************************
// loop()
void loop() {

  // Hiszterézis pinek olvasásása, hiszterézis beállítása
  byte Hyst = hister_olvas();
  lcd.setCursor(14,2);
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
