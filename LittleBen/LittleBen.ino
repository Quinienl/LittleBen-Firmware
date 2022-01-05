/* 
 *  LittleBen
 *  
 *  A master clock for eurorack systems
 *  By: Quinie.nl
 *  
 */

// Includes needed for running this code.
// TimerOne will handle the timing of the pulses
// TM1637 need for screen functions

#include <TimerOne.h>  
#include <TM1637.h>

//Clock Settings
byte statusClock = 1; // 0 = stopped, 1 = playing, 2 = pause
long intervalMicroSeconds;  // variable for interval for timer running clock pulses calculted on BPM value

// Calculate Clock Variables
long clockPeriodes[8]; // storing periodes between clock incomming 
int maxPeriodeTimeout =  30000; // max time between clock pulse before starting it as new calculation
long clockLastTime = 0; // last time a clock was recieved
long clockCurrentTime = 0; // current clock time
byte clockNumberOfReadings = 0; // number of readings incomming clock
byte clockCurrentIndex = 0; // current number reading
byte clockCounter = 0; // used for counter function output

// These are used to output the clock on the 595 chip
// The bit are linked to the ouput as 2,4,6,8,1,3,5,7
byte outputClockBits = B00001111; 
byte outputResetBits = B11110000;
byte outputDividerBits =  B00000000;

// Variables for the clock divider
byte dividerCounter[8] {0,0,0,0,0,0,0,0}; // counters for the clock divider
byte divider[8] {2,6,3,1,16,12,4,2}; // Dividers (see bit linked read reversed 7,5,3,1,8,6,4,2)

//{2,4,8,12,1,3,6,16}

float bpm = 126;  // BPM value 
byte pulsePerBeat = 1; // Pulse per beat
byte maxPulsePerBeat = 48; // maximum pulse per beat (too large will result in wierd stuff)
byte beatCounter = 0; // Counter for the beats
byte resetOnBeat = 16; // Reset on beat 
String clockType[7] = { "INT ", "OUT ", "CALC", "RAND", "COUN", "CLIN", "CLOU" }; // String values for screen clocktype
byte currentClockType = 0; // current clock type


#define TM1637_DISPLAY // Dipslay
#define TM1637_CLK_PIN 10 // Display Clock pin
#define TM1637_DIO_PIN 11 // Display Date pin 
#define TM1637_BRIGHTNESS 1 // Brightness of display (0-10)

#define startTriggerPin 3 // Start trigger pin
#define stopTriggerPin 2 // Stop trigger pin

// 595 output pins 
byte outputLatchPin = 5; 
byte outputClockPin = 6; 
byte outputDataPin = 4;  

// Rotary Encoder Inputs pins
#define inputCLK 9
#define inputDT 8
#define inputSW 7

// Current en previous states of rotary pins
byte currentStateSW; 
byte previousStateSW;
byte currentStateCLK;
byte previousStateCLK; 

// Current menu item
byte menuItem = 0;

// Display timer variables to show values like play, pauze and stop
long displayTimer;
int displayTimeOut = 500; // how long to show value before returning to screen
byte brightnessStop = 1;
byte brightnessStart = 4;
byte brightnessPause = 4;
byte brightnessOuterClock = 4;
String displayOuterClockState[4] = { "____", "----", "||||", "++++" };
byte displayOuterClockIndex = 0;

TM1637 tm(TM1637_CLK_PIN, TM1637_DIO_PIN);

void setup() {
  // Set timer for internal clock
  Timer1.initialize(intervalMicroSeconds);
  Timer1.setPeriod(calculateIntervalMicroSecs());
  Timer1.attachInterrupt(ClockPulseInternal);
  
  attachInterrupt(digitalPinToInterrupt(stopTriggerPin), stopTrigger, FALLING );
  attachInterrupt(digitalPinToInterrupt(startTriggerPin), pauseTrigger, FALLING );

  // Set all the pins of 74HC595 as OUTPUT
  pinMode(outputLatchPin, OUTPUT);
  pinMode(outputDataPin, OUTPUT);  
  pinMode(outputClockPin, OUTPUT);

  // Rotary inputs
  pinMode (inputCLK,INPUT_PULLUP);
  pinMode (inputDT,INPUT_PULLUP);
  pinMode (inputSW,INPUT); 
  // set state of rotary
  previousStateCLK = digitalRead(inputCLK);
  previousStateSW = digitalRead(inputSW);

  // Display timer start time
  displayTimer = millis();

  // Start display
  tm.begin();
  tm.setBrightness(TM1637_BRIGHTNESS);
  displayBPMBasedOnStateClock();
}

void loop() {
  CheckRotarySwitch(); // Check Rotary Switch
  CheckRotary(); // Check Rotary 
}

// Check if rotary switch has been pushed
// Set the menuState
void CheckRotarySwitch() {
   currentStateSW = digitalRead(inputSW);
   if (currentStateSW != previousStateSW && currentStateSW == 0) {
      if(menuItem == 4) {
        menuItem = 0;
      } else {
        menuItem++;
      }
      displayMenuItem();
   }
   previousStateSW = currentStateSW; 
}


