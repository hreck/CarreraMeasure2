/*
CarreraMeasure2 - An arduino sketch to measure times of slotcars
 Copyright (C) 2012  Harm Reck
 
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */






#include <LiquidCrystal.h>
#include <Wire.h>

#define expander        B0100000  // I²C address of the expander, might be different depending on wiring

// pins for our buttons
#define resetPin        0
#define modePin         1
#define plusPin         2
#define minusPin        3

//modes
#define RACE_MODE       1
#define CONF_MODE_TIME  2
#define CONF_MODE_LAP   3

#define EDIT_OPT_1      1
#define EDIT_OPT_2      2
#define EDIT_OPT_3      3

//leds for startampel
#define RED_LEFT        B00000001
#define YELLOW_LEFT     B00000010
#define GREEN_LEFT      B00000100

#define RED_RIGHT       B00010000
#define YELLOW_RIGHT    B00100000
#define GREEN_RIGHT     B01000000

LiquidCrystal lcd(8,9,10,11,12,13); // might have to change this

volatile long curmillis = 0;
long lastmillis = 0;
int lapnr = 0;
long curlaptime = 0;
long fastest = 99999; 

boolean race_running = false;
int mode = CONF_MODE_TIME;

int editing = 0;

long timermillis = 0;

long refreshmils = 0;


int mins = 0;
int secs = 0;

int raceMins = 0;
int raceSecs = 0;

int lap = 0;
int raceLap = 0;

boolean lastButton[] = {
  LOW, LOW, LOW, LOW};
boolean currentButton[] = {
  LOW, LOW, LOW, LOW};
int buttonPins[] = {
  4, 5, 7, 6};
int pressCount = 0;

byte selChar[8] = {
  B00100,
  B01010,
  B10001,
  B00000,
  B10001,
  B01010,
  B00100,
};



void setup() {  
  lcd.begin(16,2);
  lcd.createChar(0, selChar);

  refreshmils = millis();

  Serial.begin(9600);
  attachInterrupt(0, time, RISING);
  pinMode(buttonPins[resetPin], INPUT);
  pinMode(buttonPins[modePin], INPUT);
  pinMode(buttonPins[plusPin], INPUT);
  pinMode(buttonPins[minusPin], INPUT);
  Wire.begin(); 
  expanderWrite(0xFF);

  enterTimeConfMode();

}


void initLCD() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("00 00:000s");
  lcd.setCursor(0,1);
  lcd.print("   00:000s"); 
  printRaceTimeLeft();

}

void printRaceTimeLeft(){

  char minbuf[6]="";
  sprintf(minbuf, "%02d:%02d", raceMins, raceSecs);
  lcd.setCursor(11,1);
  lcd.print(minbuf);


}

void raceOver(){
  race_running = false;
  byte writeByte = 0;
  writeByte |= RED_LEFT;
  writeByte |= RED_RIGHT;
  expanderWrite(~writeByte);
}

void enterTimeConfMode(){  
  race_running = false;
  editing = EDIT_OPT_1;
  mode = CONF_MODE_TIME;

  printTimeConf();

}

void enterLapConfMode(){
  race_running = false;
  editing = EDIT_OPT_1;
  mode = CONF_MODE_LAP;

  printLapConf();

}

void printLapConf(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(" Lap Limit:");
  lcd.setCursor(0,0);
  if(editing == EDIT_OPT_1){    
    lcd.write((uint8_t)0);
  }
  else{
    lcd.print(" ");
  }
  lcd.setCursor(1,1);
  char lapbuf[4] = "";
  sprintf(lapbuf, "%03d", lap);
  lcd.print(lapbuf);
  lcd.setCursor(0,1);
  if(editing == EDIT_OPT_2){   
    lcd.write((uint8_t)0); 
  }
  else{
    lcd.print(" ");
  }    

}

void printTimeConf(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(" Time Limit:");
  if(editing == EDIT_OPT_1){
    lcd.setCursor(0,0);
    lcd.write((uint8_t)0);
  }

  lcd.setCursor(0,1);
  refreshTimeLimitConf();
}


