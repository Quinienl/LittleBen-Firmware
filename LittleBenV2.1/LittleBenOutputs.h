#ifndef LITTLEBENOUTPUTS_H
#define LITTLEBENOUTPUTS_H
#include <Arduino.h>

class LittleBenOutput {
  
  private:
   
    void pulseClockDivider();
    void pulseBeat(byte ppqn);
    void pulseRandom();

    byte id;  // ID of the output
    bool outputbit = 1; // 0 or 1 to output 

    String name;

    byte type; // Clock 0, Beat 1, Random 2
    String typeName[3] = { "Clock", "Beat", "Random"};

    byte ppqnCount;

    byte clockDividerCount;
    byte clockDivider = 1; 

    byte beatCount;
    byte beatCountDivider = 1;

    byte randomRange = 50;

  public:

    // Functions
    LittleBenOutput(byte id, String outputName );

    void Pulse(byte ppqn);
    bool GetOutputBit();
    
    String GetName();
    
    String GetTypeName();
    String GetTypeValueText(); 
    
    void SetType(int value);
    void SetTypeValue(int value);

    void resetCounters(); // Reset all counter used when LittleBen is stopped.
};
#endif
