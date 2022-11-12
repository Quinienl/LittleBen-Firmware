#include "LittleBenOutputs.h"

LittleBenOutput::LittleBenOutput(byte ID, String Name) {
  this->id = ID;
  this->name = Name;
  this->type = 0;
}

String LittleBenOutput::GetName() {
  return name;
}

byte LittleBenOutput::GetType() {
  return type;
}
String LittleBenOutput::GetTypeName() {
  return typeName[type];
}


byte LittleBenOutput::GetclockDivider() {
  return clockDivider;
}
byte LittleBenOutput::GetbeatCountDivider() {
  return beatCountDivider;
}
byte LittleBenOutput::GetrandomRange() {
  return randomRange;
}

void LittleBenOutput::SetType(int value) {
  int tmp = type + value;; // integer temp value for calulations and suppressing bufferoverflow of bytes
  
  // check for out of range items
  if(tmp < 0) { type = 2;} // random (last)
  else if(tmp >= 3) { type = 0;} // clock (first)
  else {type = tmp;} // in between
  
}

void LittleBenOutput::SetclockDivider(byte value) {
  clockDivider = value;
}

void LittleBenOutput::SetbeatCountDivider(byte value) {
  beatCountDivider = value;
}

void LittleBenOutput::SetrandomRange(byte value) {
  randomRange = value;
}


void LittleBenOutput::SetTypeValue(int value) {
  int tmp; // integer temp value for calulations and suppressing bufferoverflow of bytes
  switch(type) {
    case 0: //Clock
      tmp = clockDivider + value;
      if(tmp <= 0) {clockDivider = 1;}
      else if(tmp >= 255) {clockDivider = 255;}
      else {clockDivider = tmp;}
      break;
    case 1: //Beat
      tmp = beatCountDivider + value;
      if(tmp <= 0) {beatCountDivider = 1;}
      else if(tmp >= 255) {beatCountDivider = 255;}
      else {beatCountDivider = tmp;}
      break;
    case 2: //Random
      tmp = randomRange + value;
      if(tmp <= 0) {randomRange = 1;}
      else if(tmp > 100) {randomRange = 100;}
      else {randomRange = tmp;}
      break;
  }
}

// Reset all counter used when LittleBen is stopped.
void LittleBenOutput::resetCounters() {
  clockDividerCount = 0;
  beatCount = 0;
  ppqnCount = 0;
}

String LittleBenOutput::GetTypeValueText() {
  char s[180];
  
  switch(type) {
    case 0: //Clock
      if(clockDivider == 1) {
        return "Every Clock"; 
      } else {
        sprintf(s, "Every %i Clockpulses", clockDivider);
        return s;
      }
      break;
    case 1: //Beat
      if(beatCountDivider == 1) {
        return "Every Beat"; 
      } else {
        sprintf(s, "Every %i Beats", beatCountDivider);
        return s;
      }
      break;
    case 2: //Random
      sprintf(s, "Percent %i", randomRange);
      return s;
      break;
  }
  return ""; // empty value if no case is true. This will never be reached if everything is ok. But will supress warning in compiler.
}

void LittleBenOutput::Pulse(byte ppqn) {
  switch(type) {
    case 0: //Clock
      pulseClockDivider();
      break;
    case 1: //Beat
      pulseBeat(ppqn);
      break;
    case 2: //Divider
      pulseRandom();
      break;
  } 
}

void LittleBenOutput::pulseClockDivider() {
    clockDividerCount++; // count
    // if count is over ppqn set back to zero
    if(clockDividerCount >= clockDivider ) {
      outputbit = 1;
      clockDividerCount = 0;
    } else {
      outputbit = 0;
    }
}

void LittleBenOutput::pulseBeat(byte ppqn) {
    ppqnCount++; // count
    // if count is over ppqn set back to zero
    if(ppqnCount >= (ppqn+1) ) {
      ppqnCount = 0;
      beatCount++;
    }
    
    if (beatCount >= beatCountDivider) {
      outputbit = 1;
      beatCount = 0;
    } else {
      outputbit = 0;
    }
}

void LittleBenOutput::pulseRandom() {
  byte rnd = random(0, 101); // select number between 0 and 100
  outputbit = 0; // set bit to zero for no output
  
  if(rnd <= randomRange) {
    outputbit = 1; // if random number is smaller or equal to range number set bit to output true
  }
}

bool LittleBenOutput::GetOutputBit() {
  return outputbit;
}
