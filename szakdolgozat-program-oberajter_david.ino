
#include <LiquidCrystal.h>
#include <EEPROM.h>
#include "LCDKeypad.h"
#include "Wire.h"
// #include <SPI.h>
// #include <SD.h>
#include "SdFat.h"
SdFat SD;

LiquidCrystal lcd(8,9,4,5,6,7);  

#define SD_CS_PIN 3

#define DS3231_I2C_ADDRESS 0x68

//Date:
#define YEAR 0
#define MONTH 1
#define DAYS 2
#define WEEKDAY 3

//weekday state:
#define SUN 0
#define MON 1
#define TUE 2
#define WED 3
#define THU 4
#define FRI 5
#define SAT 6

//Time:
#define HOURS 0
#define MINUTES 1
#define SECONDS 2

//menu list:
#define IDLEMENU 0
#define SETDATE 1
#define SETTIME 2
#define DONE 3

byte year = 0;
byte month = 1;
byte days = 1;
byte hours = 0;
byte minutes = 0;
byte seconds = 0;
byte weekday = 0;
byte datesetting = 0;
byte timesetting = 0;
byte menustate = 0;

//keypad, menu

int keypad_pin = A0;
int keypad_value = 0;
int keypad_value_old = 0;
char btn_push;

byte mainMenuPage = 1;
byte mainMenuPageOld = 1;
byte mainMenuTotal = 6;

// Flowmeter

byte statusLed    = 13;
byte sensorInterrupt = 0;  // 0 = digital pin 2
byte sensorPin       = 2;

float calibrationFactor = 6.25;
volatile byte pulseCount;  

float flowRate;
float osszviz;     
float kobmeter;
float Vszamla;
unsigned int flowMilliLitres;
unsigned long totalMilliLitres;
unsigned long oldTime;
unsigned int total;                 
unsigned int ktotal;   

float Vszamla1;
float kobmeter1;

int aset;
int elocation;

int ekob = 20;
int eszamla = 100;

int eszamlapos = 0;
int ekobpos = 0;

// SD

File adatok;
File myfile;
char filename[] = "00000000.CSV";

//LED

int piros = 15;
int sarga = 16;
int zold  = 17;
//----------------------------------------------------------------------------SETUP START------------------------------------------------------------------
void setup()
{
  pinMode(piros,OUTPUT);
  pinMode(sarga,OUTPUT); 
  pinMode(zold,OUTPUT); 

     Serial.begin(9600);
      while (!Serial) {
        ; 
      }

    lcd.begin(16,2);
    Wire.begin();
    MainMenuDisplay();
    delay(1000);

    pinMode(statusLed, OUTPUT);
    digitalWrite(statusLed, HIGH);

    pinMode(SD_CS_PIN, OUTPUT);
    
    pinMode(sensorPin, INPUT);
    digitalWrite(sensorPin, HIGH);

      pulseCount        = 0;
      flowRate          = 0.0;
      flowMilliLitres   = 0;
      totalMilliLitres  = 0.0;
      oldTime           = 0;
      osszviz           = 0.0;
      kobmeter          = 0.0;
      Vszamla           = 0.0;
      kobmeter1         = 0.0;
      Vszamla1          = 0.0;
      
    attachInterrupt(sensorInterrupt, pulseCounter, FALLING);

    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("initialization failed!");
        return;
      }
      Serial.println("initialization done.");

}
//----------------------------------------------------LOOP START--------------------------------------------------------------------------------
void loop()
{   
    btn_push = ReadKeypad();
    MainMenuBtn();
    
    if(btn_push == 'S')
    {
        WaitBtnRelease();
        switch (mainMenuPage)
        {
            case 1:
              Orabeallitas();
              break;
            case 2:
              Vizfogyasztas();
              break;
            case 3:
              Szamlaadatok();
              break;
            case 4:
              Multhavifogy();
              break;
            case 5:
              Ehavifogy();
              break;
            case 6:
              Pontosido();
              break;
        }

          MainMenuDisplay();
          WaitBtnRelease();
    }
    delay(10);
  
}//---------------------------------------------------------------------- Loop end --------------------------------------------------------

