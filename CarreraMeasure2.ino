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
#define RACE_MODE_LAP   2
#define CONF_MODE_TIME  3
#define CONF_MODE_LAP   4

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

LiquidCrystal lcd(4,5,6,7,8,9); // might have to change this

volatile long slot1mils = 0;
volatile long slot2mils = 0;
long lastslot1mils = 0;
long lastslot2mils = 0;
int lapnrSlot1 = 0;
int lapnrSlot2 = 0;
long curlaptimeSlot1 = 0;
long curlaptimeSlot2 = 0;
long fastestSlot1 = 99999;
long fastestSlot2 = 99999;

boolean race_running = false;
int mode = CONF_MODE_TIME;

int editing = 0;

long timermillis = 0;

long refreshmils = 0;

long slot1StartMils = 0;
long slot2StartMils = 0;

long slot1RaceTime = 0;
long slot2RaceTime = 0;


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
int buttonPins[] = {          // pinout for the buttons
  10, 11, 12, 13};            // change according to your wiring       
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

byte lightsBuffer = 0;



void setup() {  
  lcd.begin(16,4);
  lcd.createChar(0, selChar);

  refreshmils = millis();

  Serial.begin(9600);
  attachInterrupt(0, time, RISING);
  attachInterrupt(1, time1, RISING);
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
  lcd.print("000          000");
  lcd.setCursor(0,1);
  lcd.print("00:000s");
  lcd.print("  ");
  lcd.print("00:000s");
  lcd.setCursor(-4,2); // for some reason theres a 4 col offset in line 2 and 3. might just be my display no idea
  lcd.print("00:000s");
  lcd.print("  ");
  lcd.print("00:000s"); 
  if(mode == RACE_MODE){
  printRaceTimeLeft();
  }else if(mode == RACE_MODE_LAP){
   printLapLimit(); 
  }

}

void printLapLimit(){
  char lapbuf[4]="";
  sprintf(lapbuf, "%03d", lap);
  lcd.setCursor(6,0);
  lcd.print(lapbuf);
}