// Check if rotary is turned
// Update menu item
void CheckRotary() {
   currentStateCLK = digitalRead(inputCLK);
   
   if (currentStateCLK != previousStateCLK && currentStateCLK == 0){   
      switch(menuItem) {
        case 0:     
          UpdateBPM();
          break;
        case 1:
          UpdateBPMDecimal();
          break;
        case 2:
          UpdateClockType();
          break;
        case 3:
          UpdatePulsePerBeat();
          break;
        case 4:
          UpdateResetOnBeat();         
          break;
     }
   }   
   previousStateCLK = currentStateCLK;
}

// Rotary updates BPM 
void UpdateBPM() {
  if (digitalRead(inputDT) != currentStateCLK) { 
     bpm++; 
   } else {
     bpm--;
   }
   displayBPMBasedOnStateClock();
   updateTimer();
}

// Updates BPM Decimal
void UpdateBPMDecimal() {
  if (digitalRead(inputDT) != currentStateCLK) { 
     bpm += 0.1; 
   } else {
     bpm -=0.1;
   }
   displayBPMBasedOnStateClock();
   updateTimer();
}

// Updates pulse per beat
void UpdatePulsePerBeat() {
  if (digitalRead(inputDT) != currentStateCLK) { 
     if(pulsePerBeat != maxPulsePerBeat) {pulsePerBeat++;}
   } else {
     if(pulsePerBeat != 1) {pulsePerBeat--;}
   }
   displayPulsePerBeat();
   updateTimer();
}

// Updates reset on beat
void UpdateResetOnBeat() {
  if (digitalRead(inputDT) != currentStateCLK) { 
     if(resetOnBeat != 255) {resetOnBeat++;}
   } else {
     if(resetOnBeat != 1) {resetOnBeat--;}
   }
   displayResetOnBeat();
}

// Update the type of clock
void UpdateClockType() {
  if (digitalRead(inputDT) != currentStateCLK) { 
     if(currentClockType != 6) {currentClockType++;}
   } else {
     if(currentClockType != 0) {currentClockType--;}
   }
   displayClockType();
}

void displayPulsePerBeat() {
  setDisplayValueInt(pulsePerBeat);
}

void displayResetOnBeat() {
  setDisplayValueInt(resetOnBeat);
}

void displayClockType() {
  tm.display(clockType[currentClockType]);
}

void setDisplayValueInt(int value) {
  byte empty = 0;
  if(value < 10) {
    empty++;
  }
  if(value < 100) {
    empty++;
  }
  if(value < 1000) {
    empty++;
  }
  tm.display(value, false, false,empty);
}

// Display BPM based on current clock state
void displayBPMBasedOnStateClock() {
  if(currentClockType == 1) {
    displayBPMBasedOnStateClockOuter();
  } else {
    displayBPMBasedOnStateClockInternal();
  }
}

void displayBPMBasedOnStateClockOuter() {
  tm.changeBrightness(brightnessOuterClock);
  tm.display(displayOuterClockState[displayOuterClockIndex]);
  displayOuterClockIndex++;
  if(displayOuterClockIndex == 4){
    displayOuterClockIndex = 0;
  }
}

void displayBPMBasedOnStateClockInternal(){
  switch(statusClock) {
     case 0: // Stop
      tm.changeBrightness(brightnessStop);
      tm.display(bpm);
      break;
     case 1: // Start
      tm.changeBrightness(brightnessStart);
      tm.display(String(bpm,1))->scrollLeft(500);
      break;
     case 2: // Pause
      tm.changeBrightness(brightnessPause);
      tm.display(bpm);
      break;
    }
}
// Update the timer
void updateTimer() {
  Timer1.setPeriod(calculateIntervalMicroSecs());
}

// Clock comes from internal
void ClockPulseInternal() {
  switch(currentClockType) {
    case 0: // Internal
      if (statusClock == 1) {
        ClockPulse();
      }
      break;
    case 2: // Calculated clock
      ClockPulse();
      break;
    case 3: // Random
      if (statusClock == 1) {
        ClockPulseRandom();
      }
      break;
    case 4: // Count
      if (statusClock == 1) {
        ClockPulseCount();
      }
      break;
    case 5: // Divider Internal
      if (statusClock == 1) {
        ClockPulseDivider();
      }
      break;
  }  
    
  // DisplayTimer is updated (in loop not)
  if (displayTimer < millis()) {
     displayMenuItem();
  }
}

// Clock pulse comes from outside
void ClockPulseOuter() {
  switch(currentClockType) {
  case 1: // Outer - External
    ClockPulse();
    displayMenuItem();
    break;
  case 6: // Outer divider
    ClockPulseDivider();
    displayMenuItem();
    break;
  }
}

