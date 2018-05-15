/////////////////////////////////////////////////////////////////////////
//
// Odin SDR Console by Laurence Barker G8NJJ
//
// this sketch provides a knob and switch interface through USB and CAT
//
// configdata.c
// this file holds the code to save and load settings to/from FLASH
/////////////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include "types.h"
#include <DueFlashStorage.h>
DueFlashStorage dueFlashStorage;                 // instance of flash reader

#define VFLASHINITPATTERN 0x6D                    // addr 0 set to this if configured

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
//    5A      6
//    5B      7
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




//
// array of factory encoder settings. User can reconfigure from this.
//
const EEncoderActions GFactoryEncoderActions[] =
{
  eENAFGain,
  eENAGCLevel,
  eENFilterHigh,
  eENFilterLow,
  eENDiversityGain,
  eENDiversityPhase,
  eENMulti,
  eENDrive  
};


//
// array of factory indicator settings. User can reconfigure from this.
//
const EIndicatorActions GFactoryIndicatorActions[] =
{
  eINVFOAB,
  eINMOX,
  eINTune,
  eINCTune,
  eINLock,
  eINRIT,
  eINNR 
};



//
// array of factory pushbutton settings. User can reconfigure from this.
// 1-7 are encoder 1-7 "click" settings; 8-23 are pushbuttons 1-16
//
const EButtonActions GFactoryButtonActions[] =
{
  ePBABVfo,                           // SW1
  ePBMox,                             // SW2
  ePBTune,                            // SW3
  ePBCTune,                           // SW4
  ePBLock,                            // SW5
  ePBAtoB,                            // SW6
  ePBBtoA,                            // SW7
  ePBSplit,                           // SW8
  ePBRIT,                             // SW9
  ePBRITMinus,                        // SW10
  ePBRITPlus,                         // SW11
  ePBBandDown,                        // SW12
  ePBModeDown,                        // SW13
  ePBStartStop,                       // SW14
  ePBBandUp,                          // SW15
  ePBModeUp,                          // SW16
  ePBNRStep,                          // SW17
  ePBAFMute,                          // encoder 2 click
  ePBFilterReset,                     // encoder 3 click
  ePBDiversityFastSlow,               // encoder 4 click
  ePBEncoderClick                     // encoder 5 click
};




//
// RAM storage of loaded settings
// these are loaded from FLASH after boot up
//
EBaudRates GUSBBaudRate;
EDualFnEncoders GEncoderOperation;                   // global for all encoders
EEncoderActions GEncoderMainActions[VMAXENCODERS];
EEncoderActions GEncoder2ndActions[VMAXENCODERS];
EIndicatorActions GIndicatorActions[VMAXINDICATORS];
EButtonActions GButtonActions[VMAXBUTTONS];
bool GBottomEncoderStrings;                           // true to have legends at the display bottom
bool GSideEncoderStrings;                             // true to display legends at the side
byte GEncoderDivisor;                                // number of edge events per declared click
byte GVFOEncoderDivisor;                             // number of edge events per declared click



//
// function to copy all config settings to flash
// this copies the current RAM vaiables to the persistent storage
//
void CopySettingsToFlash(void)
{
  int Addr=1;
  byte Setting;
  int Cntr;
  
//
// first set that we have initialised the FLASH
//
  dueFlashStorage.write(0, VFLASHINITPATTERN);
//
// now copy settings from RAM data
//
  Setting = (byte)GUSBBaudRate;
  dueFlashStorage.write(Addr++, Setting);
  Setting = (byte) GEncoderOperation;
  dueFlashStorage.write(Addr++, Setting);
  Setting = (byte) GBottomEncoderStrings;
  dueFlashStorage.write(Addr++, Setting);
  Setting = (byte) GSideEncoderStrings;
  dueFlashStorage.write(Addr++, Setting);
  Setting = (byte) GEncoderDivisor;
  dueFlashStorage.write(Addr++, Setting);
  Setting = (byte) GVFOEncoderDivisor;
  dueFlashStorage.write(Addr++, Setting);
// write encoders  
  for (Cntr=0; Cntr < VMAXENCODERS;   Cntr++)
  {
    Setting = (byte)GEncoderMainActions[Cntr];
    dueFlashStorage.write(Addr++, Setting);
  }
// write indicators  
  for (Cntr=0; Cntr < VMAXINDICATORS;   Cntr++)
  {
    Setting = (byte)GIndicatorActions[Cntr];
    dueFlashStorage.write(Addr++, Setting);
  }
// write pushbuttons  
  for (Cntr=0; Cntr < VMAXBUTTONS;   Cntr++)
  {
    Setting = (byte)GButtonActions[Cntr];
    dueFlashStorage.write(Addr++, Setting);
  }
}



