/*
 * Sensing Scents: A Smarter AirWick
 * By Noal Balint and Graham Sela
*/

#include <LiquidCrystal.h>
#include <OneWire.h>
#include <DallasTemperature.h>

int mostfetPin = 7;
int lightPin = A0;
int tempPin = 6;
int echoPin = 5;
int trigPin = 4;
int motionPin = A4;
int contactPin = A5;

int sprayShots = 2400;
int sprayDelay = 0;

int flushDuration = 0;
int state = 0;

unsigned long sprayInterupt = 0;
unsigned long menuInterupt = 0;
const int sprayButton = 3;
const int menuButton = 2;

//These may need to be resized depending on how many timers we need.
bool timerSet[2] = {false, false};
unsigned long timer[2] = {0,0};

int redPin = A3;
int greenValue = 0;
int greenPin = A2;

// spray iminent not included here– this will be represented by a blinking LED – related to state 6
char *states[] = {
  "resting     ", // 0
  "pee         ", // 1
  "poop        ", // 2
  "cleaning    ", // 3
  "menu open   ", // 4
  "sensing use ", // 5
  "spray       ", // 6
  "cannot spray", // 7
};

char *delays[] = {
  "10s", // 0
  "20s", // 1
  "30s", // 2
};

LiquidCrystal lcd(8, 9, 10, 11, 12, 13);
OneWire oneWire(tempPin);
DallasTemperature sensors(&oneWire);

void setup() {
  Serial.begin(9600);   // debug terminal
  sensors.begin();      // temperature setup
  lcd.begin(16, 2);     // lcd setup
  pinMode(mostfetPin, OUTPUT);
  pinMode(lightPin, INPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(motionPin, INPUT);
  pinMode(contactPin, INPUT);
  pinMode(menuButton, INPUT); //For switching to Menu Mode,
  attachInterrupt(1, menuISR, RISING); //will change the state of the system.
  pinMode(sprayButton, INPUT); //For Spray interupt. On button click,
  attachInterrupt(0, sprayISR, RISING); //will power toilet freshener to spray once.
}

void loop() {

  LEDstate();
  
  isSprayBlocked();
  // need to always be sensing for this
  flushDuration = senseToiletFlush();
  
  // display temp
  lcd.setCursor(0,0);
  lcd.print("temp: ");
  lcd.print(temp());
  lcd.print("C       ");
  lcd.setCursor(0,1);
  lcd.print("shots left: ");
  lcd.print(sprayShots);
  
  // if room is dark don't run normal program
  if (!light()) {

    state = 0;
    printState(0);
    // LED STATE ASSIGNMENT
    
  // ANOTHER ELSEIF FOR OPPERATOR MENU IMPLEMENTATION / INTERUPT



  // stay in cleaning mode until the brush gets returned 
  } else if (state == 3) {
    if (motion() == 1) {
      state = 0;
    }

  // if the unit has determined it's state, stop sensing
  } else if (state != 0) {

    // if toilet brush returns, no longer in cleaning mode
    if (motion() == 1){
      state = 0;
    }
                // 'in use' LED mode
                
  } else {
    senseUseType();
  }
}

int temp() {
  sensors.requestTemperatures();
  return sensors.getTempCByIndex(0);
}
bool motion() {
  return digitalRead(motionPin);
}

int light() {
  return analogRead(lightPin);
}

// adapted from Rui Santos, https://randomnerdtutorials.com/complete-guide-for-ultrasonic-sensor-hc-sr04/
// distance measured in cm
int distance(){
    digitalWrite(trigPin, LOW);
    delayMicroseconds(5);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    return (pulseIn(echoPin, HIGH)/2)/29.1;
}

int senseToiletFlush() {
  int start = millis(); 
  // 0 == contact, 1 == no contact
  while (digitalRead(contactPin) == 0) {
    // nop; burn millis counter
  }
  int endTime = millis();
  int duration = endTime - start;
                                            // debugging
                                            //  if (duration > 3){
                                            //    Serial.print("duration is: ");
                                            //    Serial.println(duration);
                                            //  }
  return duration;
}

void senseUseType() {
  if (motion()) {
    state = 3;
    printState(3);
  }
  else if (flushDuration > 1000) {
    // poop
    printState(2);
    spray(2);
  }
  else if (flushDuration > 100){
    // pee
    printState(1);
    spray(1);
  } else {
  // if no toilet brush movement nor flush, go back to base state.
    state = 0;
  }
}

void spray(int shots){
    
                                       // delay based on preference set by user should go here too
                                       // this is something that we could/should probably use with EEPROM
                                       
  // there was no flush: do not spray, this was a misread
  if (shots == 1){
    Serial.println("SPRAY 1");
    digitalWrite(mostfetPin, HIGH);
    // delay for 1s, but don't actually delay for 1s??
    delay(1000);
    digitalWrite(mostfetPin, LOW);
  } else if (shots == 2){
    Serial.println("SPRAY 2");
    digitalWrite(mostfetPin, HIGH); // x2 - how to do this? hold for 1s, rest 1s, hold for 1s again?
    delay(1000);
    digitalWrite(mostfetPin, LOW);
    delay(1000);
    digitalWrite(mostfetPin, HIGH);
    delay(1000);
    digitalWrite(mostfetPin, LOW);
  }
}

bool isSprayBlocked(){
  if (distance() < 10) {
    // angry blink leds 
    printState(7);
  }
}
void LEDstate() {
  if (state == 6) {
    blink();  
  } else if (sprayShots < 500) {
    analogWrite(redPin, 255);
    analogWrite(greenPin, 0);
  } else if (state != 0) {
    analogWrite(greenPin, 255);
    analogWrite(redPin, 0);
  } else if (state == 0) {
    analogWrite(greenPin, 0);
    analogWrite(redPin, 0);
  }
}

void blink() {

  if (!timerSet[0]) {
    
    timerSet[0] = true;
    timer[0] = millis();
  }
  
  if (wait(0, 1000)) {
    
    analogWrite(greenPin, greenValue);
    
    if (greenValue == 0) {
      greenValue = 255;
    } else if (greenValue == 255) {
      greenValue = 0;
    }
  }
}

bool wait(int timerNum, int interval) {
  
  if (millis() - timer[timerNum] >= interval) {
    
    timerSet[timerNum] = false;
    return true;
    
  } else {
    
    return false;
  }
}

// for debugging
void printState(int state) {
  Serial.print("light is: ");
  Serial.println(light());
  Serial.print("motion is: ");
  Serial.println(motion());
  Serial.print("distance is: ");
  Serial.println(distance());
  Serial.print("flushTime is: ");
  Serial.println(flushDuration);
  Serial.print("state is: ");
  Serial.println(states[state]);
  Serial.println("####################");
}
 
//Interupt for immediate spray. Checks to see if bounced, then sprays if it has not.
void sprayISR() {

  unsigned long currInterupt = millis();
  if (currInterupt - sprayInterupt >= 50) {
    spray(1);
    state = 6;
  }
  sprayInterupt = currInterupt;
}

//Interrupt for switching to menu state or selecting menu options. Will either show menu options or execute the
//task depending on state.
void menuISR() {

  unsigned long currInterupt = millis();
  if (currInterupt - menuInterupt >= 50) {
    if (state != 4) {
      state = 4;
      
    } else {
      spray(1);
    }
  }
  menuInterupt = currInterupt;
}
