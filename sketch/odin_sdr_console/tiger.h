/////////////////////////////////////////////////////////////////////////
//
// Odin SDR Console by Laurence Barker G8NJJ
//
// this sketch provides a knob and switch interface through USB and CAT
//
// tiger.h
// this file holds the CAT parsing code
// large CAT file = tiger....
/////////////////////////////////////////////////////////////////////////
#ifndef __tiger_h
#define __tiger_h
#include <Arduino.h>
#include "types.h"


//
// an enumerated list of all of the debug CAT commands
//
enum EDebugCATCommands
{
  eLEDOn,                           // turn LED on
  eLEDOff,                          // turn LED off
  eIdent,                           // identify hardware
  eNoDebugCommand                   // exception
};

//
// firstly an enumerated list of all of the CAT commands
// ordered as per documentation, not alphsabetically!
enum ECATCommands
{
  eZZAG,                          // master AG gain
  eZZLA,                          // RX1 AF gain
  eZZLE,                          // RX2 AF gain
  eZZPA,                          // RX1 attenuation
  eZZPB,                          // RX2 attenuation
  eZZAR,                          // RX1 AGC threshold
  eZZAS,                          // RX2 AGC threshold
  eZZGT,                          // RX1 AGC speed
  eZZGU,                          // RX2 AGC speed
  eZZFL,                          // RX1 filter low cut
  
  eZZFS,                          // RX2 filter low cut
  eZZFH,                          // RX1 filter high cut
  eZZFR,                          // RX2 filter high cut
  eZZPC,                          // TX drive
  eZZMG,                          // TX mic gain
  eZZFA,                          // set VFO A absolute freq
  eZZFB,                          // set VFO B absolute freq
  eZZAE,                          // VFO A dowen n steps
  eZZAF,                          // VFO A up n steps
  eZZBE,                          // VFO B down n steps
  
  eZZBF,                          // VFO B up n steps
  eZZVG,                          // VOX gain
  eZZXH,                          // VOX delay
  eZZVE,                          // VOX on/off
  eZZCL,                          // CW sidetone
  eZZCS,                          // CW speed
  eZZTX,                          // MOX state
  eZZTU,                          // TUNE state
  eZZBD,                          // RX1 band down
  eZZBU,                          // RX1 band up
  
  eZZBS,                          // RX1 set/read band
  eZZBA,                          // RX2 band down
  eZZBB,                          // RX2 band up
  eZZBT,                          // RX2 set/read band
  eZZMD,                          // RX1 mode
  eZZME,                          // RX2 mode
  eZZRT,                          // rIT state
  eZZRU,                          // RIT step up
  eZZRD,                          // RIT step down
  eZZSP,                          // SPLIT state
  
  eZZCN,                          // RX1 click tune
  eZZCO,                          // RX2 click tune
  eZZUX,                          // VFO A LOCK state
  eZZUY,                          // VFO B LOCK state
  eZZSM,                          // display S meter
  eZZRM,                          // display TX metering
  eZZNR,                          // RX1 NR mode
  eZZNS,                          // RX1 enhanced NR mode
  eZZNV,                          // RX2 NR mode
  eZZNW,                          // RX2 enhanced NR mode

  eZZNA,                          // RX1 NB mode
  eZZNB,                          // RX1 enhanced NB mode
  eZZNC,                          // RX2 NB mode
  eZZND,                          // RX2 enhanced NB mode
  eZZNN,                          // RX1 SNB
  eZZNO,                          // RX2 SNB
  eZZNT,                          // RX1 ANF
  eZZNU,                          // RX2 ANF
  eZZMA,                          // RX1 mute
  eZZMB,                          // RX2 mute

  eZZPS,                          // radio START
  eZZSQ,                          // RX1 squelch level
  eZZSX,                          // RX2 squelch level
  eZZSO,                          // RX1 squelch on/off
  eZZSV,                          // RX2 squelch on/off
  eZZVS,                          // VFO swap/copy
  eZZXN,                          // combined RX1 status
  eZZXO,                          // combined RX2 status
  eZZXV,                          // combined VFO status
  eZZAC,                          // get VFO tuning step

  eZZDB,                          // diversity reference source
  eZZDC,                          // diversity RX2 gain
  eZZDD,                          // diversity phase
  eZZDE,                          // diversity enable
  eZZDG,                          // diversity RX1 gain
  eZZDH,                          // diversity receiver source
  eZZSW,                          // VFO A/B enabled for TX
  eZZRX,                          // RX1 step atten
  eZZRY,                          // RX2 step atten
  eZZCP,                          // compander on/off

  eZZCT,                          // compander threshold
  eZZLI,                          // puresignal on/off
  eZZUT,                          // puresignal 2 tone
  eZZUS,                          // puresignal single cal
  eZZMO,                          // monitor on/off
  
  eNoCommand                      // this is an exception condition
};


// defines for the ZZVS message:
#define V_ZZVS_ATOB 0
#define V_ZZVS_BTOA 1
#define V_ZZVS_SWAP 2

typedef enum
{
  eNone,                          // no parameter
  eBool,                          // boolean parameter
  eNum,                           // numeric parameter     
  eStr                            // string parameter
}ERXParamType;



//
// this struct holds a record to describe ione CAT command
//
struct SCATCommands
{
  char* CATString;                // eg "ZZAR"
  ERXParamType RXType;            // type of parameter expected on receive
  int MinParamValue;              // eg "-999"
  int MaxParamValue;              // eg "9999"
  byte NumParams;                 // number of parameter bytes in a "set" command
  bool AlwaysSigned;              // true if the param version should always have a sign
};



extern SCATCommands GCATCommands[];

//
// initialise CAT handler
//
void InitCAT(void);


//
// ScanParseSerial()
// scans input serial stream for characters; parses complete commands
// when it finds one
//
void ScanParseSerial(void);



//
// ParseCATCmd()
// Parse a single command in the local input buffer
// process it if it is a valid command
//
void ParseCATCmd(void);
#ifdef SENSORDEBUG
void ParseDebugCATCmd(void);
#endif

//
// create CAT message:
// this creates a "basic" CAT command with no parameter
// (for example to send a "get" command)
//
void MakeCATMessageNoParam(ECATCommands Cmd);


//
// make a CAT command with a numeric parameter
//
void MakeCATMessageNumeric(ECATCommands Cmd, int Param);

//
// make a CAT command with a bool parameter
//
void MakeCATMessageBool(ECATCommands Cmd, bool Param);

//
// make a CAT command with a string parameter
// the string is truncated if too long, or padded with spaces if too short
//
void MakeCATMessageString(ECATCommands Cmd, char* Param);

//
// helper to convert int to string
//
void LongToString(long Param, char* Output, int CharCount);
//
// helper to append a string with a character
//
void Append(char* s, char ch);


#endif //not defined
