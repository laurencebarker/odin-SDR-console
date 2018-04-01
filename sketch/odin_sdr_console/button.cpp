/////////////////////////////////////////////////////////////////////////
//
// Odin SDR Console by Laurence Barker G8NJJ
//
// this sketch provides a knob and switch interface through USB and CAT
//
// button.c
// this file holds the code to debounce pushbutton inputs
/////////////////////////////////////////////////////////////////////////

#include "types.h"
#include "button.h"
#include "iopins.h"
#include "display.h"
#include "cathandler.h"
#include "configdata.h"
#include "encoders.h"


//
// note switch and encoder numbering:
// in the software switches are numbered 0-22, and encoders 0-6. The VFO encoder is treated separately.
// these correspond to the control of Kjell's PCB as follows:
//
// switch numbering:
//    PCB   software
//    SW1                 0
//    SW2                 1
//    SW3                 2
//    SW4                 3
//    SW5                 4
//    SW6                 5
//    SW7                 6
//    SW8                 7
//    SW9                 8
//    SW10                9
//    SW11               10
//    SW12               11
//    SW13               12
//    SW14               13
//    SW15               14
//    SW16               15
//    SW17               16
//    encoder 2 push     17
//    encoder 3 push     18
//    encoder 4 push     19
//    encoder 5 push     20
//



bool GButtonPressed[VMAXBUTTONS];                 // true when button debounced as pressed

//
// I/O pins for each button
// 1-16 are pushbuttons 1-16; 17-23 are the 7 "normal" encoder clicks
// (and on Kjell's PCB, 3 of those are used for normal pushbuttons)
//
byte GButtonPinNumbers[VMAXBUTTONS] =
{
  VPINBUTTON1,
  VPINBUTTON2,
  VPINBUTTON3,
  VPINBUTTON4,
  VPINBUTTON5,
  VPINBUTTON6,
  VPINBUTTON7,
  VPINBUTTON8,
  VPINBUTTON9,
  VPINBUTTON10,
  VPINBUTTON11,
  VPINBUTTON12,
  VPINBUTTON13,
  VPINBUTTON14,
  VPINBUTTON15,
  VPINBUTTON16,
  VPINBUTTON17,
  VPINENCODER1SW,
  VPINENCODER3SW,
  VPINENCODER5SW,
  VPINENCODER7SW
};


//
// array of debounce states
// these are simply the last 8 samples of the pin, starting in the LSB
//
byte GInputDebounce[VMAXBUTTONS];


//
// initialise
// simply set all the debounce inputs to 0xFF (button released)
//
void GButtonInitialise(void)
{
  int Cntr;

  for (Cntr=0; Cntr < VMAXBUTTONS; Cntr++)
    GInputDebounce[Cntr] = 0xFF;
}


//
// called when a button is pressed
// the parameter is the button number (0..N-1)
//
void ButtonPressed(int ButtonNum)
{
  EButtonActions AssignedAction;                          // programmed function for this button
  
  GButtonPressed[ButtonNum] = true;
  AssignedAction = GetButtonAction(ButtonNum);
  switch (GDisplayPage)                                   // display dependent handling:
  {
    case eIOTestPage:
      DisplayButtonHandler(ButtonNum, true); 
      break;
    case eFrontPage:
    case eAboutPage:
    case eFreqEntryPage:
    case eBandPage:
    case eModePage:
    case eNRPage:
    case eRFPage:
      if (AssignedAction == ePBEncoderClick)
        EncoderHandleButton(ButtonNum, true);
      else if (AssignedAction != ePBNone)
        CATHandlePushbutton(ButtonNum, AssignedAction, true); 
      break;
    case eSettingsPage:                                 // no action when on these pages
    case eConfigurePage:
      break;
  }
}



//
// called when a button is released
// the parameter is the button number (0..N-1)
//
void ButtonReleased(int ButtonNum)
{
  EButtonActions AssignedAction;                          // programmed function for this button
  
  GButtonPressed[ButtonNum] = false;
  AssignedAction = GetButtonAction(ButtonNum);
  switch (GDisplayPage)                                   // display dependent handling:
  {
    case eIOTestPage:
      DisplayButtonHandler(ButtonNum, false); 
      break;
    case eFrontPage:
    case eAboutPage:
    case eFreqEntryPage:
    case eBandPage:
    case eModePage:
    case eNRPage:
    case eRFPage:
      if (AssignedAction == ePBEncoderClick)
        EncoderHandleButton(ButtonNum, false);
      else if (AssignedAction != ePBNone)
        CATHandlePushbutton(ButtonNum, AssignedAction, false); 
      break;
    case eSettingsPage:                                 // no action when on these pages
    case eConfigurePage:
      break;
  }
}


#define VEDGEMASK 0b00000111                                  // 3 most recent samples
#define VPRESSPATTERN 0b00000100                              // 2 consecutive presses
#define VRELEASEPATTERN 0b00000011                            // 2 consecutive releases

//
// Tick
// read each pin into the LSB of its debounce register; then look for press or release pattern
// only accept a "pressed" pattern if button is currently released and vice versa
void ButtonTick(void)
{
  int Cntr;
  byte Input;                                   // becomes the new bit sequence for an input
  
  for(Cntr=0; Cntr < VMAXBUTTONS; Cntr++)
  {
    Input = GInputDebounce[Cntr] << 1;          // mocve it left
    if (digitalRead(GButtonPinNumbers[Cntr]) == HIGH)
      Input |= 1;                               // set bottom bit if input was high
    GInputDebounce[Cntr] = Input;               // write it back
    if (!GButtonPressed[Cntr])
    {
      if ((Input & VEDGEMASK) == VPRESSPATTERN)
        ButtonPressed(Cntr);
    }
    else      // button is already pressed
    {
      if ((Input & VEDGEMASK) == VRELEASEPATTERN)
        ButtonReleased(Cntr);
    }
  }
}