void getFileName(){
sprintf(filename, "%02d%02d%02d.csv", year, month, days);
}

void pulseCounter()
{
  pulseCount++;
}

void Orabeallitas()
{   
  lcd.clear();
   int outStatus=0; 
   while(outStatus==0)
   {
            incTime();
            printTime();
            buttonListen();
       if(ReadKeypad()=='L')
       {
          unsigned long pressedMillis= millis();
          while(ReadKeypad()=='L')
          { 
            
            unsigned long currentMillis = millis();
            if(currentMillis-pressedMillis>1500)
            {
              outStatus=1;
              break;
            }          
          }       
       }
  }
}

void Vizfogyasztas()
{         
          lcd.clear();
          float ertekek[3];
          float Vszamla2 = 0.000;
          float kobmeter2 = 0.000;
          elocation = 0;
          for (int i = 0; i < 3; ) 
          {
            EEPROM.get(elocation, ertekek[i]);
            elocation = elocation + sizeof(ertekek[i]);
            delay(50);
            i++;
          }
          EEPROM.get(250, eszamlapos);
          if(eszamlapos>eszamla)
          {
            eszamla=eszamlapos;
           }
          EEPROM.get(200, ekobpos);
          if(ekobpos>ekob)
          {
            ekob=ekobpos;
          }
          EEPROM.get(eszamla, Vszamla2);
          EEPROM.get(ekob, kobmeter2);
          if(kobmeter1<kobmeter2)
          {
            kobmeter1=kobmeter2;
            Vszamla1=Vszamla2;
            }
  
              readDS3231time(&seconds, &minutes, &hours, &weekday, &days, &month, &year); 
              int j=month;
              int k=days;
              int h=hours;
              int p;
              getFileName();
                         
       while(ReadKeypad()!= 'L')
        {
          readDS3231time(&seconds, &minutes, &hours, &weekday, &days, &month, &year);
                  
          /*
          A teszteles vegso fazisaban arra jutottam hogy a Vizfogyasztas menupont,
          valoszinusithetoen tulterheli a dinamikus memoriat ami tobbnyire lefagyaszthatja 
          az Arduino-t, ezert kenytelenek vagyunk valamit kikapcsolni ebben az esetben
          az SD kartyara valo irast valasztottam mivel ezt hasznalja a memoria legnagyobb
          reszet. A rendszernek e problemajat egy nagyobb tárhellyel rendelkezo Arduino-ra 
          (pl.Arduino Mega) valo cserevel megoldhatjuk es akkor mindent hasznalhatunk.
          */
          /*
            adatok = SD.open(filename, FILE_WRITE);
                if(adatok)
                 {
                 adatok.print(osszviz);
                 adatok.print(",");
                 adatok.print(hours);
                 adatok.println("");
                 adatok.close();
                }
           */
               
            if((millis() - oldTime) > 1000)
           { 
            detachInterrupt(sensorInterrupt);
            flowRate = ((1000.0 / (millis() - oldTime)) * pulseCount) / calibrationFactor;
            oldTime = millis();
            flowMilliLitres = (flowRate / 60) * 1000;
            totalMilliLitres += flowMilliLitres;
            osszviz = totalMilliLitres * 0.001;  
            
            kobmeter = osszviz * 0.001;
        
            Vszamla = (kobmeter * (ertekek[0]+ertekek[1]+ertekek[2]))*(1+2*((float)1/(float)10)+((float)2/(float)100));
            pulseCount = 0;
            attachInterrupt(sensorInterrupt, pulseCounter, FALLING);
           }
       
            if(kobmeter1!=kobmeter)
              {
              kobmeter1+=kobmeter;
              }
            if(Vszamla1!=Vszamla)
              {
               Vszamla1+=Vszamla;
              }

            if (k != days)
             {
              digitalWrite(zold,LOW);
              digitalWrite(sarga,LOW);
              digitalWrite(piros,LOW);
              osszviz = 0.0;
              totalMilliLitres  = 0.0;
              kobmeter = 0.0;
              Vszamla = 0.0;
              k=days;
              getFileName();
             }
           
                 switch (int(osszviz))
                 {
                  case 40 ... 79:
                  digitalWrite(zold,HIGH);
                  break;
                  case 80 ... 119:
                  digitalWrite(sarga,HIGH);
                  break;
                  case 120 ... 9000:
                  digitalWrite(piros,HIGH);
                  break;
                  }

              if(minutes == 1 && seconds == 30)
              {
                if(j!=month)
                  {ekob = ekob + 5;
                   eszamla = eszamla + 7;
                   Vszamla1 = 0;
                   kobmeter1 = 0;
                   j+=1;
                   if (j==13)
                   {j=1;}
                   if(ekob == 60 && eszamla == 184)
                    {
                    ekob=20;
                    eszamla=100;
                    }
                   }
                EEPROM.put(ekob, kobmeter1);
                EEPROM.put(eszamla, Vszamla1);
                eszamlapos = eszamla;
                ekobpos = ekob;
                EEPROM.put(250, eszamlapos);
                EEPROM.put(200, ekobpos);
              }
               else if(minutes == 15 && seconds == 30)
               {
                EEPROM.put(ekob, kobmeter1);
                EEPROM.put(eszamla, Vszamla1);
                }
                else if(minutes == 30 && seconds == 30)
               {
                EEPROM.put(ekob, kobmeter1);
                EEPROM.put(eszamla, Vszamla1);
                }
                else if(minutes == 45 && seconds == 30)
               {
                EEPROM.put(ekob, kobmeter1);
                EEPROM.put(eszamla, Vszamla1);
                }
                else if(minutes == 59 && seconds == 30)
               {
                EEPROM.put(ekob, kobmeter1);
                EEPROM.put(eszamla, Vszamla1);
                }

                   lcd.setCursor(0,0);
                   lcd.print("Atfolyas: ");
                   lcd.setCursor(11,0);
                   lcd.print(int(osszviz));          
                   lcd.print(".");
                   total = (osszviz - int(osszviz)) * 10;
                   lcd.setCursor(14,0);
                   lcd.print(total, DEC);
                   lcd.print("L"); 
             
                   lcd.setCursor(0,1);
                   lcd.print("SELECT -> SZAMLA");
                      if(ReadKeypad()== 'S' && pulseCount==0)
                      { 
                        lcd.clear();
                        lcd.setCursor(0,0);
                        lcd.print("Kobmet: ");
                        lcd.setCursor(7,0);
                        lcd.print(kobmeter);
                        lcd.print("m3"); 
                        
                        lcd.setCursor(0,1);
                        lcd.print("Ossz:");
                        lcd.print(Vszamla);
                        lcd.print(" Din");
                        delay(5000); 
                        lcd.clear();
                      }
        }    
} 
   