void enterRaceMode(){
  mode = RACE_MODE;
  //lcd.noCursor();

  reset();
}

void loop() {



  if(mode ==  RACE_MODE){ 




    if(timermillis >0 && race_running){
      long now = millis();
      if(now - timermillis >=1000){
        if(raceSecs > 0){
          raceSecs--;
        }
        else{
          if(raceMins > 0){
            raceSecs = 59;
            raceMins--;
          }
        }
        printRaceTimeLeft();

        if(raceMins==0 && raceSecs == 0){
          raceOver();
        }

        timermillis = now;


      } 

    }


    // See if we have a new laptime
    if(curmillis - lastmillis > 300){ //debounce probaply better do it in hardware
      if(lastmillis == 0){

        lastmillis = curmillis;
      }
      else{
        curlaptime = curmillis - lastmillis;
        if(curlaptime > 99999)
          curlaptime = 99999L;
        lapnr++;
        if(lapnr > 99)
          lapnr = 0;
        if(curlaptime < fastest){
          fastest = curlaptime; 
        }

        printOut();

        lastmillis = curmillis;

      }

    }


  }




  //resetButton
  currentButton[resetPin] = debounce(resetPin);
  if (lastButton[resetPin] == LOW && currentButton[resetPin] == HIGH)
  {

    if (mode == RACE_MODE){

      if(race_running){
        reset();

      }
      else{
        start();
      }
    }

    else if ( CONF_MODE_TIME ){
      if(mins>0 || secs > 0){
        enterRaceMode();
      }
    }




  }
  lastButton[resetPin] = currentButton[resetPin];  


  //modeButton
  currentButton[modePin] = debounce(modePin);
  if (lastButton[modePin] == LOW && currentButton[modePin] == HIGH)
  {

    if ( mode ==  RACE_MODE ){

      enterTimeConfMode();
    }

    else if (mode == CONF_MODE_TIME){
      if(editing == EDIT_OPT_2){
        editing = EDIT_OPT_3;
      }
      else if(editing == EDIT_OPT_3){
        editing = EDIT_OPT_1;
      }
      else if(editing == EDIT_OPT_1){
        editing = EDIT_OPT_2;      
      }

      printTimeConf();


    }

    else if( mode == CONF_MODE_LAP){
      if(editing == EDIT_OPT_1){
        editing = EDIT_OPT_2;  
      } 
      else if(editing == EDIT_OPT_2){
        editing = EDIT_OPT_1;
      }
      printLapConf();

    }





  }
  lastButton[modePin] = currentButton[modePin];  



  //plusButton
  currentButton[plusPin] = debounce(plusPin);
  if (lastButton[plusPin] == LOW && currentButton[plusPin] == HIGH)
  {

    if ( mode ==  CONF_MODE_TIME){

      if (editing ==  EDIT_OPT_3){
        if(secs < 59){
          secs++;
        }
        else{
          secs = 0;
          if(mins < 99)
            mins++;
        }
        printTimeConf();
      }
      else if (editing ==  EDIT_OPT_2){
        if(mins < 99){
          mins++;
          printTimeConf();
        }
      }

      else if(editing ==  EDIT_OPT_1){
        enterLapConfMode();
      }

    }

    else if ( mode == CONF_MODE_LAP){

      if(editing ==  EDIT_OPT_1){
        enterTimeConfMode();
      }
      else if(editing ==  EDIT_OPT_2){
        if(lap < 999){
          lap++;
          printLapConf();
        }
      }

    }
  }
  lastButton[plusPin] = currentButton[plusPin];  


  //minusButton
  currentButton[minusPin] = debounce(minusPin);
  if (lastButton[minusPin] == LOW && currentButton[minusPin] == HIGH)
  {

    if( mode ==  CONF_MODE_TIME){

      if(editing == EDIT_OPT_3){
        if(secs > 0){
          secs--;
        }
        else{
          if(mins > 0){
            secs = 59;
            mins--;
          }
        }
        printTimeConf();
      }
      if (editing == EDIT_OPT_2){
        if(mins > 0){            
          mins--;
          printTimeConf();
        }
      }
      if (editing ==  EDIT_OPT_1){
        enterLapConfMode();
      }





    }

    else if( mode == CONF_MODE_LAP){


      if(editing == EDIT_OPT_2){
        if(lap > 0){            
          lap--;
          printLapConf();
        }
      }


      else if(editing == EDIT_OPT_1){
        enterTimeConfMode();


      }



    }



  }
  lastButton[minusPin] = currentButton[minusPin];  

}


