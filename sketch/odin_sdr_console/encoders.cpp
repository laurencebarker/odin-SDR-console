/////////////////////////////////////////////////////////////////////////
//
// Odin SDR Console by Laurence Barker G8NJJ
//
// this sketch provides a knob and switch interface through USB and CAT
//
// encoder.c
// this file holds the code to manage the rotary encoders
// it looks like it needs two technologies:
// interrupt driven code for optical VFO encoder (apparenrtly largely bounce free)
// polled code for very bouncy mechanicsl encoders for other controls
/////////////////////////////////////////////////////////////////////////

#include "MechEncoder.h"
#include <Arduino.h>
#include <Encoder.h>
#include "types.h"
#include "encoders.h"
#include "iopins.h"
#include "display.h"
#include "configdata.h"
#include "cathandler.h"


//
// note switch and encoder numbering:
// in the software switches are numbered 0-20, and encoders 0-7. The VFO encoder is treated separately.
// these correspond to the control of Kjell's PCB as follows:
//
// encoder numbering:
//    PCB   software
//    VFO    (treated separately)
//    2A      0
//    2B      1
//    3A      2
//    3B      3
//    4A      4
//    4B      5
//    5A      6
//    5B      7


//
// global variables
//
byte EncoderGroup;                                  // sets which ones are updated every ms
EEncoderActions GMultiAction;                       // assigned action for "multifunction"

//
// 8 encoders: one VFO (fast) encoder and 7 "normal" ones 
//
Encoder VFOEncoder(VPINVFOENCODERA, VPINVFOENCODERB);

long old_ct;

struct  EncoderData                           // holds data for one slow encoder
{
  NoClickEncoder* Ptr;                          // ptr to class
  int16_t LastPosition;                       // previous position
};

  EncoderData EncoderList[VMAXENCODERS];

//
// variables to say whether encoders have main action or 2nd action, if dual function
// false if main function, true if 2nd function
//
bool Is2ndAction[VMAXENCODERS];


  
//
// initialise - set up pins & construct data
// these are constructed now because otherwise the configdata settings wouldn't be available yet.
//
void InitEncoders(void)
{
  EncoderList[0].Ptr = new NoClickEncoder(VPINENCODER1A, VPINENCODER1B, GEncoderDivisor, true);
  EncoderList[1].Ptr = new NoClickEncoder(VPINENCODER2A, VPINENCODER2B, GEncoderDivisor, true);
  EncoderList[2].Ptr = new NoClickEncoder(VPINENCODER3A, VPINENCODER3B, GEncoderDivisor, true);
  EncoderList[3].Ptr = new NoClickEncoder(VPINENCODER4A, VPINENCODER4B, GEncoderDivisor, true);
  EncoderList[4].Ptr = new NoClickEncoder(VPINENCODER5A, VPINENCODER5B, GEncoderDivisor, true);
  EncoderList[5].Ptr = new NoClickEncoder(VPINENCODER6A, VPINENCODER6B, GEncoderDivisor, true);
  EncoderList[6].Ptr = new NoClickEncoder(VPINENCODER7A, VPINENCODER7B, GEncoderDivisor, true);
  EncoderList[7].Ptr = new NoClickEncoder(VPINENCODER8A, VPINENCODER8B, GEncoderDivisor, true);
}




//
// encoder 1ms tick
// these are all now serviced at this rate, with a total time used of around 35 microseconds
// 
void EncoderFastTick(void)
{
  EncoderList[0].Ptr->service();
  EncoderList[1].Ptr->service();
  EncoderList[2].Ptr->service();
  EncoderList[3].Ptr->service();
  EncoderList[4].Ptr->service();
  EncoderList[5].Ptr->service();
  EncoderList[6].Ptr->service();
  EncoderList[7].Ptr->service();
}