void Szamlaadatok()
{  
    aset = 0;
    lcd.clear();
    lcd.print ("Elso ertek:");
    lcd.blink();
    char value[]= "00.00";
    float Vadatok;
    int cursorPos;
    boolean lcdNeedsUpdate=true;
      while(ReadKeypad()!= 'L')
      {    
          if(aset == 4)
          {
          lcd.setCursor(0,0);
          lcd.print("Masodik ertek");
          }
          else if(aset == 8)
          {
          lcd.setCursor(0,0);
          lcd.print("Harmadik ertek");
          }
          else if(aset == 12)
          {
          lcd.setCursor(0,0);
          lcd.print("Bevitel kesz");
          lcd.setCursor(0,1);
          lcd.print("Kilepes L gomb");
          }
          char btn = ReadKeypad();
          WaitBtnRelease();
          if (btn == 'R' || btn == 'U' || btn == 'D' || btn == 'S')lcdNeedsUpdate=true;         

            switch (btn)
            {
              case 'R':
              if (cursorPos<4) cursorPos++;
              break;
              case 'L':  
              if (cursorPos>0) 
              cursorPos--;
              break;
              case 'U':
              if (value[cursorPos]<'9') value[cursorPos]++;
              break;
              case 'D':
              if (value[cursorPos]>'0') value[cursorPos]--;
              break;
              case 'S': 
                Vadatok = atof(value);
                strcpy(value,"00.00");
                EEPROM.put(aset, Vadatok);
                Serial.println(Vadatok);
                Serial.println("To address:");
                Serial.println(aset);
                Serial.println("");
                delay(500);
                aset = aset + sizeof(Vadatok);
                cursorPos=0;
                break;
             }    
                  if (lcdNeedsUpdate)
                  { 
                  lcd.setCursor(0,1);
                  lcd.print(value);
                  lcd.setCursor(cursorPos,1);
                  lcdNeedsUpdate=false;
                  }
 
    }
}   

