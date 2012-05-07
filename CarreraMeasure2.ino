#include <LiquidCrystal.h>
#include <Wire.h>

#define expander B0100000

// pins for our buttons
#define resetPin  0
#define modePin  1
#define plusPin  2
#define minusPin  3

//modes
#define RACE_MODE 1
#define CONF_MODE 2

#define EDIT_MINS  1
#define EDIT_SECS  2

//leds for startampel
#define RED_LEFT B00000001
#define YELLOW_LEFT B00000010
#define GREEN_LEFT B00000100

#define RED_RIGHT B00010000
#define YELLOW_RIGHT B00100000
#define GREEN_RIGHT B01000000

LiquidCrystal lcd(8,9,10,11,12,13); // might have to change this

volatile long curmillis = 0;
long lastmillis = 0;
int lapnr = 0;
long curlaptime = 0;
long fastest = 32000; 

boolean race_running = false;
int mode = CONF_MODE;

int editing = EDIT_MINS;

long timermillis = 0;


int mins = 0;
int secs = 0;

int raceMins = 0;
int raceSecs = 0;

boolean lastButton[] = {
  LOW, LOW, LOW, LOW};
boolean currentButton[] = {
  LOW, LOW, LOW, LOW};
int buttonPins[] = {
  4, 5, 7, 6};
int pressCount = 0;



void setup() {
  lcd.begin(16,2);
  Serial.begin(9600);
  attachInterrupt(0, time, RISING);
  pinMode(buttonPins[resetPin], INPUT);
  pinMode(buttonPins[modePin], INPUT);
  pinMode(buttonPins[plusPin], INPUT);
  pinMode(buttonPins[minusPin], INPUT);
  Wire.begin(); // join i2c bus (address optional for master)
  expanderWrite(0xFF);

  enterConfMode();

  // initLCD();

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
  //  lcd.clear();
  //  lcd.setCursor(0,0);
  //  lcd.print("Race Over");

  byte writeByte = 0;
  writeByte |= RED_LEFT;
  writeByte |= RED_RIGHT;
  expanderWrite(~writeByte);
}

void enterConfMode(){
  lcd.cursor();
  race_running = false;
  mode = CONF_MODE;
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Time Limit:");
  lcd.setCursor(0,1);
  refreshTimeLimitConf();


}

void enterRaceMode(){
  mode = RACE_MODE;
  lcd.noCursor();

  reset();
}

void loop() {

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
      //Serial.write("lastmillis == 0\n");
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




  //resetButton
  currentButton[resetPin] = debounce(resetPin);
  if (lastButton[resetPin] == LOW && currentButton[resetPin] == HIGH)
  {
    switch(mode){
    case RACE_MODE:

      if(race_running){
        reset();

      }
      else{
        start();
      }
      break;

    case CONF_MODE:
      if(mins>0 || secs > 0){
        enterRaceMode();
      }
      break;


    }

  }
  lastButton[resetPin] = currentButton[resetPin];  


  //modePin
  currentButton[modePin] = debounce(modePin);
  if (lastButton[modePin] == LOW && currentButton[modePin] == HIGH)
  {
    switch(mode){
    case RACE_MODE:

      enterConfMode();
      break;

    case CONF_MODE:
      if(editing == EDIT_MINS){
        editing = EDIT_SECS;
      }
      else if(editing == EDIT_SECS){
        editing = EDIT_MINS;
      }

      refreshTimeLimitConf();
      break;



    }

  }
  lastButton[modePin] = currentButton[modePin];  



  //plusButton
  currentButton[plusPin] = debounce(plusPin);
  if (lastButton[plusPin] == LOW && currentButton[plusPin] == HIGH)
  {
    switch(mode){
    case CONF_MODE:
      switch(editing){
      case EDIT_SECS:
        if(secs < 59){
          secs++;
        }
        else{
          secs = 0;
          if(mins < 99)
            mins++;
        }
        break; 
      case EDIT_MINS:
        if(mins < 99)
          mins++;
        break;


      }

      refreshTimeLimitConf();

      break;

    }

  }
  lastButton[plusPin] = currentButton[plusPin];  


  //minusButton
  currentButton[minusPin] = debounce(minusPin);
  if (lastButton[minusPin] == LOW && currentButton[minusPin] == HIGH)
  {
    switch(mode){
    case CONF_MODE:
      switch(editing){
      case EDIT_SECS:
        if(secs > 0){
          secs--;
        }
        else{
          if(mins > 0){
            secs = 59;
            mins--;
          }
        }
        break; 
       case EDIT_MINS:
       if(mins > 0){            
            mins--;
          }

      }

      refreshTimeLimitConf();

      break;

    }

  }
  lastButton[minusPin] = currentButton[minusPin];  

}


void refreshTimeLimitConf(){
  char minbuf[6]="";
  sprintf(minbuf, "%02d:%02d", mins, secs);
  lcd.setCursor(0,1);
  lcd.print(minbuf);
  if(editing == EDIT_MINS){
    lcd.setCursor(0,1);
  }
  else if(editing == EDIT_SECS){
    lcd.setCursor(3,1);
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
    delay(5);
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






