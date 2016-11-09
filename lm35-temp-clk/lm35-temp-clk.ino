#include <LiquidCrystal_I2C.h>
#include <Wire.h>


LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);


int lm35;
float temperature;
int backlightswitch = 4;
float avgtemp[10];

int VDD = 461; // Vin -V diode

float HYST = 0.5;
float HEATON = 27;
int MINTEMP = 18;
boolean is_heating = false;
boolean prev_is_heating = false;


unsigned long int heatcounter = 0;

unsigned long int counter = 0;
unsigned long prevtimestamp = 0;
unsigned long currtimestamp;
unsigned long backlightprevtimestamp = 0;
unsigned long backlightcurrtimestamp;
long interval = 15 * 1000; // #interval*1000 millisec delay for temperature read
long newinterval = interval;
long backlightinterval = 86400 * 1000; // #backlightinterval*1000 millisec delay for backlight
boolean backlight = false;
int avgtemper = 0;


// Heat on
#define HEAT_ON 0
byte heaton[8] = {4, 31, 0, 10, 21, 0, 31, 4};

// Heat off
#define HEAT_OFF 1
byte heatoff[8] = {4, 31, 0, 0, 0, 0, 31, 4};

// Bell
#define BELL 2
byte bell[8] = {4, 14, 14, 14, 31, 31, 4, 0};

// Timer
#define TIMER 3
byte timer[8] = {14, 17, 21, 21, 23, 17, 17, 14};

// ON
#define ON 4
byte on[8] = {16, 25, 26, 28, 24, 24, 24, 16};

// OFF
#define OFF 5
byte off[8] = {16, 24, 24, 24, 24, 28, 26, 17};


// TEMP_BAR for graphical temperature
#define TEMP_BAR 7
byte temp_bar[8];

// AVG_BAR for graphical average temperature
#define AVG_BAR 6


// Functions

// Create temeprature bar LCD char
void temp_bar_char(int temper, int bar) {
  if (int(temper) < 19) {
    temp_bar[0] = 25; temp_bar[1] = 17; temp_bar[2] = 25;  temp_bar[3] = 17; temp_bar[4] = 25; temp_bar[5] = 17; temp_bar[6] = 25; temp_bar[7] = 23;
    lcd.createChar(bar, temp_bar);
  }
  switch ( (int)temper) {
    case 19:
      temp_bar[0] = 24; temp_bar[1] = 16; temp_bar[2] = 24;  temp_bar[3] = 16; temp_bar[4] = 24; temp_bar[5] = 16; temp_bar[6] = 24; temp_bar[7] = 19;
      lcd.createChar(bar, temp_bar);
      break;
    case 20:
      temp_bar[0] = 24; temp_bar[1] = 16; temp_bar[2] = 24;  temp_bar[3] = 16; temp_bar[4] = 24; temp_bar[5] = 16; temp_bar[6] = 27; temp_bar[7] = 16;
      lcd.createChar(bar, temp_bar);
      break;
    case 21:
      temp_bar[0] = 24; temp_bar[1] = 16; temp_bar[2] = 24;  temp_bar[3] = 16; temp_bar[4] = 24; temp_bar[5] = 19; temp_bar[6] = 24; temp_bar[7] = 16;
      lcd.createChar(bar, temp_bar);
      break;
    case 22:
      temp_bar[0] = 24; temp_bar[1] = 16; temp_bar[2] = 24;  temp_bar[3] = 16; temp_bar[4] = 27; temp_bar[5] = 16; temp_bar[6] = 24; temp_bar[7] = 16;
      lcd.createChar(bar, temp_bar);
      break;
    case 23:
      temp_bar[0] = 24; temp_bar[1] = 16; temp_bar[2] = 24;  temp_bar[3] = 19; temp_bar[4] = 24; temp_bar[5] = 16; temp_bar[6] = 24; temp_bar[7] = 16;
      lcd.createChar(bar, temp_bar);
      break;
    case 24:
      temp_bar[0] = 24; temp_bar[1] = 16; temp_bar[2] = 27;  temp_bar[3] = 16; temp_bar[4] = 24; temp_bar[5] = 16; temp_bar[6] = 24; temp_bar[7] = 16;
      lcd.createChar(bar, temp_bar);
      break;
    case 25:
      temp_bar[0] = 24; temp_bar[1] = 19; temp_bar[2] = 24;  temp_bar[3] = 16; temp_bar[4] = 24; temp_bar[5] = 16; temp_bar[6] = 24; temp_bar[7] = 16;
      lcd.createChar(bar, temp_bar);
      break;
    case 26:
      temp_bar[0] = 27; temp_bar[1] = 16; temp_bar[2] = 24;  temp_bar[3] = 16; temp_bar[4] = 24; temp_bar[5] = 16; temp_bar[6] = 24; temp_bar[7] = 16;
      lcd.createChar(bar, temp_bar);
      break;
  }
  if (int(temper) > 26) {
    temp_bar[0] = 31; temp_bar[1] = 17; temp_bar[2] = 25; temp_bar[3] = 17; temp_bar[4] = 25; temp_bar[5] = 17; temp_bar[6] = 25; temp_bar[7] = 17;
    lcd.createChar(bar, temp_bar);
  }
}