void printRaceTimeLeft(){

  char minbuf[6]="";
  sprintf(minbuf, "%02d:%02d", raceMins, raceSecs);
  lcd.setCursor(6,0);
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

void enterRaceLapMode(){
  mode = RACE_MODE_LAP;
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
  }

  if ( mode == RACE_MODE || mode == RACE_MODE_LAP){

    // See if we have a new laptime for slot 1
    if(slot1mils - lastslot1mils > 300){ //debounce probaply better do it in hardware
      if(lastslot1mils == 0){

        lastslot1mils = slot1mils;
        slot1StartMils = slot1mils;
      }
      else{
        if(mode == RACE_MODE || (mode == RACE_MODE_LAP && lapnrSlot1 < lap)){
          curlaptimeSlot1 = slot1mils - lastslot1mils;
          if(curlaptimeSlot1 > 99999)
            curlaptimeSlot1 = 99999L;
          lapnrSlot1++;
          if(lapnrSlot1 > 999)
            lapnrSlot1 = 0;
          if(curlaptimeSlot1 < fastestSlot1){
            fastestSlot1 = curlaptimeSlot1; 


          }

          

          lastslot1mils = slot1mils;

          if(mode == RACE_MODE_LAP && lapnrSlot1 >= lap){
            slot1RaceTime = slot1mils - slot1StartMils;
            slot1Red();
            
          }
          printOut();
        }


      }

    }

    if(slot2mils - lastslot2mils > 300){ //debounce probaply better do it in hardware
      if(lastslot2mils == 0){

        lastslot2mils = slot2mils;
        slot2StartMils = slot2mils;
      }
      else{
        if(mode == RACE_MODE || (mode == RACE_MODE_LAP && lapnrSlot2 < lap)){
          curlaptimeSlot2 = slot2mils - lastslot1mils;
          if(curlaptimeSlot2 > 99999)
            curlaptimeSlot2 = 99999L;
          lapnrSlot2++;
          if(lapnrSlot2 > 999)
            lapnrSlot2 = 0;
          if(curlaptimeSlot2 < fastestSlot2){
            fastestSlot2 = curlaptimeSlot2; 
          }

          

          lastslot2mils = slot2mils;

          if(mode == RACE_MODE_LAP && lapnrSlot2 >= lap){
            slot2RaceTime = slot2mils - slot2StartMils;
            slot2Red();
            
          }
          printOut();

        }

      }

    }

  }







  //resetButton
  currentButton[resetPin] = debounce(resetPin);
  if (lastButton[resetPin] == LOW && currentButton[resetPin] == HIGH)
  {

    if (mode == RACE_MODE || mode == RACE_MODE_LAP){

      if(race_running){
        reset();

      }
      else{
        start();
      }
    }

    else if ( mode == CONF_MODE_TIME ){
      if(mins>0 || secs > 0){
        enterRaceMode();
      }
    }
    else if ( mode == CONF_MODE_LAP ){
      if(lap > 0){
        enterRaceLapMode();
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
    else if ( mode == RACE_MODE_LAP){
      enterLapConfMode();
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
  slot1mils = 0L;
  lastslot1mils = 0L;
  lapnrSlot1 = 0;
  curlaptimeSlot1 = 0L;
  fastestSlot1 = 99999L;  
  
  slot1StartMils = 0L;
  slot1RaceTime = 0L;

  slot2StartMils = 0L;
  slot2RaceTime = 0L;

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
    slot1mils = millis(); 
}

void time1(){
  if(race_running)
    slot2mils = millis(); 
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
  //  Serial.print(lapnrSlot1);
  //  Serial.print(";");
  //  Serial.print(curlaptimeSlot1);
  //  Serial.print(";");
  //  Serial.println(fastestSlot1);

  lcd.clear();
  lcd.setCursor(0,0);

  char lapbuf[3]="";
  char timebuf[9]="";
  char fastbuf[9]="";

  char lapbuf2[3]="";
  char timebuf2[9]="";
  char fastbuf2[9]="";

  sprintf(lapbuf,"%03d", lapnrSlot1);      
  lcd.print(lapbuf);
  lcd.print("          ");
  sprintf(lapbuf2,"%03d", lapnrSlot2);    
  lcd.print(lapbuf2); 
  lcd.setCursor(0,1);
  int lapsec = curlaptimeSlot1 / 1000;
  int lapmillis = curlaptimeSlot1 - lapsec * 1000;
  sprintf(timebuf, "%02d:%03ds", lapsec, lapmillis);
  //Serial.println(timebuf);
  lcd.print(timebuf);
  lcd.print("  ");
  lapsec = curlaptimeSlot2 / 1000;
  lapmillis = curlaptimeSlot2 - lapsec * 1000;
  sprintf(timebuf2, "%02d:%03ds", lapsec, lapmillis);
  //Serial.println(timebuf);
  lcd.print(timebuf2);


  lcd.setCursor(-4,2); // strange offset again

  int fastsec = fastestSlot1 / 1000;
  int fastmillis = fastestSlot1 - fastsec * 1000;
  sprintf(fastbuf, "%02d:%03ds", fastsec, fastmillis);
  //Serial.println(fastbuf);
  lcd.print(fastbuf);
  lcd.print("  ");
  fastsec = fastestSlot2 / 1000;
  fastmillis = fastestSlot2 - fastsec * 1000;
  sprintf(fastbuf2, "%02d:%03ds", fastsec, fastmillis);
  lcd.print(fastbuf2);
  
  if(slot1RaceTime > 0){
    char racetimebuf1[7] = "";
    
    int racesecs1 = slot1RaceTime / 1000;
    int racemins1 = racesecs1 / 60;
    racesecs1 = racesecs1 - racemins1 * 60;
    
    sprintf(racetimebuf1, "%02d:%02dm", racemins1, racesecs1);
    lcd.setCursor(-4,3);
    lcd.print(racetimebuf1); 
    
  }  
  
  if(slot2RaceTime > 0){
    
    char racetimebuf2[7] = "";
    
    int racesecs2 = slot2RaceTime / 1000;
    int racemins2 = racesecs2 / 60;
    racesecs2 = racesecs2 - racemins2 * 60;
    
    sprintf(racetimebuf2, "%02d:%02dm", racemins2, racesecs2);
    lcd.setCursor(6,3);
    lcd.print(racetimebuf2); 
    
    
  }
  
  

  if(mode == RACE_MODE){
  printRaceTimeLeft();
  }else if(mode == RACE_MODE_LAP){
   printLapLimit(); 
  }
}


void slot1Red(){
  byte writeByte = ~lightsBuffer;
  writeByte &= ~GREEN_LEFT;
  writeByte |= RED_LEFT;
  expanderWrite(~writeByte);
  
}


void slot2Red(){
  byte writeByte = ~lightsBuffer;
  writeByte &= ~GREEN_RIGHT;
  writeByte |= RED_RIGHT;
  expanderWrite(~writeByte);
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
  lightsBuffer = _data;
}