// Clock pulse comes from outside will calculate avarge bpm.
void ClockPulseCalculate() {

  long periode;
   
  if(clockLastTime == 0) {
    clockLastTime = clockCurrentTime;
    clockNumberOfReadings++;
  } else {
    periode = clockCurrentTime - clockLastTime;
    if(periode >= maxPeriodeTimeout){
      clockLastTime = clockCurrentTime;
      clockNumberOfReadings = 1;
      clockCurrentIndex = 0;
    } else {
      clockPeriodes[clockCurrentIndex] = periode;
      clockLastTime = clockCurrentTime;
      clockCurrentIndex++;
      if(clockCurrentIndex == 8) {
        clockCurrentIndex = 0;
      }
      clockNumberOfReadings++;
    }
  }
  if(clockNumberOfReadings >= 8) {
    calculateClock();
    statusClock = 1;
  }
}

void calculateClock() {
  long total = 0;
  long avg;
  long bpmtemp;
  
  for (int i = 0; i < 8; i++) {
    total = total + clockPeriodes[i];
  }
  avg = total /8L;
  bpmtemp = (6000000L / avg); 
  bpm = (int)bpmtemp/100;
  updateTimer();
}

// send a random clock value out.
void ClockPulseRandom() {
  outputClock(random(0, 256));
  delay(4);
  outputClock(B00000000);
}

// send a counter clock value out.
void ClockPulseCount() {
  outputClock(clockCounter);
  delay(4);
  outputClock(B00000000);
  
  clockCounter++;
}


// send a counter clock value out.
void ClockPulseDivider() {
  //loop dividers and counters
  for (int i = 0; i < 8; i++) {
    dividerCounter[i]++;
    // if counter and divider are equal
    if(dividerCounter[i] == divider[i]) {
      dividerCounter[i] = 0; // counter back to 0
      bitSet(outputDividerBits, i); // set the bit to true in the output
    }
  }
 
  outputClock(outputDividerBits);
  delay(4);
  outputClock(B00000000);
  outputDividerBits = B00000000; // reset bits to 0
}

void resetDividers() {
  for (int i = 0; i < 8; i++) {
    dividerCounter[i] = 0;
  }
}


// 
void ClockPulse() {
  
  beatCounter++;
    
  outputClock(outputClockBits);
  delay(4);
  outputClock(B00000000);

  if(beatCounter >= (resetOnBeat*pulsePerBeat) ) {
    sendResetOut();
  }
}

void sendResetOut() {  
  beatCounter = 0;
  outputClock(outputResetBits);
  delay(4);
  outputClock(B00000000);  
}

void displayMenuItem() {
  switch(menuItem) {
    case 0:     
      displayBPMBasedOnStateClock();
      break;
    case 1:
      displayBPMBasedOnStateClock();
      break;
    case 2:
      displayClockType();
      break;
    case 3:
      displayPulsePerBeat(); 
      break;
    case 4:
      displayResetOnBeat();
      break;
  }
}

// Output the clock/reset
void outputClock(byte pins)
{
   digitalWrite(outputLatchPin, LOW);
   shiftOut(outputDataPin, outputClockPin, LSBFIRST, pins);
   digitalWrite(outputLatchPin, HIGH);
}

// Stop the clock
void stopTrigger() {
  switch(currentClockType) {
    case 0: // Internal
      displayStop();
      statusClock = 0;
      beatCounter = 0;
      sendResetOut();
      break;
    case 3: // Random
      displayStop();
      statusClock = 0;
      beatCounter = 0;
      sendResetOut();
      break;
    case 4: // Counter
      displayStop();
      statusClock = 0;
      beatCounter = 0;
      sendResetOut();
      break;
    case 5: // Internal Divider
      displayStop();
      statusClock = 0;
      sendResetOut();
      resetDividers();
      break;
  }
}

// Pause the clock
void pauseTrigger() {
  clockCurrentTime = millis();
  switch(currentClockType) {
    case 0: // Internal
       displayPauseOrPlaySetStatusClock();
       break;
    case 1: // Outer Clock
       ClockPulseOuter();
       break;
    case 2: // Calculate Clock
       ClockPulseCalculate();
       break;
    case 3: // Random
       displayPauseOrPlaySetStatusClock(); 
       break;
    case 4: // Counter
       displayPauseOrPlaySetStatusClock(); 
       break;
    case 5: // Internal Divider
       displayPauseOrPlaySetStatusClock(); 
       break;
    case 6: // Outer Divider
       ClockPulseDivider(); 
       break;
  }  
}

void displayStop() {
  tm.display("STOP");
  displayTimer = millis() + displayTimeOut;
}

void displayPauseOrPlaySetStatusClock(){
  if(statusClock == 1){ 
    statusClock = 2;
    tm.changeBrightness(brightnessPause);
    tm.display("PAUS");
  } else {
    statusClock = 1;
    tm.changeBrightness(brightnessStart);
    tm.display("PLAY");
  }
  displayTimer = millis() + displayTimeOut;
}

long calculateIntervalMicroSecs() {
  return 60L * 1000 * 1000 / bpm / pulsePerBeat;
}
