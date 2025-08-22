#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <Servo.h>

LiquidCrystal_I2C lcd(0x27,16,2);

const byte ROWS=4; 
const byte COLS=4; 
char keys[ROWS][COLS]={{'1','2','3','A'},{'4','5','6','B'},{'7','8','9','C'},{'*','0','#','D'}};
byte rowPins[ROWS]={22,23,24,25};
byte colPins[COLS]={26,27,28,29};
Keypad keypad=Keypad(makeKeymap(keys),rowPins,colPins,ROWS,COLS);

const int trigPin=7;
const int echoPin=6;

Servo servoPrimary;
Servo servoSecondary;

int currentTank=0;
unsigned long lastSwitch=0;
unsigned long switchInterval=60000;

float cupPresentThreshold=10.0;
float targetDistance=5.0;
unsigned long valveTimeout=600000;

long readUltrasonic() {
  digitalWrite(trigPin,LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin,HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin,LOW);
  long duration=pulseIn(echoPin,HIGH,30000);
  if(duration==0) return 9999;
  return duration/58;
}

void openValve(int tank) {
  if(tank==0) servoPrimary.write(90); else servoSecondary.write(90);
}
void closeValve(int tank) {
  if(tank==0) servoPrimary.write(0); else servoSecondary.write(0);
}

void setTargetByKey(char k){
  if(k=='1') targetDistance=6.0;
  else if(k=='2') targetDistance=5.0;
  else if(k=='3') targetDistance=4.0;
  else if(k=='4') targetDistance=3.0;
  else if(k=='5') targetDistance=2.5;
  else if(k=='6') targetDistance=2.0;
}

void showHome(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Place cup...");
  lcd.setCursor(0,1);
  lcd.print("Select:1-6 size");
}

void setup(){
  pinMode(trigPin,OUTPUT);
  pinMode(echoPin,INPUT);
  servoPrimary.attach(9);
  servoSecondary.attach(10);
  closeValve(0);
  closeValve(1);
  lcd.init();
  lcd.backlight();
  showHome();
}

void loop(){
  long d=readUltrasonic();
  if(d<cupPresentThreshold){
    lcd.setCursor(0,0);
    lcd.print("Cup detected   ");
    lcd.setCursor(0,1);
    lcd.print("1-6 Volume sel ");
    char k=keypad.getKey();
    if(k){
      if(k>='1'&&k<='6'){
        setTargetByKey(k);
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Dispensing...");
        unsigned long start=millis();
        if(millis()-lastSwitch>switchInterval){ currentTank^=1; lastSwitch=millis(); }
        openValve(currentTank);
        while(true){
          long dd=readUltrasonic();
          lcd.setCursor(0,1);
          lcd.print("Lvl d=");
          lcd.print(dd);
          lcd.print("cm   ");
          if(dd<=targetDistance) break;
          if(millis()-start>valveTimeout) break;
          if(keypad.getKey()=='#') break;
        }
        closeValve(currentTank);
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Done. Remove cup");
        lcd.setCursor(0,1);
        lcd.print("Hold * to reset");
        unsigned long t0=millis();
        while(millis()-t0<5000){
          if(keypad.getKey()=='*') break;
          delay(10);
        }
        showHome();
      } else if(k=='A'){
        currentTank=0; lastSwitch=millis();
        lcd.clear(); lcd.setCursor(0,0); lcd.print("Primary ready");
        delay(1000); showHome();
      } else if(k=='B'){
        currentTank=1; lastSwitch=millis();
        lcd.clear(); lcd.setCursor(0,0); lcd.print("Secondary ready");
        delay(1000); showHome();
      } else if(k=='*'){
        showHome();
      }
    }
  } else {
    lcd.setCursor(0,0);
    lcd.print("Place cup...   ");
    char k=keypad.getKey();
    if(k>='1'&&k<='6'){ setTargetByKey(k); }
    if(k=='A'){ currentTank=0; }
    if(k=='B'){ currentTank=1; }
  }
  delay(50);
}
