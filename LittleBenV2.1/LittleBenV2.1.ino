/* 
 *  LittleBen
 *  
 *  A master clock for eurorack systems
 *  By: Quinie.nl
 *  
 */

// Includes needed for running this code.
// TimerOne will handle the timing of the pulses

#include <TimerOne.h>  
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "LittleBenOutputs.h"

int const maxLBOutput = 8;
// Load the outputs as class
LittleBenOutput LBOutputs[maxLBOutput] = {
  LittleBenOutput(0, "One"),
  LittleBenOutput(1, "Two"),
  LittleBenOutput(2, "Three"),
  LittleBenOutput(3, "Four"),
  LittleBenOutput(4, "Five"),
  LittleBenOutput(5, "Six"),
  LittleBenOutput(6, "Seven"),
  LittleBenOutput(7, "Eight")
};

byte selectedLBOutput = 0;

//Clock Settings
byte statusClock = 1; // 0 = stopped, 1 = playing, 2 = pause
String statusClockText[3] = { "STOP", "PLAY", "PAUSE"};
byte clockSource = 0; // 0 = internal, 1 = external
String clockSourceText[2] = { "internal", "external"};

long intervalMicroSeconds;  // variable for interval for timer running clock pulses calculted on BPM value

//// Calculate Clock Variables
//long clockPeriodes[9]; // storing periodes between clock incomming 
//int maxPeriodeTimeout =  30000; // max time between clock pulse before starting it as new calculation
//long clockLastTime = 0; // last time a clock was recieved
//long clockCurrentTime = 0; // current clock time
//byte clockNumberOfReadings = 0; // number of readings incomming clock
//byte clockCurrentIndex = 0; // current number reading
//byte clockCounter = 0; // used for counter function output

// These are used to output the clock on the 595 chip
// The bit are linked to the ouput as 2,4,6,8,1,3,5,7 <-- note: double check note sure
byte outputClockBits = B00000000; 

float bpm = 126;  // BPM value 
byte ppqn = 24; // Pulse per beat

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

//Oled display settings
#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 32 
#define OLED_RESET     4 
#define SCREEN_ADDRESS 0x3C

bool updateScreen = false;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

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


  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
  // Serial.println(F("SSD1306 allocation failed"));
  for(;;); // Don't proceed, loop forever
  }
  
  display.clearDisplay();
  display.setRotation(2); //set the display to rotate 180 degrees
  //display.dim(true); //dim the display to 50%, better to avoid noise  

  display.setTextSize(2);       
  display.setTextColor(SSD1306_WHITE);

  printScreenDefault();

}

void loop() {
  CheckRotarySwitch(); // Check Rotary Switch
  CheckRotary(); // Check Rotary 

  // Check is screen needs update
  if(updateScreen == true) { 
    // if menu is not 4 it is not the default menu
    if(menuItem <= 2) {
      printScreenDefault();
    } else {
      printScreenOutputs();
    }
    updateScreen= false;
  }
}

// Check if rotary switch has been pushed
// Set the menuState
void CheckRotarySwitch() {
   currentStateSW = digitalRead(inputSW);
   if (currentStateSW != previousStateSW && currentStateSW == 0) {
      if(menuItem == 5) {
        menuItem = 0;
      } else {
        menuItem++;
      }
      updateScreen = true;
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
          UpdateClockSource();
          break;
        case 3:
          UpdateOutputSelector();
          break;
        case 4:
          UpdateOutputType();         
          break;
        case 5:
          UpdateOutputTypeValue();         
          break;
     }
   }   
   previousStateCLK = currentStateCLK;
}

// Rotary updates BPM 
void UpdateBPM() {
  if(clockSource == 0) { // only update if clock is internal
    if (digitalRead(inputDT) != currentStateCLK) { 
       bpm++; 
     } else {
       bpm--;
     }
     updateTimer();
     updateScreen = true;
  }
}

// Updates BPM Decimal
void UpdateBPMDecimal() {
  if(clockSource == 0) { // only update if clock is internal
    if (digitalRead(inputDT) != currentStateCLK) { 
       bpm += 0.1; 
     } else {
       bpm -=0.1;
     }
     updateTimer();
     updateScreen = true;
  }
}

// Update the type of clock
void UpdateClockSource() {
   clockSource = !clockSource;
   updateScreen = true;
}

void UpdateOutputSelector() {
  int tmp = selectedLBOutput; // integer temp value for calulations and suppressing bufferoverflow of bytes
  if (digitalRead(inputDT) != currentStateCLK) { 
     tmp++; 
   } else {
     tmp--;
   }
   
   if(tmp < 0) {
      selectedLBOutput = 7;
   } else if(tmp >= 8) {
      selectedLBOutput = 0;
   } else {
      selectedLBOutput = tmp;
   }
   updateScreen = true;
}

void UpdateOutputType() {
  if (digitalRead(inputDT) != currentStateCLK) { 
     LBOutputs[selectedLBOutput].SetType(1); // add 1
   } else {
     LBOutputs[selectedLBOutput].SetType(-1);
   }
   updateScreen = true;
}

void UpdateOutputTypeValue() {
  if (digitalRead(inputDT) != currentStateCLK) { 
     LBOutputs[selectedLBOutput].SetTypeValue(1); // add 1
   } else {
     LBOutputs[selectedLBOutput].SetTypeValue(-1);
   }
   updateScreen = true;
}

// Update the timer
void updateTimer() {
  Timer1.setPeriod(calculateIntervalMicroSecs());
}