//
// function to copy initial settings to FLASH
// this sets the factory defaults
// the settings here should match the fornt panel legend!
//
void InitialiseFlash(void)
{
  int Cntr;
  
  GUSBBaudRate = eBaud115200;
  GEncoderOperation = eDualFnClick;
  GBottomEncoderStrings = true;
  GSideEncoderStrings = true;
  GEncoderDivisor = 2;
  GVFOEncoderDivisor = 4;

  
// initialise encoders
  for(Cntr=0; Cntr < VMAXENCODERS; Cntr++)
    GEncoderMainActions[Cntr] = GFactoryEncoderActions[Cntr];
// initialise indicators
  for(Cntr=0; Cntr < VMAXINDICATORS; Cntr++)
    GIndicatorActions[Cntr] = GFactoryIndicatorActions[Cntr];
// initialise indicators
  for(Cntr=0; Cntr < VMAXBUTTONS; Cntr++)
    GButtonActions[Cntr] = GFactoryButtonActions[Cntr];

// now copy them to FLASH
  CopySettingsToFlash();
}



//
// function to load config settings from flash
//
void LoadSettingsFromFlash(void)
{
  int Addr=1;
  byte Setting;
  int Cntr;

//
// first see if we have initialised the FLASH previously
// if not, copy settings to it
//
  Setting = dueFlashStorage.read(0);
  if (Setting != VFLASHINITPATTERN)
    InitialiseFlash();
//
// now copy out settings to RAM data
//
  GUSBBaudRate = (EBaudRates)dueFlashStorage.read(Addr++);
  GEncoderOperation = (EDualFnEncoders)dueFlashStorage.read(Addr++);
  GBottomEncoderStrings = (bool)dueFlashStorage.read(Addr++);
  GSideEncoderStrings = (bool)dueFlashStorage.read(Addr++);
  GEncoderDivisor = (byte)dueFlashStorage.read(Addr++);
  GVFOEncoderDivisor = (byte)dueFlashStorage.read(Addr++);
// read encoders  
  for (Cntr=0; Cntr < VMAXENCODERS;   Cntr++)
    GEncoderMainActions[Cntr] = (EEncoderActions)dueFlashStorage.read(Addr++);
// read indicators  
  for (Cntr=0; Cntr < VMAXINDICATORS;   Cntr++)
    GIndicatorActions[Cntr] = (EIndicatorActions)dueFlashStorage.read(Addr++);
// read pushbuttons  
  for (Cntr=0; Cntr < VMAXBUTTONS;   Cntr++)
    GButtonActions[Cntr] = (EButtonActions)dueFlashStorage.read(Addr++);
}


//
// functions to retrieve assigned button, indicator and encoder actions
// for encoder: 
//
EEncoderActions GetEncoderAction(unsigned int Encoder, bool Is2ndFunction)
{
  EEncoderActions Action1;
  EEncoderActions Result;

  Action1 = GEncoderMainActions[Encoder];           //  main
  
  if (Action1 == eENMulti)
    Result = eENMulti;
  else if (Is2ndFunction)
  {
    if (Encoder < (VMAXENCODERS-1))                    // if not last encoder
      Result = GEncoderMainActions[Encoder+1];        //  2nd function is assigned to the next encoder number
    else
      Result = Action1;
  }
  else
    Result = Action1;
  
  return Result;   
}


EIndicatorActions GetIndicatorAction(unsigned int Indicator)
{
  return GIndicatorActions[Indicator];
}


EButtonActions GetButtonAction(unsigned int Button)
{
  return GButtonActions[Button];
}


//
// functions to store assigned button, indicator and encoder actions
// this is a precursor to doing a "save to flash"
//
void SetEncoderAction(unsigned int Encoder, EEncoderActions Setting)
{
  GEncoderMainActions[Encoder] = Setting;
}

void SetIndicatorAction(unsigned int Indicator, EIndicatorActions Setting)
{
  GIndicatorActions[Indicator] = Setting;
}

void SetButtonAction(unsigned int Button, EButtonActions Setting)
{
  GButtonActions[Button] = Setting;
}

