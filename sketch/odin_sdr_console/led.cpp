/////////////////////////////////////////////////////////////////////////
//
// Odin SDR Console by Laurence Barker G8NJJ
//
// this sketch provides a knob and switch interface through USB and CAT
//
// LED.c
// this file holds the code to control 8 LED indicators
/////////////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include "globalinclude.h"
#include "mechencoder.h"
#include "led.h"
#include "iopins.h"
#include "types.h"



//
// array of I/O pins
//
int LEDPinList[] = 
{
  VPININDICATOR1,
  VPININDICATOR2,
  VPININDICATOR3,
  VPININDICATOR4,
  VPININDICATOR5,
  VPININDICATOR6,
  VPININDICATOR7
};



//
// note LEDs numbered 0-7 here!
//
void SetLED(unsigned int LEDNumber, bool State)
{
  if (LEDNumber < VMAXINDICATORS)
  {
    if (State == true)
      digitalWrite(LEDPinList[LEDNumber], HIGH);
    else
      digitalWrite(LEDPinList[LEDNumber], LOW);
  }
}


//
// clear all LEDs
//
void ClearLEDs(void)
{
  int Cntr;

  for (Cntr = 0; Cntr < VMAXINDICATORS; Cntr++)
    SetLED(Cntr, false);
}