void Multhavifogy()
{ 
            float ekobread = 0.0;
            float eszamlaread = 0.0;
            EEPROM.get(250, eszamlapos);
                      if(eszamlapos>eszamla)
                      {
                        eszamla=eszamlapos;
                        }
            EEPROM.get(200, ekobpos);
                      if(ekobpos>ekob)
                      {
                        ekob=ekobpos;
                        }
            int location1 = ekob-5;
            int location2 = eszamla-7; 
            EEPROM.get(location1, ekobread);
            EEPROM.get(location2, eszamlaread);
            Serial.println(eszamla);
            Serial.println(ekob);
            Serial.println(ekobread);
            Serial.println(eszamlaread);

                while(ReadKeypad()!= 'L')
                   {
                    lcd.clear();
                    lcd.setCursor(0,0);
                    lcd.print("Kobm.:");
                    lcd.print(ekobread);
                    lcd.setCursor(0,1);
                    lcd.print("Szamla:");
                    lcd.print(eszamlaread);
                    delay(3000);
                    }
}

void Ehavifogy()
{     
      lcd.clear();
      float ehaviekobread = 0.0;
      float ehavieszamlaread = 0.0;
      EEPROM.get(250, eszamlapos);
          if(eszamlapos>eszamla)
          {
            eszamla=eszamlapos;
            }
      EEPROM.get(200, ekobpos);
          if(ekobpos>ekob)
          {
            ekob=ekobpos;
            }
      EEPROM.get(ekob, ehaviekobread);
      EEPROM.get(eszamla, ehavieszamlaread);
      
                while(ReadKeypad()!= 'L')
                {  
                  lcd.clear();
                  lcd.setCursor(0,0);
                  lcd.print("Kobm.:");
                  lcd.print(ehaviekobread);
                  lcd.setCursor(0,1);
                  lcd.print("Szamla:");
                  lcd.print(ehavieszamlaread);
                  delay(3000);    
                }
}

void Pontosido()
{
    lcd.clear();
   
    while(ReadKeypad()!= 'L')
    {      readDS3231time(&seconds, &minutes, &hours, &weekday, &days, &month, &year);
           lcd.setCursor(0,0);
           lcd.print("20");
           lcd.print(year);
           lcd.print("/");
           lcd.print(month);
           lcd.print("/");
           lcd.print(days);
           lcd.setCursor(0,1);
           lcd.print(hours);
           lcd.print(":");
           lcd.print(minutes);
           lcd.print(":");
           lcd.print(seconds);
           delay(1000);
           lcd.clear();
    }
}
/*----------------------------------------------- ALMENUK VEGE -----------------------------------------------------------------*/


void MainMenuDisplay()
{
    lcd.clear();
    lcd.setCursor(0,0);
    switch (mainMenuPage)
    {
        case 1:
          lcd.print("Orabeallitas");
          break;
        case 2:
          lcd.print("Vizfogyasztas");
          break;
        case 3:
          lcd.print("szamlaadatok");
          break;
        case 4:
          lcd.print("Mult havi fogy.");
          break;
        case 5:
          lcd.print("E havi fogy.");
          break;
        case 6:
          lcd.print("Pontosido");
          break;
    }
}

