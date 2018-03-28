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
// in the software switches are numbered 0-22, and encoders 0-6. The VFO encoder is treated separately.
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
//    5       6


//
// global variables
//
//int16_t oldEncPos, encPos;
#define STEPS 4
byte EncoderGroup;                                  // sets which ones are updated every ms
EEncoderActions GMultiAction;                       // assigned action for "multifunction"

//
// 8 encoders: one VFO (fast) encoder and 7 "normal" ones 
//
Encoder VFOEncoder(VPINVFOENCODERA, VPINVFOENCODERB);
NoClickEncoder encoder1(VPINENCODER1A, VPINENCODER1B, STEPS, true);
NoClickEncoder encoder2(VPINENCODER2A, VPINENCODER2B, STEPS, true);
NoClickEncoder encoder3(VPINENCODER3A, VPINENCODER3B, STEPS, true);
NoClickEncoder encoder4(VPINENCODER4A, VPINENCODER4B, STEPS, true);
NoClickEncoder encoder5(VPINENCODER5A, VPINENCODER5B, STEPS, true);
NoClickEncoder encoder6(VPINENCODER6A, VPINENCODER6B, STEPS, true);
NoClickEncoder encoder7(VPINENCODER7A, VPINENCODER7B, STEPS, true);

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
//
void InitEncoders(void)
{
//  vfoencoder.begin();
  EncoderList[0].Ptr = &encoder1;
  EncoderList[1].Ptr = &encoder2;
  EncoderList[2].Ptr = &encoder3;
  EncoderList[3].Ptr = &encoder4;
  EncoderList[4].Ptr = &encoder5;
  EncoderList[5].Ptr = &encoder6;
  EncoderList[6].Ptr = &encoder7;
}




//
// encoder 1ms tick
// 
void EncoderFastTick(void)
{
//  switch(EncoderGroup++)
//  {
//    case 0:
      EncoderList[0].Ptr->service();
      EncoderList[1].Ptr->service();
//      break;
//    case 1:
      EncoderList[2].Ptr->service();
      EncoderList[3].Ptr->service();
//      break;
//    case 2:
      EncoderList[4].Ptr->service();
      EncoderList[5].Ptr->service();
      EncoderList[6].Ptr->service();
      EncoderGroup=0;
//      break;
//  }
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
//read the VFO encoder; divide by 4 to get 600ppr
//
long ct = (VFOEncoder.read())>>2;
  if (ct != old_ct)
  {
    //format the output for printing 
    char buf[50];
    switch (GDisplayPage)                                   // display dependent handling:
    {
      case eIOTestPage:
        DisplayEncoderHandler(7, ct);
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
void EncoderHandleButton(unsigned int ButtonNum, bool IsPress)
{
  EEncoderActions Action;
  
  if (ButtonNum < VMAXENCODERS)                 // only process if it could correspond to an encoder
                                                // (user config error if not!)
  {
    if (GEncoderOperation == eDualFnClick)                  // dual fn; press toggles which function
    {
      if (IsPress)
        Is2ndAction[ButtonNum] = !Is2ndAction[ButtonNum];
    }
    else if (GEncoderOperation == eDualFnPress)             // dual fn; 2nd function while pressed
      Is2ndAction[ButtonNum] = IsPress;
//
// now send the new encoder action to the display, UNLESS it is multi;
//
    Action = GetEncoderAction(ButtonNum, Is2ndAction[ButtonNum]);
    if (Action != eENMulti)
      DisplaySetEncoderAction(ButtonNum, Action, false);    
  }
}