//
// encoder 10ms tick
// 
void EncoderSlowTick(void)
{
  int16_t Movement;                                         // normal encoder movement since last update
  byte Cntr;                                                // count encoders
  EEncoderActions Action;                                   // assigned fn of the encoder
  
  for (Cntr=0; Cntr < VMAXENCODERS; Cntr++)
  {
    Movement = EncoderList[Cntr].Ptr->getValue();
    if (Movement != 0) 
    {
      EncoderList[Cntr].LastPosition += Movement;
      switch (GDisplayPage)                                   // display dependent handling:
      {
        case eIOTestPage:
          DisplayEncoderHandler(Cntr, EncoderList[Cntr].LastPosition);
          break;
        case eFrontPage:
        case eAboutPage:
        case eFreqEntryPage:
        case eBandPage:
        case eModePage:
        case eNRPage:
        case eRFPage:
          Action = GetEncoderAction(Cntr, Is2ndAction[Cntr]);       // get the assigned action; "multi" can be set in main or 2nd
          if (Action == eENMulti)                                   // if "multi" handle differently
          {
            if (Is2ndAction[Cntr])                                  // 2nd action - update the assigned action
            {
              if (Movement > 0)
              {
                if ((int)GMultiAction < (VNUMENCODERACTIONS-2))
                  GMultiAction = (EEncoderActions)((int)GMultiAction + 1);
              }
              else if (Movement < 0)
              {
                if ((int)GMultiAction > 0)
                  GMultiAction = (EEncoderActions)((int)GMultiAction - 1); 
              }
              DisplaySetEncoderAction(Cntr, GMultiAction, true);
            }
            else
              CATHandleEncoder(Cntr, Movement, GMultiAction);       // apply the assigned action
          }
          else
            CATHandleEncoder(Cntr, Movement, Action);
          break;
        case eSettingsPage:                                 // no action when on these pages
        case eConfigurePage:
            break;
      }
    }
  }

//
//read the VFO encoder; divide by N to get the desired step count
//
long ct = (VFOEncoder.read())/GVFOEncoderDivisor;
  if (ct != old_ct)
  {
    //format the output for printing 
    char buf[50];
    switch (GDisplayPage)                                   // display dependent handling:
    {
      case eIOTestPage:
        DisplayEncoderHandler(8, ct);
        break;
      case eFrontPage:
      case eAboutPage:
      case eFreqEntryPage:
      case eBandPage:
      case eModePage:
      case eNRPage:
      case eRFPage:
        CATHandleVFOEncoder(ct-old_ct);
        break;
      case eSettingsPage:                                 // no action when on these pages
      case eConfigurePage:
          break;
    }
    old_ct=ct;
  }
}



//
// handler for a button event where button is set to be the encoder control button
// (for dual function encoders)
// check button number is in range to be an encoder button! (user programming error if not)
//
// the button number to encoder number matching is as follows:
// (the encoder 2-5 are the schematic references)
//    encoder 2 push     ButtonNum=17    belongs to encoder 1, encodernum=0
//    encoder 3 push     ButtonNum=18    belongs to encoder 3, encodernum=2
//    encoder 4 push     ButtonNum=19    belongs to encoder 5, encodernum=4
//    encoder 5 push     ButtonNum=20    belongs to encoder 7, encodernum=6
//

#define VBUTTONNUMENC0 17              // button for s/w encoder 0
#define VBUTTONNUMENC2 18              // button for s/w encoder 2
#define VBUTTONNUMENC4 19              // button for s/w encoder 4
#define VBUTTONNUMENC6 20              // button for s/w encoder 6

void EncoderHandleButton(unsigned int ButtonNum, bool IsPress)
{
  EEncoderActions Action;
  int EncoderNumber;
  
  if (ButtonNum >= VBUTTONNUMENC0)                  // only process if it could correspond to an encoder
                                                    // (user config error if not!)
  {
    EncoderNumber = (ButtonNum - VBUTTONNUMENC0)<<1;        // 0,2,4 or 6
    
    if (GEncoderOperation == eDualFnClick)                  // dual fn; press toggles which function
    {
      if (IsPress)
        Is2ndAction[EncoderNumber] = !Is2ndAction[EncoderNumber];
    }
    else if (GEncoderOperation == eDualFnPress)             // dual fn; 2nd function while pressed
      Is2ndAction[EncoderNumber] = IsPress;
//
// now send the new encoder action to the display, UNLESS it is multi;
//
    Action = GetEncoderAction(EncoderNumber, Is2ndAction[EncoderNumber]);
    if (Action != eENMulti)
      DisplaySetEncoderAction(EncoderNumber, Action, false);    
  }
}