void MainMenuBtn()
{
    WaitBtnRelease();
    if(btn_push == 'U')
    {
        mainMenuPage++;
        if(mainMenuPage > mainMenuTotal)
          mainMenuPage = 1;
    }
    else if(btn_push == 'D')
    {
        mainMenuPage--;
        if(mainMenuPage == 0)
          mainMenuPage = mainMenuTotal;    
    }
    
    if(mainMenuPage != mainMenuPageOld) //csak akkor frissitsen ha valt menupontot
    {
        MainMenuDisplay();
        mainMenuPageOld = mainMenuPage;
    }
}

char ReadKeypad()
{
  keypad_value = analogRead(keypad_pin);
  
  if(keypad_value < 100)
    return 'R';
  else if(keypad_value < 200)
    return 'U';
  else if(keypad_value < 400)
    return 'D';
  else if(keypad_value < 600)
    return 'L';
  else if(keypad_value < 800)
    return 'S';
  else 
    return 'N';

}

void WaitBtnRelease()
{
    while( analogRead(keypad_pin) < 800){} 
}

//-----------------------------------------------------------* MENU KEZELES VEGE *---------------------------------------------

//----------------------------------------------------------- ORABEALLITAS KEZDETE --------------------------------------------
void buttonListen() {
  // gomb leolvas 5x masodpercenkent
  for (int i = 0; i < 5; i++) {
  
   btn_push = ReadKeypad();
     switch (menustate){
       case SETDATE:
          switch (btn_push) 
          {
            case 'R':
              datesetting++;
              break;
            case 'L':
              datesetting--;
              if (datesetting == -1) datesetting = 3;
              break;
            case 'U':
              switch (datesetting) {
              case YEAR:
                year++;
                break;
              case MONTH:
                month++;
                break;
              case DAYS:
                days++;
                break;
              case WEEKDAY:
                weekday++;
              }     
              break;
        
             case 'D':
              switch (datesetting) {
              case YEAR:
                year--;
                if (year == -1) year = 99;
                break;
              case MONTH:
                month--;
                if (month == 0) month = 12;
                break;
              case DAYS:
                days--;
                if (days == 0) days = 31;
                break;
              case WEEKDAY:
                weekday--;
                if (weekday == 0) days = 6;
              }
            }
       break;
       case SETTIME:
            switch (btn_push) 
            {
            case 'R':
              timesetting++;
              break;
            case 'L':
              timesetting--;
              if (timesetting == -1) timesetting = 2;
              break;
            case 'U':
              switch (timesetting) {
                case HOURS:
                  hours++;
                  break;
                case MINUTES:
                  minutes++;
                  break;
                case SECONDS:
                  seconds++;
                }     
              break;
            case 'D':
              switch (timesetting) {
                case HOURS:
                  hours--;
                  if (hours == -1) hours = 23;
                  break;
                case MINUTES:
                  minutes--;
                  if (minutes == -1) minutes = 59;
                  break;
                case SECONDS:
                  seconds--;
                  if (seconds == -1) seconds = 59;
                }
            }
          break;
        case DONE:
           // DS3231 seconds, minutes, hours, day, date, month, year
           if (btn_push== 'S') {
              setDS3231time(seconds,minutes,hours,weekday,days,month,year);
           }
    }
    if (btn_push== 'S') {
      menustate++;
    }
    datesetting %= 4;
    timesetting %= 3;
    menustate %=4;
    printSetting();

    year %= 100;
    month %= 13;
    days %= 32;
    hours %= 24;
    minutes %= 60;
    seconds %= 60;
    weekday %=7;
    printTime();

    // var 1/5 befejezesig
    while(millis() % 200 != 0);
  }
}

