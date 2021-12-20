/*===============================================================================
library that include the system (!!!dont change it dude !!!)*/
#include <DallasTemperature.h>
#include <OneWire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <Wire.h>
#include <EEPROM.h>
#define ONE_WIRE_BUS 2
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensor(&oneWire);
LiquidCrystal_I2C lcd(0x27, 16, 2);

char customKey, mulaiKey;
//char stringAngka[2];
int indexKeypad = 0;
const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] = {
{'1', '2', '3', 'A'},
{'4', '5', '6', 'B'},
{'7', '8', '9', 'C'},
{'*', '0', '#', 'D'}
};

byte rowPins[ROWS] = {11,10,9,8};
byte colPins[COLS] = {7,6,5,4};

int ssr = 3; // alamati pin SSR

/*===============================================================================
//declare global variabel of fuzzy here below : */
float x, mf1, mf2, mf3, mf4, r1, r2, r3, r4 , u;
int u1 = 0, u2 = 200, u3 = 500, u4 = 1000;
float suhu_now, nilaisp, sp, error;
float sisa = 1000 - u;
/*===============================================================================*/
Keypad customKeypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
void(* resetFunc) (void) = 0; //declare reset function @ address 0

void hitung(){
//perhitungan membership function
  //error negatif
int a = -100, b = -50, c = 0, d = 5;
  mf1 = max(min(min((error-a)/(b-a),(d-error)/(d-c)),1),0); //trapesium
  
  //error positif kecil
int a1 = 0, b1 = 5, c1 = 10;
  mf2 = max(min((error-a1)/(b1-a1),(c1-error)/(c1-b1)),0); //segitiga
  
  //error positif sedang
int a2 = 5, b2 = 15, c2 = 29;
  mf3 = max(min((error-a2)/(b2-a2),(c2-error)/(c2-b2)),0); //segitiga

  //error positif besar
int a3 = 22, b3 = 30, c3 = 50, d3 = 55;
  mf4 = max(min(min((error-a3)/(b3-a3),(d3-error)/(d3-c3)),1),0); //trapesium
}

void fuzzy_rule(){
  //menerapkan hasil perhitungan membership function ke aturan
  hitung();
  r1 = mf1*u1; //jika error negatif, maka aktuator off
  r2 = mf2*u2; //jika error positif kecil, maka aktuator on sebentar
  r3 = mf3*u3; //jika error positif sedang, maka aktuator on sedang
  r4 = mf4*u4; //jika error positif besar, maka aktuator on lama
}

void defuzzyfikasi(){
  //defuzzyfikasi
  fuzzy_rule();
  u = (r1+r2+r3+r4)/(mf1+mf2+mf3+mf4);
}

void setsp(){
  lcd.setCursor(0,0);
  lcd.print("SET SP ");
  customKey = customKeypad.getKey();
  
    if(customKey >= '0' && customKey <= '9'){
    sp = sp*10 + (customKey - '0');
    lcd.setCursor(0, 1);
    lcd.print(sp);
    //lcd.print(customKey);
    //sp = customKey;
    }
  
    if(customKey == '*'){
    lcd.clear();
    delay(1000);
    nilaisp = sp;
    return;
       }
        setsp();
}

void cek_suhu(){
   sensor.requestTemperatures();
   suhu_now = sensor.getTempCByIndex(0);
   lcd.setCursor(0,0);
   lcd.print("suhu:");
   lcd.print(suhu_now);
   //sisa 5 baris sih, klo suhunya desimal
 lcd.setCursor(0,1);
 lcd.print("SP:"); 
 lcd.print(sp);
 //sisa 7 baris dengan angka 5 digit (float)
     error = sp - suhu_now;
     lcd.setCursor(11,0);
     lcd.print(" ERR:");
     lcd.setCursor(11,1);
     lcd.print(error);
 customKey = customKeypad.getKey();
    if(customKey == '*'){
    lcd.clear();
    delay(1000);
    return;
       }
        cek_suhu();
}

