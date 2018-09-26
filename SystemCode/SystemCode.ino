#include <Wire.h>  
#include <LiquidCrystal_I2C.h>
#include <SD.h>
#include "RTClib.h"

#define LOG_PERIOD 15000     //Logging period in milliseconds, recommended value 15000-60000.
#define MAX_PERIOD 60000    //Maximum logging period
#define UPD_PERIOD 1000     //Period for updating the CPM value. Uses 'moving' summation

unsigned long counts;             //variable for GM Tube events
unsigned long cpm;                 //variable for CPM
unsigned int multiplier;             //variable for calculation CPM in this sketch
unsigned long previousMillis;      //variable for time measurement
double sensitivity = 25.5; // [cps*hr/mR], average sensitivity SBM20
double D_tissue = 9.6; // [uGy], the absorbed dose by tissue for 1 [mR] radiation
double conversionToSv; // CPM per (uSv/hr) calculated as sensitivity*60/D_tissue

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);
boolean holdButton = 0, saving = 0;
long startTime;
long elapsedTime;
int melStartRec[] = {2, 3520, 4186};
int melStopRec[] = {2, 4186, 3520};
int melError[] = {3, 3000, 3000, 3000};

int melModerateRad[] = {2, 3951, 3951};
int melMediumRad[] = {3, 3951, 3951, 3951};
int melHighRad[] = {3, 4186, 4186, 4186};

// counts per second container
int k = 0;
const int cc = MAX_PERIOD/UPD_PERIOD+1;
int cps[cc];

File radLog;
int l = 0;
int cpmLog[15];

RTC_DS1307 rtc;

void setup() {
  pinMode(3, INPUT_PULLUP);
  lcd.begin(16,2); // sixteen characters across - 2 lines
  //lcd.backlight();
  lcd.setCursor(0,0);
  delay(500);
  lcd.print("Geiger Kounter");
  lcd.setCursor(0,1);
  delay(1000);

  DateTime now = rtc.now();
  lcd.print(now.day());
  lcd.print("/");
  lcd.print(now.month());
  lcd.print("/");
  lcd.print(now.year());
  lcd.print(" ");
  uint8_t hours = now.hour();
  uint8_t mins = now.minute();
  if (hours < 10) {
    lcd.print("0");
  }
  lcd.print(hours);
  lcd.print(":");
  if (mins < 10) {
    lcd.print("0");
  }
  lcd.print(mins);

  delay(5000);
  lcd.clear();

  lcd.setCursor(0,0);
  lcd.print("CPM: ");
  lcd.setCursor(0,1);
  lcd.print((char)0xe4);lcd.print("Sv: ");

  conversionToSv = sensitivity*60/D_tissue;
  counts = 0;
  cpm = 0;
  multiplier = MAX_PERIOD / LOG_PERIOD;
  pinMode(2, INPUT);                                   // set pin INT0 input for capturing GM Tube events
  digitalWrite(2, HIGH);                                 // turn on internal pullup resistors, solder C-INT on the PCB
  attachInterrupt(0, tube_impulse, FALLING);  //define external interrupts
}

void tube_impulse(){               //procedure for capturing events from Geiger Kit
  counts++;
}

void playSound(int* sound) {
  for(int n = 1; n < sound[0]+1; n++) {
    tone(5, sound[n], 75);
    delay(1.30*75);
    noTone(5);
  }
}

void playSuperAlert(void) {
  tone(5, 4186, 100);
  delay(120);
  noTone(5);
  tone(5, 4186, 800);
  delay(1.30*800);
  noTone(5);
}

void checkButton(){
  holdButton = digitalRead(3);
  startTime = millis();
  elapsedTime = 0;
  while ((holdButton==0)&&(elapsedTime<1000))
  {
    holdButton = digitalRead(3);
    elapsedTime = millis() - startTime;
  }
  
  lcd.setCursor(13,0);
  if ((elapsedTime>800)&&(saving==0))
  {

    // open sd-card
    if (SD.begin(10)) {
      saving = 1;
      lcd.print("REC");
      playSound(melStartRec);
      radLog = SD.open("RADLOG.txt", FILE_WRITE);
    }
    else {
      lcd.print("ERR");
      playSound(melError);
    }
    
    // re-initialize for saving
    for(int n = 0; n < cc; n++) {cps[n] = 0;}
    counts = 0;
    cpm = 0;
  }
  else if ((elapsedTime>800)&&(saving==1))
  {
    radLog.close();
    saving = 0;
    lcd.print("   ");
    playSound(melStopRec);
    //delay(1000);
  }
}

void outputToLCD(unsigned long val) {
    lcd.setCursor(5,0);
    lcd.print("    ");
    lcd.setCursor(5,0);
    lcd.print(val);
    lcd.setCursor(5,1);
    lcd.print("    ");
    lcd.setCursor(5,1);
    lcd.print(cpm/conversionToSv);  
}

// timing
unsigned long alertMillis = 0;
unsigned long saveMillis = 0;

void loop() {
  
  checkButton();
  unsigned long currentMillis = millis();

  if(currentMillis - previousMillis > UPD_PERIOD){
    previousMillis = currentMillis;
    cps[k++%cc] = counts;
    cpm += cps[(k-1)%cc] - cps[k%cc];
    
    outputToLCD(cpm);
    
    counts = 0;
  }

  // Buzzer alerts:
  if ((cpm>500)&&(cpm<2000)&&(currentMillis - alertMillis > 30000)) {alertMillis=currentMillis;playSound(melModerateRad);}
  if ((cpm>2000)&&(cpm<5000)&&(currentMillis - alertMillis > 30000)) {alertMillis=currentMillis;playSound(melMediumRad);}
  if ((cpm>5000)/*&&(cpm<10000)*/&&(currentMillis - alertMillis > 10000)) {alertMillis=currentMillis;playSound(melHighRad);}
  //if ((cpm>100000)&&(currentMillis - alertMillis > 5000)) {alertMillis=currentMillis;playSuperAlert();}

  if ((saving==1)&&(currentMillis - saveMillis > 60000)) {
    radLog.println(currentMillis);
    cpmLog[l++] = cpm;
    saveMillis = currentMillis;
  }
  
  if (l==15) {
    for (int i = 0; i < l; i++) {
      radLog.print(cpmLog[i]);
      radLog.print("\t");
      radLog.println(cpmLog[i]/conversionToSv, 2);
    }
    radLog.flush();
    l = 0;
  }
  
}