void printSetting() {
    switch (menustate){
        case SETDATE:
          lcd.setCursor(0,0);
          lcd.print("Beallitas:        ");
          lcd.setCursor(11,0);
          switch (datesetting) {
            case YEAR:
            lcd.print("EV   ");
            break;  
            case MONTH:
            lcd.print("Honap  ");
            break;  
          case DAYS:
            lcd.print("Nap   ");
            break;
          case WEEKDAY:
            lcd.print("Hetnap");
          }
        break;
        case SETTIME:
          lcd.setCursor(0,0);
          lcd.print("Beallitas:        ");
          lcd.setCursor(11,0);
          switch (timesetting) {  
            case HOURS:
              lcd.print("Ora  ");
              break;
            case MINUTES:
              lcd.print("Perc");
              break;
            case SECONDS:
              lcd.print("Masodperc");
          }
          break; 
        case IDLEMENU:
          lcd.setCursor(0,0);
          lcd.print("SELECT gomb az");
          lcd.setCursor(0,1);
          lcd.print("ora beallitashoz");
          break;
        case DONE:
          lcd.setCursor(0,0);
          lcd.print("Ora beallitva            ");
          lcd.setCursor(0,1);
          lcd.print("Az L gomb kilep");
      }
}

void incTime() {
 
  seconds++;
  if (seconds == 60) 
  {
    seconds = 0;
    minutes++;
    if (minutes == 60) 
    {
      
      minutes = 0;
      hours++;
      if (hours == 24) 
      {
        hours = 0;
        days++;
        weekday++;
      }
    }
  }
}

void printTime() {
  char time[17];
  switch(menustate){
    case SETDATE:
      lcd.setCursor(0,1);      
      switch(weekday){
        case SUN:
          sprintf(time, "%02i/%02i/%02i     SUN", year, month, days);
          break;
        case MON:
          sprintf(time, "%02i/%02i/%02i     MON", year, month, days);
          break;
        case TUE:
          sprintf(time, "%02i/%02i/%02i     TUE", year, month, days);
          break;
        case WED:
          sprintf(time, "%02i/%02i/%02i     WED", year, month, days);
          break;
        case THU:
          sprintf(time, "%02i/%02i/%02i     THU", year, month, days);
          break;
        case FRI:
          sprintf(time, "%02i/%02i/%02i     FRI", year, month, days);
          break;
        case SAT:
          sprintf(time, "%02i/%02i/%02i     SAT", year, month, days);
      }
      lcd.print(time);
      break;
    case SETTIME:
      lcd.setCursor(0,1);
      sprintf(time, "%02i:%02i:%02i        ", hours, minutes, seconds);
      lcd.print(time);
    } 
}

byte decToBcd(byte val)
{
  return( (val/10*16) + (val%10) );
}

byte bcdToDec(byte val)
{
  return( (val/16*10) + (val%16) );
}

void setDS3231time(byte second, byte minute, byte hour, byte dayOfWeek, byte dayOfMonth, byte month, byte year)
{
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); 
  Wire.write(decToBcd(second)); 
  Wire.write(decToBcd(minute)); 
  Wire.write(decToBcd(hour)); 
  Wire.write(decToBcd(dayOfWeek)); 
  Wire.write(decToBcd(dayOfMonth)); 
  Wire.write(decToBcd(month)); 
  Wire.write(decToBcd(year)); 
  Wire.endTransmission();
}
void readDS3231time(byte *second,byte *minute,byte *hour,byte *dayOfWeek,byte *dayOfMonth,byte *month,byte *year)
{
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); 
  Wire.endTransmission();
  Wire.requestFrom(DS3231_I2C_ADDRESS, 7);
  
  *second = bcdToDec(Wire.read() & 0x7f);
  *minute = bcdToDec(Wire.read());
  *hour = bcdToDec(Wire.read() & 0x3f);
  *dayOfWeek = bcdToDec(Wire.read());
  *dayOfMonth = bcdToDec(Wire.read());
  *month = bcdToDec(Wire.read());
  *year = bcdToDec(Wire.read());
}