void mulai(){
 /*jika SP = 0*/
 if (sp==0) {
 lcd.setCursor(0, 0);
 lcd.print("Mohon Set SP");
 lcd.setCursor(0, 1);
 delay(2000);
 return;
}

/* SISA ITU WAKTU UNTUK OFF */
/*========================= */

  sensor.requestTemperatures();
  suhu_now = sensor.getTempCByIndex(0);
  error = sp - suhu_now;
  defuzzyfikasi();
/*Edit Data Logger Di PLX*/
  Serial.print("DATA, TIME, ");
  Serial.print(suhu_now);
  Serial.print(",");
  Serial.print(error);
  Serial.print(",");
  Serial.print(mf1);
  Serial.print(",");
  Serial.print(mf2);
  Serial.print(",");
  Serial.print(mf3);
  Serial.print(",");
  Serial.print(mf4);
  Serial.print(",");
  Serial.println(u);
/*=================================*/
  Serial.print("Suhu Aktual : ");  
  Serial.println(suhu_now);  
  Serial.print("Error : ");
  Serial.println(error);
  
  //print nilai membership function
  Serial.print("Nilai mf1: ");
  Serial.println(mf1);
  Serial.print("Nilai mf2: ");
  Serial.println(mf2);
  Serial.print("Nilai mf3: ");
  Serial.println(mf3);
  Serial.print("Nilai mf4: ");
  Serial.println(mf4);


Serial.print("L1");
Serial.println(suhu_now);
lcd.setCursor(0, 0);
lcd.print("Suhu = ");
lcd.print(suhu_now);
lcd.print(" ");
lcd.setCursor(0, 1);
lcd.print("Error = ");
lcd.print(error);
lcd.print("    ");
delay(200);
 
  //print nilai defuzzyfikasi
  Serial.print("Nilai Deffuzy : ");
  Serial.println(u);

  //membuat hasil defuzzyfikasi menjadi pwm untuk mengatur ssr 
  digitalWrite(ssr, HIGH);
  delay(u);
  digitalWrite(ssr, LOW);
  delay(sisa);

  /* jika u  = 0, maka ssr akan mati 1 detik, 
     jika u(defuzzyfikasi)  = 0, maka suhu 
     aktual  = set point; karena error = sp - s.sensor*/
  if(u=0){
    sisa = 1000 - u;
    digitalWrite(ssr, LOW);
    delay(sisa);
  }

/* Reset Perhitungan */
customKey = customKeypad.getKey();
if(customKey == '*'){
lcd.clear();
analogWrite(ssr,0);
resetFunc(); //call reset
delay(800);
return;
}
mulai();
}
/*===============================================================================*/

void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  pinMode(ssr, OUTPUT);
  nilaisp = sp; 
  sensor.begin();
  Serial.println("CLEARDATA");
/*Setting Label Padac PLX DAQ*/
  Serial.println("LABEL, Waktu, DS18B20(C), Error, MF1, MF2, MF3, MF4, Deffuzy");
  delay (500);
  lcd.clear();
 
  lcd.setCursor(0, 0);
  lcd.print("HEATER OTOMATIS");
  delay(3000);
  lcd.clear();
}

void loop() {
  customKey = customKeypad.getKey();
    if(x == 0){
    //lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("1.SET POINT");
   }
     if(x == 1){
     lcd.clear();
     lcd.setCursor(0,0);
     lcd.print("2.CEK SUHU");
     }
       if(x == 2){
       lcd.clear();
       lcd.setCursor(0,0);
       lcd.print("3.MULAI");
       } 
/* SETTING INPUT KEYPAD */
switch(customKey){
 case '0' ... '9':
 break;
       case '#':
       break;
 case '*':
 break;
       case 'A':
       x++; //increment x value
       break;
 case 'B':
 x--; //decrement x value
 break;
       case 'C':
       break;
 case 'D': //if user press D button
 if(x == 0){
 lcd.clear();
 /*analogWrite(ssr,0);
   and call setsp() function*/
 setsp();
// defuzzyfikasi();
 }
            if(x == 1){
            lcd.clear();
            /*analogWrite(ssr,0) & call cekonoff() function*/
            cek_suhu();
            }
 if(x == 2){
 lcd.clear();
 mulai();
 }           
            break;
            }
       /* x would set to default (0) when more than 2*/
       if(x > 2){
       x = 0;
       }
 /* x would set to default (2) when less than 0*/
 if(x < 0){
 x = 2;
 }
} //end void loop