void showTemperature() {
  //lcd.backlight();
  lm35 = analogRead(A0);
  temperature = lm35 * VDD / 1023.0;
  for (int i = 1; i < 10; i++) {
    avgtemp[i - 1] = avgtemp[i];
  }
  avgtemp[9] = temperature;
  float avgt = 0;
  Serial.println("********************");
  Serial.print("Count: ");
  Serial.print(counter);
  Serial.print(" Interval: ");
  Serial.print(interval);
  Serial.println(" millis.");
  for (int i = 0; i < 10; i++) {
    Serial.print(i);
    Serial.print(". ");
    Serial.println(avgtemp[i]);
    avgt += avgtemp[i];
  }
  Serial.print("Sum: ");
  Serial.print(avgt);
  Serial.print(" Avg: ");
  avgt /= 10.0;
  avgtemper = int(avgt);
  Serial.print(avgt);
  Serial.print(" (int)Avg: ");
  Serial.println(avgtemper);
  temp_bar_char((int)temperature, TEMP_BAR);
  temp_bar_char(avgtemper, AVG_BAR);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print((int)temperature);
  lcd.write(byte(TEMP_BAR));
  lcd.print(avgtemper);
  lcd.write(byte(AVG_BAR));
  lcd.print((int)HEATON);
  lcd.setCursor(15, 0);
  /*
  lcd.print(interval / 1000);
  lcd.setCursor(0, 1);
  for (int i = 0 ; i < 8; i++) {
    lcd.write(byte(i));
  }
  lcd.print(" ");
  */
  lcd.print(counter % 10);
  //show_interval(interval);
  //lcd.noBacklight();
  counter++;

  if (avgt >= HEATON) {
    lcd.setCursor(13, 0);
    lcd.write(byte(HEAT_OFF));
    is_heating = false;
    prev_is_heating = false;
  }
  if ((avgt - HYST) < HEATON) {
    lcd.setCursor(13, 0);
    lcd.write(byte(HEAT_ON));
    is_heating = true;
  }
  if (!prev_is_heating && is_heating ) {
    prev_is_heating = is_heating;
    heatcounter++;
  }

  lcd.setCursor(0, 1);
  lcd.write(byte(HEAT_ON));
  lcd.print(heatcounter - 1);

  if (avgtemper <= MINTEMP) {
    lcd.setCursor(15, 1);
    lcd.write(byte(BELL));
  }
  else {
    lcd.setCursor(15, 1);
    lcd.write(byte(TIMER));
  }
  Serial.print("is_heating: ");
    Serial.print(is_heating);
    Serial.print(" prev_is_heating: ");
    Serial.println(is_heating);
  // end show_temperature
}

// Show interval settings
void show_interval(long inter) {
  if (inter / 1000 < 10) {
    lcd.setCursor(14, 0);
    lcd.print(" ");
    lcd.print(inter / 1000);
  }
  else {
    lcd.setCursor(14, 0);
    lcd.print(inter / 1000);
  }
}

// ********************************************
// setup()
void setup() {
  Serial.begin(9600);
  pinMode(backlightswitch, INPUT);
  lcd.begin(16, 2);
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("---Thermostat---");
  lcd.setCursor(0, 1);
  lcd.print("<> triton-dev <>");
  delay(2000);
  lcd.createChar(HEAT_ON, heaton);
  lcd.createChar(HEAT_OFF, heatoff);
  lcd.createChar(BELL, bell);
  lcd.createChar(TIMER, timer);
  lcd.createChar(ON, on);
  lcd.createChar(OFF, off);
  lcd.noBacklight();
  lcd.clear();
  showTemperature();
  lm35 = analogRead(A0);
  temperature = lm35 * VDD / 1023.0;
  for (int i = 0; i < 10; i++) {
    avgtemp[i] = temperature;
  }
  //interval = map(analogRead(A1), 0, 1023, 1000, 60000);
  //end setup
}

// *********************************************************
// loop()
void loop() {
  // Read LM35 every #interval# sec.
  currtimestamp = millis();
  if (currtimestamp - prevtimestamp >= interval) {
    prevtimestamp = currtimestamp;
    showTemperature();
  }

  // Turn on - off backlight
  if (digitalRead(backlightswitch) == HIGH && !backlight) {
    backlight = true;
    lcd.backlight();
    delay(1000);
  }
  if (digitalRead(backlightswitch) == HIGH && backlight) {
    backlight = false;
    lcd.noBacklight();
    delay(1000);
  }



  /*
   // Backlight on for #backlightinetrval# seconds when button pressed
   if (digitalRead(backlightswitch) == HIGH && !backlight) {
     backlightprevtimestamp = millis();
     backlight = true;
   }
   if (backlight) {
     backlightcurrtimestamp = millis();
     if (backlightcurrtimestamp - backlightprevtimestamp <= backlightinterval) {
       lcd.backlight();
       lcd.setCursor(11, 1);
       lcd.print(86400 - (backlightcurrtimestamp - backlightprevtimestamp) / 1000);
     }
     else {
       lcd.noBacklight();
       backlight = false;
     }
   }
   */
  /*
  newinterval = map(analogRead(A1), 0, 1023, 1000, 60000);
  if (newinterval != interval) {
    show_interval(newinterval);
    interval = newinterval;
  }
  */

  //end loop
}