// Clock comes from internal
void ClockPulseInternal() {
  // Check Clock for play state and internal
  if((statusClock == 1) && (clockSource == 0) ){ 
    ClockPulse();
  }
}

// Clock pulse comes from outside
void ClockPulseOuter() {
  //ClockPulseCalculate();
  ClockPulse();
}

// Clock pulse external will calculated to bpm.
// Buggy Rebuild
//----------------
void ClockPulseCalculate() {

//  long periode;
//  
//  if(clockLastTime == 0) {
//    clockLastTime = clockCurrentTime;
//    clockNumberOfReadings++;
//  } else {
//    periode = clockCurrentTime - clockLastTime;
//    if(periode >= maxPeriodeTimeout){
//      clockLastTime = clockCurrentTime;
//      clockNumberOfReadings = 1;
//      clockCurrentIndex = 0;
//    } else {
//      clockPeriodes[clockCurrentIndex] = periode;
//      clockLastTime = clockCurrentTime;
//      
//      clockCurrentIndex++;
//      if(clockCurrentIndex == 8) {
//        clockCurrentIndex = 0;
//      }
//      
//      clockNumberOfReadings++;
//    }
//  }
//  if(clockNumberOfReadings >= 8) {
//    calculateClock();
//  }
}

//void calculateClock() {
//  long total = 0;
//  long avg;
//  long bpmtemp;
//  float test = 100;
//  
//  for (int i = 0; i < 8; i++) {
//    total += clockPeriodes[i];
//  }
//  avg = total /8L;
//  bpmtemp = (6000000L / avg) /100; 
//  bpm = clockLastTime;
//  
//  updateScreen = true;
//}

void ClockPulse() {
  // run thru outputs
  for (int i = 0; i < maxLBOutput; i++) {
    LBOutputs[i].Pulse(ppqn); // Call pulse function on outputs
    bitWrite(outputClockBits, i, LBOutputs[i].GetOutputBit()); // set the output byte based on output settings
  }
  
  //output
  outputClock(outputClockBits);
  delay(4);
  outputClock(B00000000);
}


// Output the clock
void outputClock(byte pins)
{
   digitalWrite(outputLatchPin, LOW);
   shiftOut(outputDataPin, outputClockPin, LSBFIRST, pins);
   digitalWrite(outputLatchPin, HIGH);
}

// Stop the clock
void stopTrigger() {
  if(clockSource == 0){ 
    statusClock = 0;
    updateScreen = true;
  }
  // Loop thru outputs
  for (int i = 0; i < maxLBOutput; i++) {
    LBOutputs[i].resetCounters(); // Reset Counters
  }
}

// Input pulse on pause button/jack
// Pause the clock or other function depending on the clock source setting of Littleben
void pauseTrigger() {
  if(clockSource == 0){ 
    togglePauseOrPlay();
  }
  if(clockSource == 1){ 
    ClockPulseOuter();
  }
}

void togglePauseOrPlay(){
  if(statusClock == 1){ 
    statusClock = 2;
  } else {
    statusClock = 1;
  } 
  updateScreen = true;
}

long calculateIntervalMicroSecs() {
  return 60L * 1000 * 1000 / bpm / ppqn;
}

//Prints the clock status to the screen
void printScreenStatus() {
    display.setCursor(0,0);    
    display.setTextSize(1);      
    display.print(statusClockText[statusClock]); 

    display.setCursor(50,22);    
    display.setTextSize(1);      
    display.print(clockSourceText[clockSource]); 
}


// prints the BPM to the screen
void printScreenBPM() {
    display.setCursor(50,0);
    display.setTextSize(2);  
    // Internal BPM
    if(clockSource == 0){ 
      if(bpm<100){
          display.print(F(" "));
      } 
      display.print(bpm);
    }
    if(clockSource == 1){ 
      display.print("---.--");
    }
}

void printScreenMenuSelection() {
    switch(menuItem) {
      case 0: //Clock
        display.drawLine(52, 15, 86, 15, WHITE); // BPM
        break;
      case 1:
        display.drawLine(98, 15, 120, 15, WHITE); // BPM Decimal
        break;
      case 2:
        display.drawLine(52, 30, 120, 30, WHITE); // ClockSource
        break;
      case 3:
        display.drawLine(0, 15, 60, 15, WHITE); // OutputName, Output selector
        break;
      case 4:
        display.drawLine(70, 8, 120, 8, WHITE); // OutputType, Clock, Beat, Random
        break;
      case 5:
        display.drawLine(0, 31, 130, 31, WHITE); // OutputType Value
        break;
    }
}

// Print the display 
void printScreenDefault() {
    display.clearDisplay();
  
    printScreenStatus();
    printScreenBPM();
    printScreenMenuSelection();

    display.display();
}

void printScreenOutputs() {
    display.clearDisplay(); 
    printScreenOutputName();
    printScreenOutputType();
    printScreenOutputTypeValue();
    printScreenMenuSelection();
    display.display();
}

void printScreenOutputName() {
    display.setCursor(0,0);    
    display.setTextSize(2);      
    display.print(LBOutputs[selectedLBOutput].GetName()); 
}

void printScreenOutputType() {
    display.setCursor(70,0);    
    display.setTextSize(1);      
    display.print(LBOutputs[selectedLBOutput].GetTypeName()); 
}

void printScreenOutputTypeValue() {
    display.setCursor(0,23);    
    display.setTextSize(1);      
    display.print(LBOutputs[selectedLBOutput].GetTypeValueText()); 
}