void refreshTimeLimitConf(){
  char minbuf[6]="";
  sprintf(minbuf, "%02d:%02d", mins, secs);
  lcd.setCursor(0,1);
  lcd.print("                ");
  lcd.setCursor(1,1);
  lcd.print(minbuf);
  if(editing == EDIT_OPT_2){
    lcd.setCursor(0,1);
    lcd.write((uint8_t)0);
  }
  else if(editing == EDIT_OPT_3){
    lcd.setCursor(6,1);
    lcd.write((uint8_t)0);
  }


}


void reset(){
  curmillis = 0L;
  lastmillis = 0L;
  lapnr = 0;
  curlaptime = 0L;
  fastest = 99999L;  

  raceMins = mins;
  raceSecs = secs;

  race_running = false;

  initLCD();
  expanderWrite(0xFF);

}

void start(){
  raceMins = mins;
  raceSecs = secs;
  initLCD();

  startAmpel();

  timermillis = millis();

  race_running = true;
}

void time(){
  if(race_running)
    curmillis = millis(); 
}

boolean debounce(int pin)
{
  boolean current = digitalRead(buttonPins[pin]);
  if (lastButton[pin] != current)
  {
    delay(10                 );
    current = digitalRead(buttonPins[pin]);
  }
  return current;
}


void printOut(){
  Serial.print(lapnr);
  Serial.print(";");
  Serial.print(curlaptime);
  Serial.print(";");
  Serial.println(fastest);

  lcd.clear();
  lcd.setCursor(0,0);

  char lapbuf[3]="";
  char timebuf[9]="";
  char fastbuf[9]="";
  sprintf(lapbuf,"%02d", lapnr);      
  lcd.print(lapbuf);
  lcd.print(" ");
  int lapsec = curlaptime / 1000;
  int lapmillis = curlaptime - lapsec * 1000;
  sprintf(timebuf, "%02d:%03ds", lapsec, lapmillis);
  Serial.println(timebuf);
  lcd.print(timebuf);
  lcd.setCursor(0,1);
  lcd.print("   ");
  int fastsec = fastest / 1000;
  int fastmillis = fastest - fastsec * 1000;
  sprintf(fastbuf, "%02d:%03ds", fastsec, fastmillis);
  Serial.println(fastbuf);
  lcd.print(fastbuf);
}


void startAmpel(){
  expanderWrite(0xFF);
  delay(2000);
  byte writeByte = 0;
  writeByte |= RED_LEFT;
  writeByte |= RED_RIGHT;

  expanderWrite(~writeByte);
  delay(1000);
  writeByte = 0;
  writeByte |= RED_LEFT;
  writeByte |= RED_RIGHT;
  writeByte |= YELLOW_LEFT;
  writeByte |= YELLOW_RIGHT;

  expanderWrite(~writeByte);
  delay(1000);
  writeByte = 0;
  writeByte |= YELLOW_LEFT;
  writeByte |= YELLOW_RIGHT;
  expanderWrite(~writeByte);
  delay(1000);
  writeByte = 0;
  writeByte |= GREEN_LEFT;
  writeByte |= GREEN_RIGHT;
  expanderWrite(~writeByte);


}

void expanderWrite(byte _data ) {
  Wire.beginTransmission(expander);
  Wire.write(_data);
  Wire.endTransmission();
}











