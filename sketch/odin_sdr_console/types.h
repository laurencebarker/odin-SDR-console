/////////////////////////////////////////////////////////////////////////
//
// Odin SDR Console by Laurence Barker G8NJJ
//
// this sketch provides a knob and switch interface through USB and CAT
//
// types.h
//
/////////////////////////////////////////////////////////////////////////

#ifndef __TYPES_H
#define __TYPES_H

#define VMAXINDICATORS 7
#define VMAXENCODERS 8              // configurable, not including VFO
#define VMAXBUTTONS 21

//
// this type enumerates the Nextion display pages:
//
enum EDisplayPage
{
  eFrontPage,                               // "normal" front page display
  eIOTestPage,                               // I/O debug page
  eAboutPage,
  eFreqEntryPage,
  eBandPage,
  eModePage,
  eNRPage,
  eRFPage,
  eSettingsPage,
  eConfigurePage,
  eSplashPage
};

//
// types etc for editing data:
//
typedef enum
{
  eEncoders,
  ePushbuttons,
  eIndicators
} EControlTypes;

//
// this enum type lists all the functions that can be assigned to configurable encoders
//
typedef enum
{
  eENNoAction,
  eENAFGain,
  eENMasterGain,
  eENAGCLevel,
  eENFilterHigh,
  eENFilterLow,
  eENDrive,
  eENMicGain,
  eENVFOATune,
  eENVFOBTune,
  eENVOXGain,
  eENVOXDelay,
  eENCWSidetone,
  eENCWSpeed,
  eENSquelch,
  eENDiversityGain,
  eENDiversityPhase,
  eENMulti,                      // multifunction
  eENCompanderThreshold,
  eENRX1AFGain,
  eENRX2AFGain,
  eENRX1StepAtten,
  eENRX2StepAtten
} EEncoderActions;
#define VNUMENCODERACTIONS 23

//
// this enum type lists all the functions that can be assigned to LED indicators
//
typedef enum
{
  eINMOX,
  eINTune,
  eINRIT,
  eINSplit,
  eINCTune,
  eINLock,
  eINNB,
  eINNR,
  eINSNB,
  eINANF,
  eINSquelch,
  eINVFOAB,
  eINCompanderEnabled,
  eINPuresignalEnabled,
  eINEncoder2nd,
  eINNone
} EIndicatorActions;
#define VNUMINDICATORACTIONS 16

//
// this enum type lists all the functions that can be assigned to configurable pushbuttons
// note this includes the "click" switch on an encoder
//
typedef enum
{
  ePBNone,                            // no assigned function
  ePBEncoderClick,                    // for dual fn encoders
  ePBABVfo,
  ePBMox,
  ePBTune,
  ePBAFMute,
  ePBFilterReset,
  ePBBandUp,
  ePBBandDown,
  ePBModeUp,
  ePBModeDown,
  ePBAGCSpeed,
  ePBNBStep,
  ePBNRStep,
  ePBSNB,
  ePBANF,
  ePBRIT,
  ePBRITPlus,
  ePBRITMinus,
  ePBAtoB,
  ePBBtoA,
  ePBSwap,
  ePBSplit,
  ePBCTune,
  ePBLock,
  ePBStartStop,
  ePBSquelch,
  ePBAtten,
  ePBVoxOnOff,
  ePBDiversityFastSlow,
  ePBCompanderEnable,
  ePBPuresignalEnable,
  ePBPuresignal2Tone,
  ePBPuresignalSingleCal,
  ePBMonEnable
} EButtonActions;
#define VNUMBUTTONACTIONS 35

//
// this enum defines legal baud rates for serial
//
typedef enum
{
  eBaud9600,
  eBaud38400,
  eBaud115200
} EBaudRates;

//
// type to describe how encoders change between 2 functions
//
typedef enum
{
  eDualFnClick,
  eDualFnPress
} EDualFnEncoders;


//
// type to encode a band selection
//
typedef enum
{
  e160,
  e80,
  e60,
  e40,
  e30,
  e20,
  e17,
  e15,
  e12,
  e10,
  e6,
  eGen,
  e2,                       // be aware the order here different from CAT list!
  eWWV
} EBand;


//
// type to encode a mode
//
typedef enum
{
  eLSB,
  eUSB,
  eDSB,
  eCWL,
  eCWU,
  eFM,
  eAM,
  eDIGU,
  eSPEC,
  eDIGL,
  eSAM,
  eDRM
} EMode;



//
// type to endoe noise reduction setting
//
typedef enum
{
  eNROff,
  eNR1,
  eNR2
} ENRState;


//
// type to endoe noise blanking setting
//
typedef enum
{
  eNBOff,
  eNB1,
  eNB2
} ENBState;


//
// type to encode AGC speed
//
typedef enum
{
  eFixed,
  eLong,
  eSlow,
  eMedium,
  eFast,
  eCustom
} EAGCSpeed;


//
// type to encode RX attenuation
//
typedef enum
{
  e0dB,
  e10dB,
  e20dB,
  e30dB
} EAtten;


#endif //not defined
