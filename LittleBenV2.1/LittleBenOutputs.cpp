#include "LittleBenOutputs.h"

LittleBenOutput::LittleBenOutput(byte ID, String Name) {
  this->id = ID;
  this->name = Name;
  this->type = 0;
}

String LittleBenOutput::GetName() {
  return name;
}

String LittleBenOutput::GetTypeName() {
  return typeName[type];
}

void LittleBenOutput::SetTypeValue(byte value) {
  switch(type) {
    case 0: //Clock
      clockDivider += value;
      if(clockDivider == 0) {clockDivider =1;}
      break;
    case 1: //Beat
      beatCountDivider += value;
      if(beatCountDivider == 0) {beatCountDivider =1;}
      break;
    case 2: //Random
      randomRange += value;;
      if(randomRange == 255) {randomRange = 0;}
      if(randomRange >= 101) {randomRange = 100;}
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
        snprintf(s, sizeof(s), "Every %i Clockpulses", clockDivider);
        return s;
      }
      break;
    case 1: //Beat
      if(beatCountDivider == 1) {
        return "Every Beat"; 
      } else {
        snprintf(s, sizeof(s), "Every %i Beats", beatCountDivider);
        return s;
      }
      break;
    case 2: //Random
      snprintf(s, sizeof(s), "Percent %i", randomRange);
      return s;
      break;
  }
}

void LittleBenOutput::SetType(byte value) {
  type += value; // add value to the type
  
  // check for out of range items
  if(type == 255) {
    type = 2;
  }
  if(type >= 3) {
    type = 0; // clock
  }

//  if (type == 0) {
//    outputbit = 1; // set outbit as true
//  }
}

void LittleBenOutput::Pulse(byte ppqn) {
  switch(type) {
    case 0: //Clock
      pulseClockDivider(ppqn);
      break;
    case 1: //Beat
      pulseBeat(ppqn);
      break;
    case 2: //Divider
      pulseRandom();
      break;
  } 
}

void LittleBenOutput::pulseClockDivider(byte ppqn) {
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
