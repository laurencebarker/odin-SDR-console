/////////////////////////////////////////////////////////////////////////
//
// Odin SDR Console by Laurence Barker G8NJJ
//
// this sketch provides a knob and switch interface through USB and CAT
//
// configdata.c
// this file holds the code to save and load settings to/from FLASH
/////////////////////////////////////////////////////////////////////////

#ifndef __CONFIGDATA_H
#define __CONFIGDATA_H

#include "types.h"


//
// RAM storage of loaded settings
// these are loaded from FLASH after boot up
//
extern EBaudRates GUSBBaudRate;
extern EDualFnEncoders GEncoderOperation;                   // global for all encoders
extern bool GBottomEncoderStrings;                           // true to have legends at the display bottom
extern bool GSideEncoderStrings;                             // true to display legends at the side

//
// function to copy all config settings to flash
//
void CopySettingsToFlash(void);


//
// function to load config settings from flash
//
void LoadSettingsFromFlash(void);


//
// function to check FLASH is initialised, and load it if not
// called on start-up BEFORE loading the settings
//
void CheckFlashInitialised(void);


//
// functions to retrieve assigned button, indicator and encoder actions
// the device number is 0..N-1
//
EEncoderActions GetEncoderAction(unsigned int Encoder, bool Is2ndFunction);
EIndicatorActions GetIndicatorAction(unsigned int Indicator);
EButtonActions GetButtonAction(unsigned int Button);

//
// functions to store assigned button, indicator and encoder actions
// this is a precursor to doing a "save to flash"
//
void SetEncoderAction(unsigned int Encoder, EEncoderActions Setting, bool Is2ndFunction);
void SetIndicatorAction(unsigned int Indicator, EIndicatorActions Setting);
void SetButtonAction(unsigned int Button, EButtonActions Setting);






#endif  //not defined
