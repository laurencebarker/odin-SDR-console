/////////////////////////////////////////////////////////////////////////
//
// Odin SDR Console by Laurence Barker G8NJJ
//
// this sketch provides a knob and switch interface through USB and CAT
//
// CAT handler.cpp
// this file holds the CAT handling code
// responds to parsed messages, and initiates message sends
// this is the main body of the program!
/////////////////////////////////////////////////////////////////////////

#include "display.h"
#include "cathandler.h"
#include "configdata.h"
#include "led.h"
#include <stdlib.h>



//
// global variables
//
//
// CAT data stored settings
// this is data that has been reported by The PC through a CAT message
//
bool GCatStateTX = false;                                   // true if CAT has reported TX
bool GCatStateTune = false;                                 // true if CAT has reported Tune
bool GCatStateRIT = false;                                  // true if CAT reported RIT is on
bool GCatStateALock = false;                                // true if VFO A lock is on
bool GCatStateBLock = false;                                // true if VFO B lock is on
bool GCatStateSplit = false;                                // true if SPLIT state set
bool GCatStateACTune = false;                               // true if VFO A CTUNE is on
bool GCatStateBCTune = false;                               // true if VFO B CTUNE is on
EAtten GCatStateAtten = e0dB;                               // RX1/2 atten
EAGCSpeed GCatStateAGCSpd = eMedium;                        // RX1/2 AGC speed
bool GCatStateSquelch = false;                              // RX1/2 squelch
bool GCatStateSNB = false;                                  // RX1/2 spectral noise blanker
bool GCatStateANF = false;                                  // RX1/2 notch filter
bool GCatStateABTX = false;                                 // true if VFO B enabled for TX
ENRState GCatStateNR = eNROff;                              // RX1/2 NR
ENBState GCatStateNB = eNBOff;                              // RX1/2 NB
EMode GCatStateMode;                                        // RX1/2 mode
EBand GCatStateBand;                                        // RX1/2 band
unsigned long GCatFrequency_Hz;                             // current frequency in Hz
int GCatAGCThreshold;                                       // current AGC threshold
int GCatFilterLow;                                          // current filter passband low edge
int GCatFilterHigh;                                         // current filter passband high edge
int GCatSquelchLevel;                                       // current squelch level
int GCatRX1AFGain;                                          // current RX1 AF gain
int GCatRX2AFGain;                                          // current RX2 AF gain
int GCatMastAFGain;                                         // current master AF gain
int GCatDriveLevel;                                         // current TX drive level
int GCatMicGain;                                            // current TX mic gain
int GCatVoxGain;                                            // current TX vox gain
int GCatVoxDelay;                                           // current TX vox delay
int GCatCWTone;                                             // current CW sidetone
int GCatCWSpeed;                                            // current CW speed
long GCatDiversityPhase;                                    // current diversity phase (-18000 to 18000, meaning -180 to 180 degrees)
int GCatDiversityGain;                                      // current diversity gain (0 to 5000, meaning 0.000 to 5.000)
bool GCatDiversityRefSource;                                // current reference (true: RX1; false: RX2)

int GVFOStepSize;                                           // current VFO step, in Hz
int GDisplayThrottle;                                       // !=0 for a few ticks after a display frequency update

//
// console requested settings
//
bool GConsoleTX = false;                                    // true if console has requested TX
bool GConsoleTune = false;                                  // true if console has requested Tune
bool GConsoleVFOA = true;                                   // true if we want VFO A
bool GConsoleExtTX = false;                                 // true if externally triggered TX

//
// requests to change CAT state
bool GToggleRadioOnOff;                             // true if we need to toggle radio on/off message
bool GToggleMute;                                   // true if we need to toggle bool
bool GFilterReset;                                  // true if we need to initiate a filter reset
bool GToggleVoxOnOff;                               // true to initiate toggle VOX on/off

//
#define GPERIODICREFRESHDELAY 10                    // 10ms ticks; initially 100ms/step for testing
#define GPERIODICSEQUENCELENGTH 10                   // length of refresh sequence 
#define VDISPLAYTHROTTLETICKS 10                    // no freq update from VFO encoder until 10 ticks after the last
int GPeriodicRefreshTimer;
int GeriodicRefreshState;
int GPeriodicRefreshSubState;

//
// these variables are true if there is a "live, unactioned" request for periodic update
//
bool GLiveRequestFrequency;
bool GLiveRequestVFOStatus;
bool GLiveRequestRXStatus;
bool GLiveRequestMode;
 

#define VGETTIMEOUT 100                           // 1s timeout on a "get" command where we wait for the answer
#define VRECENTTHRESHOLD  500                     // 5 s timeout before we need to re-poll for data

//
// timeouts, recent values and click counts
//

// band
int GBandTimeout;

// AGC threshold
int GAGCThresholdTimeout;
int GAGCThresholdRecent;
int GAGCThresholdClicks; 

// filter low/high
int GFilterLowTimeout;
int GFilterLowRecent;
int GFilterLowClicks;
int GFilterHighTimeout;
int GFilterHighRecent;
int GFilterHighClicks;

// squelch
int GSquelchLevelTimeout;
int GSquelchLevelRecent;
int GSquelchLevelClicks;

// AF gains
int GRX1AFGainTimeout;
int GRX1AFGainRecent;
int GRX1AFGainClicks;
int GRX2AFGainTimeout;
int GRX2AFGainRecent;
int GRX2AFGainClicks;
int GMastAFGainTimeout;
int GMastAFGainRecent;
int GMastAFGainClicks;

// TX drive
int GDriveLevelTimeout;
int GDriveLevelRecent;
int GDriveLevelClicks;

// TX mic gain
int GMicGainTimeout;
int GMicGainRecent;
int GMicGainClicks;

// VOX gain/delay
int GVoxGainTimeout;
int GVoxGainRecent;
int GVoxGainClicks;
int GVoxDelayTimeout;
int GVoxDelayRecent;
int GVoxDelayClicks;

// CW tone/speed
int GCWToneTimeout;
int GCWToneRecent;
int GCWToneClicks;
int GCWSpeedTimeout;
int GCWSpeedRecent;
int GCWSpeedClicks;

// diversity
int GDiversityPhaseTimeout;
int GDiversityPhaseRecent;
int GDiversityPhaseClicks;
int GDiversityGainTimeout;
int GDiversityGainRecent;
int GDiversityGainClicks;
int GDiversitySourceTimeout;
bool GDiversityStepFast = true;                    // true for fast steps

// lookup VFO step size from CAT msg parameter, in hz
// this table must match the CAT definition for ZZAC!
int GStepLookup[] = 
{
  1, 2, 10, 25, 50, 
  100, 250, 500, 1000, 
  2000, 2500, 5000, 6250,
  9000, 10000, 12500, 15000,
  20000, 25000, 30000, 50000, 
  100000, 250000, 500000, 1000000, 10000000 
};


//
// filter reset settings
// these are indexed by mode (ie 12 settings)
// note CWL and CWU need to reference to CW sidetone
//
int GFilterResetLowValues[] =
{
  -2850,  // eLSB
  150,    // eUSB
  -3300,  // eDSB
  -250,   // eCWL
  -250,   // eCWU
  -8000,  // eFM
  -5000,  // eAM
  1000,   // eDIGU
  -4000,  // eSPEC
  -2000,  // eDIGL
  -4000,  // eSAM
  1000    // eDRM
};

int GFilterResetHighValues[] =
{
  -150,   // eLSB
  2850,   // eUSB
  3300,   // eDSB
  250,    // eCWL
  250,    // eCWU
  8000,   // eFM
  5000,   // eAM
  2000,   // eDIGU
  4000,   // eSPEC
  -1000,  // eDIGL
  4000,   // eSAM
  2000    // eDRM
};



//
// initialise CAT handler
//
void InitCATHandler(void)
{
  
}


//
// get the low, high erdges of "ideal" filter settings
// returns a frequency in Hz
//
int GetOptimumIFFilterLow(void)
{
  int FilterPBLow;                              // passband limits looked up

  FilterPBLow = GFilterResetLowValues[(int)GCatStateMode];
//
// for CW: use the low/high as freq either cide of CW sidetone  
//
  if (GCatStateMode == eCWL)
  {
    FilterPBLow = FilterPBLow - GCatCWTone;
  }
  else if (GCatStateMode == eCWU)
  {
    FilterPBLow = FilterPBLow + GCatCWTone;
  }
  return FilterPBLow;
}


int GetOptimumIFFilterHigh(void)
{
  int FilterPBHigh;                              // passband limits looked up

  FilterPBHigh = GFilterResetHighValues[(int)GCatStateMode];
//
// for CW: use the low/high as freq either cide of CW sidetone  
//
  if (GCatStateMode == eCWL)
  {
    FilterPBHigh = FilterPBHigh - GCatCWTone;
  }
  else if (GCatStateMode == eCWU)
  {
    FilterPBHigh = FilterPBHigh + GCatCWTone;
  }
  return FilterPBHigh;
}



int GUpdateIndicator;                                     // indicator number being updated (0 - 6)
//
// function to set LEDs to the required state
// update one per tick
//
void UpdateIndicators(void)
{
  EIndicatorActions Action;                               // assigned function of indicator
  bool NewState = false;                                  // state LED should be set to
  
  if (GDisplayPage != eIOTestPage)                        // in I/O test, the LEDs are driven by the screen PBs
  {
    if (++GUpdateIndicator >= VMAXINDICATORS)
      GUpdateIndicator = 0;
  
    Action = GetIndicatorAction(GUpdateIndicator);          // find what indicator does, and implement it
    switch(Action)
    {
      case eINMOX:                                          // locally initiated MOX
        NewState = GConsoleTX;
        break;
        
      case eINTune:                                         // locally initiated TUNE
        NewState = GConsoleTune;
        break;
  
      case eINRIT:
        NewState = GCatStateRIT;
        break;
        
      case eINSplit:
        NewState = GCatStateSplit;
        break;
        
      case eINCTune:
        if(GConsoleVFOA)
          NewState = GCatStateACTune;
        else
          NewState = GCatStateBCTune;
        break;
        
      case eINLock:
        if(GConsoleVFOA)
          NewState = GCatStateALock;
        else
          NewState = GCatStateBLock;
        break;
  
      case eINNB:
        if (GCatStateNB != eNBOff)
          NewState = true;
        break;
        
      case eINNR:
        if (GCatStateNR != eNROff)
          NewState = true;
        break;
        
      case eINSNB:
        NewState = GCatStateSNB;
        break;
        
      case eINANF:
        NewState = GCatStateANF;
        break;
        
      case eINSquelch:
        NewState = GCatStateSquelch;
        break;

      case eINVFOAB:
        if(GConsoleVFOA)
          NewState = true;
        break;

      case eINCompanderEnabled:
        break;
        
      case eINPuresignalEnabled:
        break;
        
      case eINEncoder2nd:
        break;
              
      case eINNone:
        break;
    }
    SetLED(GUpdateIndicator, NewState);                     // finally set the LED
  }
}



//
// clip to numerical limtis allowed for a given message type
//
int ClipParameter(int Param, ECATCommands Cmd)
{
  SCATCommands* StructPtr;

  StructPtr = GCATCommands + (int)Cmd;
//
// clip the parameter to the allowed numeric range
//
  if (Param > StructPtr->MaxParamValue)
    Param = StructPtr->MaxParamValue;
  else if (Param < StructPtr->MinParamValue)
    Param = StructPtr->MinParamValue;
  return Param;  
}




//
// set console freq from incoming CAT message
// the strategy is: convert to integer Hz (before this called)
// convert to MHz and Hz
// convert to text & send to display
//
void CatDisplayFreq(long Frequency_Hz)
{
  long Freq_Hz;
  long Freq_MHz;
  char MHzString[20];
  char HzString[10];

  Freq_MHz = Frequency_Hz / 1000000;                        // get int MHz
  Freq_Hz = Frequency_Hz - Freq_MHz * 1000000;              // get int remainder Hz
  LongToString(Freq_MHz, MHzString, 2);
  Append(MHzString, '.');
  LongToString(Freq_Hz, HzString, 6);
  strcat(MHzString, HzString);
  DisplayShowFrequency(MHzString);
}



//
// Periodic request of new CAT data from the PC
// this refreshes "commonly" needed information
// initial suggested sequence:
//  0: Frequency
//  1: S meter
//  2: VFO combined status
//  3: Frequency
//  4: S Meter
//  5: RX Combined status
//  6: S meter
//  7: Mode
//  8: VFO step size
//
void PeriodicRefresh(void)
{
  if (--GPeriodicRefreshTimer < 0)                            // see if timed out
  {
    GPeriodicRefreshTimer =  GPERIODICREFRESHDELAY;           // reload if timed out
    if (++GeriodicRefreshState >= GPERIODICSEQUENCELENGTH)    // update the sequence
      GeriodicRefreshState = 0;
    switch(GeriodicRefreshState)                              // now take the right action
    {
      case 0:                                                 // request frequency
      case 3:
        if (GConsoleVFOA)
          MakeCATMessageNoParam(eZZFA);                       // request A frequency
        else
          MakeCATMessageNoParam(eZZFB);                       // request B frequency
        GLiveRequestFrequency = true;
        break;
      
      case 1:                                                 // request S meter
      case 4:
      case 6:
        if (GCatStateTX == false)
        {
          if (GConsoleVFOA)
            MakeCATMessageNumeric(eZZSM, 0);                  // request A S meter (special case)
          else
            MakeCATMessageNumeric(eZZSM, 1);                  // request B S meter (special case)
        }
        else
          MakeCATMessageNumeric(eZZRM, 5);                    // request TX fwd power meter
        break;
      
      case 2:                                                 // request VFO
        MakeCATMessageNoParam(eZZXV);                         // request combined VFO status message
        GLiveRequestVFOStatus = true;
        break;

      case 5:                                                 // request RX
        if (GConsoleVFOA)
          MakeCATMessageNoParam(eZZXN);                       // request A RX setting
        else
          MakeCATMessageNoParam(eZZXO);                       // request B RX setting
          GLiveRequestRXStatus = true;
        break;

      case 7:                                                 // request Mode
        if (GConsoleVFOA)
          MakeCATMessageNoParam(eZZMD);                       // request A mode
        else
          MakeCATMessageNoParam(eZZME);                       // request B mode
        GLiveRequestMode = true;
        break;

      case 8:
        MakeCATMessageNoParam(eZZSW);                       // request A/B VFO enabled for TX
        break;

      case 9:                                                 // if this state we sequence between infrequently needed values
        if (++GPeriodicRefreshSubState > 2)
          GPeriodicRefreshSubState = 0;
        switch(GPeriodicRefreshSubState)
        {
          case 0:                                             // VFO step size
            MakeCATMessageNoParam(eZZAC);                     // get VFO step size
            break;
            
          case 1:                                             // filter low cut
            if (GConsoleVFOA)
              MakeCATMessageNoParam(eZZFL);                   // request A filter
            else
              MakeCATMessageNoParam(eZZFS);                   // request B filter
            break;
          case 2:                                             // filter high cut
            if (GConsoleVFOA)
              MakeCATMessageNoParam(eZZFH);                   // request A filter
            else
              MakeCATMessageNoParam(eZZFR);                   // request B filter
            break;
        }
        break;
    }
  }
}


//
// check timeouts and recent values
//
void CheckTimeouts(void)
{
// band
  if (GBandTimeout != 0)                          // see if we have a band timeout live
    if(--GBandTimeout == 0)                       // if it reaches zero now, re-request
      CATRequestBand();

// AGC threshold
  if(GAGCThresholdTimeout != 0)                   // decrement AGC threshold timeout if non zero
    if (--GAGCThresholdTimeout == 0)              // if it times out, re-request
      CATRequestAGCThreshold();
  if(GAGCThresholdRecent != 0)                    // just decrement if non zero
    GAGCThresholdRecent--;

// filter low/high
  if(GFilterLowTimeout != 0)                   // decrement filter low cut timeout if non zero
    if (--GFilterLowTimeout == 0)              // if it times out, re-request
      CATRequestFilterLow();
  if(GFilterLowRecent != 0)                    // just decrement if non zero
    GFilterLowRecent--;

  if(GFilterHighTimeout != 0)                   // decrement filter high cut timeout if non zero
    if (--GFilterHighTimeout == 0)              // if it times out, re-request
      CATRequestFilterHigh();
  if(GFilterHighRecent != 0)                    // just decrement if non zero
    GFilterHighRecent--;


// squelch
  if(GSquelchLevelTimeout != 0)                   // decrement squelch level timeout if non zero
    if (--GSquelchLevelTimeout == 0)              // if it times out, re-request
      CATRequestSquelchLevel();
  if(GSquelchLevelRecent != 0)                    // just decrement if non zero
    GSquelchLevelRecent--;


// AF gains
  if(GRX1AFGainTimeout != 0)                    // decrement channel AF gain timeout if non zero
    if (--GRX1AFGainTimeout == 0)               // if it times out, re-request
      CATRequestRX1AFGain();
  if(GRX1AFGainRecent != 0)                     // just decrement if non zero
    GRX1AFGainRecent--;

  if(GRX2AFGainTimeout != 0)                    // decrement channel AF gain timeout if non zero
    if (--GRX2AFGainTimeout == 0)               // if it times out, re-request
      CATRequestRX2AFGain();
  if(GRX2AFGainRecent != 0)                     // just decrement if non zero
    GRX2AFGainRecent--;

  if(GMastAFGainTimeout != 0)                   // decrement master AF gain timeout if non zero
    if (--GMastAFGainTimeout == 0)              // if it times out, re-request
      CATRequestMastAFGain();
  if(GMastAFGainRecent != 0)                    // just decrement if non zero
    GMastAFGainRecent--;


// TX drive
  if(GDriveLevelTimeout != 0)                   // decrement drive level timeout if non zero
    if (--GDriveLevelTimeout == 0)              // if it times out, re-request
      CATRequestDriveLevel();
  if(GDriveLevelRecent != 0)                    // just decrement if non zero
    GDriveLevelRecent--;


// TX mic gain
  if(GMicGainTimeout != 0)                   // decrement TX mic gain timeout if non zero
    if (--GMicGainTimeout == 0)              // if it times out, re-request
      CATRequestMicGain();
  if(GMicGainRecent != 0)                    // just decrement if non zero
    GMicGainRecent--;

// VOX gain/delay
  if(GVoxGainTimeout != 0)                   // decrement VOX gain timeout if non zero
    if (--GVoxGainTimeout == 0)              // if it times out, re-request
      CATRequestVoxGain();
  if(GVoxGainRecent != 0)                    // just decrement if non zero
    GVoxGainRecent--;

  if(GVoxDelayTimeout != 0)                   // decrement VOX delay timeout if non zero
    if (--GVoxDelayTimeout == 0)              // if it times out, re-request
      CATRequestVoxDelay();
  if(GVoxDelayRecent != 0)                    // just decrement if non zero
    GVoxDelayRecent--;


// CW tone/speed
  if(GCWToneTimeout != 0)                   // decrement CW Tone timeout if non zero
    if (--GCWToneTimeout == 0)              // if it times out, re-request
      CATRequestCWTone();
  if(GCWToneRecent != 0)                    // just decrement if non zero
    GCWToneRecent--;

  if(GCWSpeedTimeout != 0)                   // decrement CW speed timeout if non zero
    if (--GCWSpeedTimeout == 0)              // if it times out, re-request
      CATRequestCWSpeed();
  if(GCWSpeedRecent != 0)                    // just decrement if non zero
    GCWSpeedRecent--;

// Diversity Gain
  if(GDiversityGainTimeout != 0)                   // decrement diversity gain timeout if non zero
    if (--GDiversityGainTimeout == 0)              // if it times out, re-request
      CATRequestDiversityGain();
  if(GDiversityGainRecent != 0)                    // just decrement if non zero
    GDiversityGainRecent--;

// diversity phase
  if(GDiversityPhaseTimeout != 0)                   // decrement diversity phase timeout if non zero
    if (--GDiversityPhaseTimeout == 0)              // if it times out, re-request
      CATRequestDiversityPhase();
  if(GDiversityPhaseRecent != 0)                    // just decrement if non zero
    GDiversityPhaseRecent--;

// diversity source
  if(GDiversitySourceTimeout != 0)                   // decrement diversity ref source timeout if non zero
    if (--GDiversitySourceTimeout == 0)              // if it times out, re-request
      CATRequestDiversityRefSource();

  if(GDisplayThrottle != 0)                 // display update throttle
    GDisplayThrottle--;
}


//
// helper to decide if enabled for TX
//
bool IsEnabledForTX(void)
{
  bool Result = false;
  if (((GConsoleVFOA == true) && (GCatStateABTX == false))                // VFO=A, and A enabled
      ||((GConsoleVFOA == false) && (GCatStateABTX == true)))            // VFO=B, and B enabled
    Result = true;
    
  return Result;
}

//
// set ext mox effect
// true if ext mox input active
//
void CATExtMox(bool IsActiveMOX)
{
  GConsoleExtTX = IsActiveMOX;
  if ((GConsoleTX || GConsoleExtTX) && IsEnabledForTX)
    MakeCATMessageNumeric(eZZTX, 1);                            // then send CAT message
  else
    MakeCATMessageNumeric(eZZTX, 0);
  
}



//
// periodic tick
//
void CATHandlerTick(void)
{
  PeriodicRefresh();
  CheckTimeouts();
  UpdateIndicators();
}




//
// toggle between A and B VFOs
// Clear stored clicks, timeouts and recents for message requests that are A/B VFO sensitive
//
void ToggleAB(void)
{
  GConsoleVFOA = !GConsoleVFOA;
  DisplayShowABState(GConsoleVFOA);
  GAGCThresholdTimeout = 0;
  GAGCThresholdRecent = 0;
  GAGCThresholdClicks=0;
  GFilterLowTimeout = 0;
  GFilterLowRecent = 0;
  GFilterLowClicks = 0;
  GFilterHighTimeout = 0;
  GFilterHighRecent = 0;
  GFilterHighClicks = 0;
  GSquelchLevelTimeout = 0;
  GSquelchLevelRecent = 0;
  GSquelchLevelClicks = 0;
}


//
// handler for pushbutton events: 
//
void CATHandlePushbutton(unsigned int Button, EButtonActions Action, bool IsPressed)
{
  switch(Action)
  {
    
    case ePBNone:                             // no assigned function - should be trapped by button.cpp
    case ePBEncoderClick:                     // for dual fn encoders  - should be trapped by button.cpp
      break;
      
    case ePBABVfo:                            // change between A and B. Save the variable then set display.
      if (IsPressed)
        ToggleAB();
      break;
      
    case ePBMox:
      if (IsPressed)                                            // toggle the requested TX/RX state
      {
        if (GConsoleTX)                                         // if already transmitting, toggle
        {
          GConsoleTX = ! GConsoleTX;
          GConsoleTune = false;                                 // tune is ties to TX; in this case we don't want tune
        }
        else if (IsEnabledForTX())                              // if not TXing, but we are enabled to TX, also toggle
        {
          GConsoleTX = ! GConsoleTX;
          GConsoleTune = false;                                 // tune is ties to TX; in this case we don't want tune
        }
        
// now send TX or not TX depending on outcome
        if (GConsoleTX || GConsoleExtTX)                        // then send CAT message
          MakeCATMessageNumeric(eZZTX, 1);
        else
          MakeCATMessageNumeric(eZZTX, 0);
      }
      break;
      
    case ePBTune:
      if (IsPressed)                                            // toggle the requested TX/RX state
      {
        GConsoleTune = ! GConsoleTune;
        GConsoleTX = GConsoleTune;                                    // turn TX on or off with tune
        if (GConsoleTune)                                             // then send CAT message
          MakeCATMessageNumeric(eZZTU, 1);
        else
          MakeCATMessageNumeric(eZZTU, 0);
      }
      break;
      
    case ePBAFMute:
      if (IsPressed)
      {
        GToggleMute = true;
        CATRequestMute();
      }
      break;
      
    case ePBFilterReset:
      CATRequestCWTone();                                       // request sidetone (for CW frequencies)
      GFilterReset = true;                                      // set flag saying to action it
      break;
      
    case ePBBandUp:
      if (IsPressed)
      {
        if(GConsoleVFOA)
          MakeCATMessageNoParam(eZZBU);
        else
          MakeCATMessageNoParam(eZZBB);
        MakeCATMessageNoParam(eZZBS);               // then request band update
      }
      break;
      
    case ePBBandDown:
      if (IsPressed)
      {
        if(GConsoleVFOA)
          MakeCATMessageNoParam(eZZBD);
        else
          MakeCATMessageNoParam(eZZBA);
        MakeCATMessageNoParam(eZZBS);                 // then request band update
      }
      break;
      
    case ePBModeUp:                                   // increment mode, then save
      if (IsPressed)
      {
        if (GCatStateMode == eDRM)                    // if last, wrap to first
          GCatStateMode = eLSB;
        else
          GCatStateMode = (EMode)((int)GCatStateMode + 1);
        CATSetMode(GCatStateMode);                    // send message
        DisplayShowMode(GCatStateMode);
      }
      break;
      
    case ePBModeDown:
      if (IsPressed)
      {
        if (GCatStateMode == eLSB)                    // if first, wrap to last
          GCatStateMode = eDRM;
        else
          GCatStateMode = (EMode)((int)GCatStateMode -1);
        CATSetMode(GCatStateMode);                    // send message
        DisplayShowMode(GCatStateMode);
      }
      break;

    case ePBAGCSpeed:
      if (IsPressed)
      {
        if (GCatStateAGCSpd != eFast)               // increment with wrap
          GCatStateAGCSpd  = (EAGCSpeed)((int)GCatStateAGCSpd + 1);
        else
          GCatStateAGCSpd = eFixed;
        CATSetAGCSpeed(GCatStateAGCSpd);
        DisplayShowAGCSpeed(GCatStateAGCSpd);
      }
      break;
      
    case ePBNBStep:
      if (IsPressed)
      {
        if (GCatStateNB != eNB2)               // increment with wrap
          GCatStateNB  = (ENBState)((int)GCatStateNB + 1);
        else
          GCatStateNB = eNBOff;
        CATSetNBState(GCatStateNB);
        DisplayShowNBState(GCatStateNB);
      }
      break;
      
    case ePBNRStep:
      if (IsPressed)
      {
        if (GCatStateNR != eNR2)               // increment with wrap
          GCatStateNR  = (ENRState)((int)GCatStateNR + 1);
        else
          GCatStateNR = eNROff;
        CATSetNRState(GCatStateNR);
        DisplayShowNRState(GCatStateNR);
      }
      break;
      
    case ePBSNB:
      if (IsPressed)
      {
        GCatStateSNB = !GCatStateSNB;                         // toggle the current variable
        CATSetSNBState(GCatStateSNB);                         // send to CAT
        DisplayShowSNBState(GCatStateSNB);
      }
      break;
      
    case ePBANF:
      if (IsPressed)
      {
        GCatStateANF = !GCatStateANF;                         // toggle the current variable
        CATSetANFState(GCatStateANF);                         // send to CAT
        DisplayShowANFState(GCatStateANF);
      }
      break;
      
    case ePBRIT:
      if (IsPressed)
      {
        if(GCatStateRIT)
          MakeCATMessageBool(eZZRT, false);
        else
          MakeCATMessageBool(eZZRT, true);
      }
      break;
      
    case ePBRITPlus:
      if (IsPressed)
        MakeCATMessageNoParam(eZZRU);
      break;
      
    case ePBRITMinus:
      if (IsPressed)
        MakeCATMessageNoParam(eZZRD);
      break;
      
    case ePBAtoB:
      if (IsPressed) 
        MakeCATMessageNumeric(eZZVS, V_ZZVS_ATOB);
      break;
      
    case ePBBtoA:
      if (IsPressed) 
        MakeCATMessageNumeric(eZZVS, V_ZZVS_BTOA);
      break;
      
    case ePBSwap:
      if (IsPressed) 
        MakeCATMessageNumeric(eZZVS, V_ZZVS_SWAP);
      break;
      
    case ePBSplit:
      if (IsPressed)
      {
        GCatStateSplit = !GCatStateSplit;                         // toggle the current variable
        CATSetSplitOnOff(GCatStateSplit);                         // send to CAT
        DisplayShowSplit(GCatStateSplit);
      }
      break;
      
    case ePBCTune: 
      if (IsPressed)
      {
        if (GConsoleVFOA)                                           // we hold state for A and B CTUNE: change the correct one
        {
          GCatStateACTune = !GCatStateACTune;                       // toggle the current variable
          CATSetCTuneOnOff(GCatStateACTune);                        // send to CAT
        }
        else
        {
          GCatStateBCTune = !GCatStateBCTune;                       // toggle the current variable
          CATSetCTuneOnOff(GCatStateBCTune);                        // send to CAT
        }
      }
      break;
      
    case ePBLock:
      if (IsPressed)
      {
        if (GConsoleVFOA)                                           // we hold state for A and B CTUNE: change the correct one
        {
          GCatStateALock = !GCatStateALock;                       // toggle the current variable
          CATSetVFOLock(GCatStateALock);                        // send to CAT
          DisplayShowLockState(GCatStateALock);
        }
        else
        {
          GCatStateBLock = !GCatStateBLock;                       // toggle the current variable
          CATSetVFOLock(GCatStateBLock);                        // send to CAT
          DisplayShowLockState(GCatStateBLock);
        }
      }
      break;
      
    case ePBStartStop:
      if (IsPressed)
      {
        GToggleRadioOnOff = true;
        CATRequestRadioOnOff();
      }
      break;
      
    case ePBSquelch:
      if (IsPressed)
      {
        GCatStateSquelch = !GCatStateSquelch;                         // toggle the current variable
        CATSetSquelchOnOff(GCatStateSquelch);                         // send to CAT
      }
      break;
      
    case ePBAtten:    
      if (IsPressed)
      {
        if (GCatStateAtten != e30dB)               // increment with wrap
          GCatStateAtten  = (EAtten)((int)GCatStateAtten + 1);
        else
          GCatStateAtten = e0dB;
        CATSetAttenuation(GCatStateAtten);
        DisplayShowAtten(GCatStateAtten);
      }
      break;

    case ePBVoxOnOff:                             // toggle VOX on/off
      if (IsPressed)
      {
        GToggleVoxOnOff = true;
        CATRequestVoxOnOff();
      }
      break;

    case ePBDiversityFastSlow:                    // fast/slow controls
      if (IsPressed)                                          // toggle the requested TX/RX state
        GDiversityStepFast = !GDiversityStepFast;             // toggle the local variable
      break;

    case ePBCompanderEnable:
      break;
      
    case ePBPuresignalEnable:
      break;
      
    case ePBPuresignal2Tone:
      break;
      
    case ePBPuresignalSingleCal:
      break;
      
    case ePBMonEnable:
      break;
      
      
  }
}



//
// handler for encoder events. Has the following parameters: 
// encoder number 0-7, with VFO=7);
// number of +/- clicks
// true if it is the main function, false if 2nd function
//
//
// encoder passes the following parameters: 
// encoder number 0-7, with VFO=7);
// number of +/- clicks
// the programmed function the encoder is assigned to
//
void CATHandleEncoder(unsigned int Encoder, int Clicks, EEncoderActions AssignedAction)
{
//
// firstly re-map an A/B channel AF gain to the correct A/B handler
//
  if (AssignedAction == eENAFGain)
  {
    if (GConsoleVFOA == true)
      AssignedAction = eENRX1AFGain;
    else
      AssignedAction = eENRX2AFGain;
  }

  switch(AssignedAction)
  {
    case eENNoAction:                        // no action - ignore
    case eENMulti:                           // multifunction already trapped
      break;

    case eENAFGain:                         // this shouldn't happen - mapped to RX1 or RX2
      break;    

    case eENMasterGain:
      GMastAFGainClicks += Clicks;          // set how many "unactioned" steps
      if(GMastAFGainRecent != 0)            // if recent current threshold exists, use it
        SendMastAFGainClicks();
      else
        CATRequestMastAFGain();
      break;    

    case eENAGCLevel:                         // update AGC threshold
      GAGCThresholdClicks += Clicks;          // set how many "unactioned" steps
      if(GAGCThresholdRecent != 0)            // if recent current threshold exists, use it
      {
        SendAGCThresholdClicks();
        DisplayShowAGCThreshold(GCatAGCThreshold);
      }
      else
        CATRequestAGCThreshold();
      break;    

//
// IF filter high: behaviour different in LSB-like spectrum reversed modes 
// (drive the opposite filter edge, in th eopposite direction)
//
    case eENFilterHigh:
      if((GCatStateMode == eLSB) || (GCatStateMode == eCWL) || (GCatStateMode == eDIGL))     // if reversed spectrum
      {
        GFilterLowClicks -= Clicks;          // set how many "unactioned" steps
        if(GFilterLowRecent != 0)            // if recent current threshold exists, use it
          SendFilterLowClicks();
        else
          CATRequestFilterLow();
      }
      else                                                                                  // none reversed spectrum
      {
        GFilterHighClicks += Clicks;          // set how many "unactioned" steps
        if(GFilterHighRecent != 0)            // if recent current threshold exists, use it
          SendFilterHighClicks();
        else
          CATRequestFilterHigh();
      }
      break;    

//
// IF filter low: behaviour different in LSB-like spectrum reversed modes 
// (drive the opposite filter edge, in th eopposite direction)
//
    case eENFilterLow:
      if((GCatStateMode == eLSB) || (GCatStateMode == eCWL) || (GCatStateMode == eDIGL))     // if reversed spectrum
      {
        GFilterHighClicks -= Clicks;          // set how many "unactioned" steps
        if(GFilterHighRecent != 0)            // if recent current threshold exists, use it
          SendFilterHighClicks();
        else
          CATRequestFilterHigh();
      }
      else
      {
        GFilterLowClicks += Clicks;          // set how many "unactioned" steps
        if(GFilterLowRecent != 0)            // if recent current threshold exists, use it
          SendFilterLowClicks();
        else
          CATRequestFilterLow();
      }
      break;    

    case eENDrive:
      GDriveLevelClicks += Clicks;          // set how many "unactioned" steps
      if(GDriveLevelRecent != 0)            // if recent current threshold exists, use it
        SendDriveLevelClicks();
      else
        CATRequestDriveLevel();
      break;    

    case eENMicGain:
      GMicGainClicks += Clicks;          // set how many "unactioned" steps
      if(GMicGainRecent != 0)            // if recent current threshold exists, use it
        SendMicGainClicks();
      else
        CATRequestMicGain();
      break;    

    case eENVFOATune:
      CATEncoderVFOABTune(Clicks, true);
      break;    

    case eENVFOBTune:
      CATEncoderVFOABTune(Clicks, false);
      break;    

    case eENVOXGain:
      GVoxGainClicks += Clicks;          // set how many "unactioned" steps
      if(GVoxGainRecent != 0)            // if recent current threshold exists, use it
        SendVoxGainClicks();
      else
        CATRequestVoxGain();
      break;    

    case eENVOXDelay:
      GVoxDelayClicks += Clicks;          // set how many "unactioned" steps
      if(GVoxDelayRecent != 0)            // if recent current threshold exists, use it
        SendVoxDelayClicks();
      else
        CATRequestVoxDelay();
      break;    

    case eENCWSidetone:
      GCWToneClicks += Clicks;          // set how many "unactioned" steps
      if(GCWToneRecent != 0)            // if recent current threshold exists, use it
        SendCWToneClicks();
      else
        CATRequestCWTone();
      break;    

    case eENCWSpeed:
      GCWSpeedClicks += Clicks;          // set how many "unactioned" steps
      if(GCWSpeedRecent != 0)            // if recent current threshold exists, use it
        SendCWSpeedClicks();
      else
        CATRequestCWSpeed();
      break;    

    case eENSquelch:
      GSquelchLevelClicks += Clicks;          // set how many "unactioned" steps
      if(GSquelchLevelRecent != 0)            // if recent current threshold exists, use it
        SendSquelchLevelClicks();
      else
        CATRequestSquelchLevel();
      break;

    case eENDiversityGain:
      GDiversityGainClicks += Clicks;          // set how many "unactioned" steps
      if(GDiversityGainRecent != 0)            // if recent current threshold exists, use it
        SendDiversityGainClicks();
      else
        CATRequestDiversityRefSource();
      break;
      
    case eENDiversityPhase:
      GDiversityPhaseClicks += Clicks;          // set how many "unactioned" steps
      if(GDiversityPhaseRecent != 0)            // if recent current threshold exists, use it
        SendDiversityPhaseClicks();
      else
        CATRequestDiversityPhase();
      break;

    case eENCompanderThreshold:                 // adjust compander threshold
      break;
      
    case eENRX1AFGain:                          // RX1 AF gain (as opposed to current channel AF gain)
      GRX1AFGainClicks += Clicks;               // set how many "unactioned" steps
      if(GRX1AFGainRecent != 0)                 // if recent current threshold exists, use it
        SendRX1AFGainClicks();
      else
        CATRequestRX1AFGain();
      break;
      
    case eENRX2AFGain:                          // RX2 AF gain
      GRX2AFGainClicks += Clicks;               // set how many "unactioned" steps
      if(GRX2AFGainRecent != 0)                 // if recent current threshold exists, use it
        SendRX2AFGainClicks();
      else
        CATRequestRX2AFGain();
      break;
      
    case eENRX1StepAtten:                       // RX1 step atten (badged as RF gain)
      break;
      
    case eENRX2StepAtten:                       // RX2 step atten (badged as RF gain)
      break;
  }
}


//
// VFO encoder: simply request N steps up or down
//
void CATHandleVFOEncoder(int Clicks)
{
  
  if (Clicks != 0)
  {
    GCatFrequency_Hz += GVFOStepSize * Clicks;
    if (GDisplayThrottle == 0)
    {
      CatDisplayFreq(GCatFrequency_Hz);
      GDisplayThrottle = VDISPLAYTHROTTLETICKS;
    }
    GLiveRequestFrequency = false;                        // cancel any CAT frequency update
    if (Clicks < 0)
    {
      if (GConsoleVFOA == true)                           // down N clicks
        MakeCATMessageNumeric(eZZAE, -Clicks);
      else
        MakeCATMessageNumeric(eZZBE, -Clicks);
    }
    else
    {
      if (GConsoleVFOA == true)                           // up N clicks
        MakeCATMessageNumeric(eZZAF, Clicks);
      else
        MakeCATMessageNumeric(eZZBF, Clicks);
    }
  }
}



//
// encoder assigned to VFO tune
// similar to above, but can be A/B
// simply request N steps up or down
//
void CATEncoderVFOABTune(int Clicks, bool IsA)
{
//
// first send the tune messages:
//  
  GLiveRequestFrequency = false;                        // cancel any CAT frequency update
  if (Clicks < 0)
  {
    if (IsA == true)                           // down N clicks
      MakeCATMessageNumeric(eZZAE, -Clicks);
    else
      MakeCATMessageNumeric(eZZBE, -Clicks);
  }
  else if (Clicks > 0)
  {
    if (IsA == true)                           // up N clicks
      MakeCATMessageNumeric(eZZAF, Clicks);
    else
      MakeCATMessageNumeric(eZZBF, Clicks);
  }
//
// now display if the message is for the currently displayed VFO
//
  if ((IsA == GConsoleVFOA) && (Clicks != 0))
  {
    GCatFrequency_Hz += GVFOStepSize * Clicks;
    CatDisplayFreq(GCatFrequency_Hz);
  }
}




/////////////////////////////////////////////////////////////////////////////////////
//
// handlers to set new data from user (display/encoder/button) to CAT handler
// These are needed where the display sets something
// (pushbutton and encoder selection handled by the CAT code directly)
// new frequency: convert to a float frequency, then an int frequency value in Hz
// then back to a string padded with zeroes.
// avoids assumptions about string format entered by the user
//
void CATSetFrequency(char* Frequency)         // string with MHz as ASCII
{
  double FMHz;                                // float value entered by user
  long FreqMHz;                               // integer MHz
  char NewFreq[20];                           // output buffer

  FMHz=atof(Frequency);                       // convert to float freq, in MHz
  GCatFrequency_Hz = (long)(FMHz*1.0E6);      // now int MHz
  CatDisplayFreq(GCatFrequency_Hz);
//
// now convert to freq in Hz string zero padded  
// and send CAT message
// be aware that 11 digits is too big for a long - so pre-pad with zeros
//
  strcpy(NewFreq, "000");
  LongToString(GCatFrequency_Hz, NewFreq+3, 8);
  if(GConsoleVFOA == true)
    MakeCATMessageString(eZZFA, NewFreq);
  else
    MakeCATMessageString(eZZFB, NewFreq);
  GLiveRequestFrequency = false;              // cancel any request for a new freq value
}


//
// lookup the integer to send for a given band as defined by the enum parameter
// this must match the definition of eband!
//
int GBandCATLookup[] =
{
  160,    // e160
  80,     // e80
  60,     // e60
  40,     // e40
  30,     // e30
  20,     // e20
  17,     // e17
  15,     // e15
  12,     // e12
  10,     // e10
  6,      // e6
  888,    // eGen
  2,      // e2
  999     // eWWV
};



///////////////////////   VOX ON/OFF   //////////////////////////
//
// request VOX on/off state
// we don't use timeout as user can simply press again
//
void CATRequestVoxOnOff(void)
{
  MakeCATMessageNoParam(eZZVE);
}
//
// send MUTE request to CAT
//
void CATSetVoxOnOff(bool IsOn)
{
  MakeCATMessageBool(eZZVE, IsOn);
}



///////////////////////   MUTE   //////////////////////////
//
// request AF MUTE state
// we don't use timeout as user can simply press again
//
void CATRequestMute(void)
{
  if (GConsoleVFOA == true)
    MakeCATMessageNoParam(eZZMA);
  else
    MakeCATMessageNoParam(eZZMB);
}
//
// send MUTE request to CAT
//
void CATSetMute(bool IsMute)
{
  if (GConsoleVFOA == true)
    MakeCATMessageBool(eZZMA, IsMute);
  else
    MakeCATMessageBool(eZZMB, IsMute);
}



//////////////////// RADIO START/STOP /////////////////////
//
// request Radio On/Off state
// we don't use timeout as user can simply press again
//
void CATRequestRadioOnOff(void)
{
  MakeCATMessageNoParam(eZZPS);
}
//
// send Radio On/Off request to CAT
//
void CATSetRadioOnOff(bool IsStart)
{
  MakeCATMessageBool(eZZPS, IsStart);
}


/////////////////////// BAND //////////////////////////////
//
// request the current band
// sends a request message, ten sets timeout
//
void CATRequestBand(void)
{
  if (GConsoleVFOA == true)
    MakeCATMessageNoParam(eZZBS);
  else
    MakeCATMessageNoParam(eZZBT);
  GBandTimeout = VGETTIMEOUT;
}


//
// send Band change request to CAT
//
void CATSetBand(EBand Band)
{
  int CATBandNum;

  CATBandNum = GBandCATLookup[(int)Band];             // get the required CAT parameter
  if (GConsoleVFOA == true)
    MakeCATMessageNumeric(eZZBS, CATBandNum);
  else
    MakeCATMessageNumeric(eZZBT, CATBandNum);
}


/////////////////////// MODE //////////////////////////////
//
// send Mode change request to CAT
//
void CATSetMode(EMode Mode)
{
  GCatStateMode = Mode;
  if (GConsoleVFOA == true)
    MakeCATMessageNumeric(eZZMD, (int)Mode);
  else
    MakeCATMessageNumeric(eZZME, (int)Mode);
  GLiveRequestMode = false;
}


//
// send NR change request to CAT
//
void CATSetNRState(ENRState State)
{
  GCatStateNR = State;
  if (GConsoleVFOA == true)
  {
    switch(State)
    {
      case eNROff: MakeCATMessageBool(eZZNR, false);  break;
      case eNR1:   MakeCATMessageBool(eZZNR, true); break;
      case eNR2:   MakeCATMessageBool(eZZNS, true); break;
    }
  }
  else                                                    // VFO B
  {
    switch(State)
    {
      case eNROff: MakeCATMessageBool(eZZNV, false); break;
      case eNR1:   MakeCATMessageBool(eZZNV, true); break;
      case eNR2:   MakeCATMessageBool(eZZNW, true); break;
    }
  }
  GLiveRequestRXStatus = false;
}


//
// send NB change request to CAT
//
void CATSetNBState(ENBState State)
{
  GCatStateNB = State;
  if (GConsoleVFOA == true)
  {
    switch(State)
    {
      case eNBOff: MakeCATMessageBool(eZZNA, false); break;
      case eNB1:   MakeCATMessageBool(eZZNA, true); break;
      case eNB2:   MakeCATMessageBool(eZZNB, true); break;
    }
  }
  else                                                    // VFO B
  {
    switch(State)
    {
      case eNBOff: MakeCATMessageBool(eZZNC, false); break;
      case eNB1:   MakeCATMessageBool(eZZNC, true); break;
      case eNB2:   MakeCATMessageBool(eZZND, true); break;
    }
  }
  GLiveRequestRXStatus = false;
}


//
// send SNB change request to CAT
//
void CATSetSNBState(bool SNBState)
{
  GCatStateSNB = SNBState;
  if (GConsoleVFOA)
    MakeCATMessageBool(eZZNN, SNBState);
  else
    MakeCATMessageBool(eZZNO, SNBState);
  GLiveRequestRXStatus = false;  
}


//
// send ANF change request to CAT
//
void CATSetANFState(bool ANFState)
{
  GCatStateANF = ANFState;
  if (GConsoleVFOA)
    MakeCATMessageBool(eZZNT, ANFState);
  else
    MakeCATMessageBool(eZZNU, ANFState);
  GLiveRequestRXStatus = false;  
}


//
// send AGC speed change request to CAT
//
void CATSetAGCSpeed(EAGCSpeed Speed)
{
  if (GConsoleVFOA == true)
    MakeCATMessageNumeric(eZZGT, (int)Speed);
  else
    MakeCATMessageNumeric(eZZGU, (int)Speed);
  GLiveRequestRXStatus = false;
}


//
//send Squelch on/off request to CAT
//
void CATSetSquelchOnOff(bool State)
{
  GCatStateSquelch = State;
  if (GConsoleVFOA)
    MakeCATMessageBool(eZZSO, State);
  else
    MakeCATMessageBool(eZZSV, State);
  GLiveRequestRXStatus = false;  
}


//
// send SPLIT on/off request to CAT
//
void CATSetSplitOnOff(bool State)
{
  GCatStateSplit = State;
  MakeCATMessageBool(eZZSP, State);
  GLiveRequestVFOStatus = false;  
}


//
// send CTUNE on/off request to CAT
//
void CATSetCTuneOnOff(bool State)
{
  if (GConsoleVFOA)
  {
    MakeCATMessageBool(eZZCN, State);
    GCatStateACTune = State;
  }
  else
  {
    MakeCATMessageBool(eZZCO, State);
    GCatStateBCTune = State;
  }
  GLiveRequestVFOStatus = false;  
}


//
// send VFO Lock on/off request to CAT
//
void CATSetVFOLock(bool State)
{
  if (GConsoleVFOA)
  {
    MakeCATMessageBool(eZZUX, State);
    GCatStateALock = State;
  }
  else
  {
    MakeCATMessageBool(eZZUY, State);
    GCatStateBLock = State;
  }
  GLiveRequestVFOStatus = false;  
}



/////////////////////////////// AGC THRESHOLD ///////////////////////////////////////
//
// request AGC threshold
// if recent data: send the recent data to the display
// if no recent data: sends a request message and sets a timeout
//
void CATRequestAGCThreshold(void)
{
  if(GAGCThresholdRecent != 0)
    DisplayShowAGCThreshold(GCatAGCThreshold);
  else
  {
    if (GConsoleVFOA == true)
      MakeCATMessageNoParam(eZZAR);
    else
      MakeCATMessageNoParam(eZZAS);
    GAGCThresholdTimeout = VGETTIMEOUT;
  }
}


//
// send AGC threshold change request to CAT
// I don't think this is complete - can be changed by an encoder too!
//
void CATSetAGCThreshold(int Threshold)
{
  if (GConsoleVFOA == true)
    MakeCATMessageNumeric(eZZAR, Threshold);
  else
    MakeCATMessageNumeric(eZZAS, Threshold);
  GAGCThresholdRecent = VRECENTTHRESHOLD;
  GCatAGCThreshold = Threshold;
}


//
// deal with an encoder turn: a CAT message has arrived so now we need to update the param and send back to CAT
// update & clip; send CAT; clear stored clicks; send to display
//
void SendAGCThresholdClicks(void)
{
  GCatAGCThreshold += GAGCThresholdClicks;                        // update
  GCatAGCThreshold = ClipParameter(GCatAGCThreshold, eZZAR);      // clip to limits
  CATSetAGCThreshold(GCatAGCThreshold);                           // send CAT command
  GAGCThresholdClicks = 0;                                        // clear the stored clicks
  DisplayShowAGCThreshold(GCatAGCThreshold);
}



#define VFILTERCLICKSTEP 50                                     // Hz per step

/////////////////////////////// FILTER LOW ///////////////////////////////////////
//
// request 
// if recent data: send the recent data to the display
// if no recent data: sends a request message and sets a timeout
//
void CATRequestFilterLow(void)
{
  if(GFilterLowTimeout == 0)
  {
    if (GConsoleVFOA == true)
      MakeCATMessageNoParam(eZZFL);
    else
      MakeCATMessageNoParam(eZZFS);
    GFilterLowTimeout = VGETTIMEOUT;
  }
}


//
// deal with an encoder turn: a CAT message has arrived so now we need to update the param and send back to CAT
// update & clip; send CAT; clear stored clicks; send to display
//
void SendFilterLowClicks(void)
{
  GCatFilterLow += (GFilterLowClicks * VFILTERCLICKSTEP);         // update
  GCatFilterLow = ClipParameter(GCatFilterLow, eZZFL);            // clip to limits
  GFilterLowClicks = 0;                                           // clear the stored clicks
  if (GConsoleVFOA == true)                                       // send the right message
    MakeCATMessageNumeric(eZZFL, GCatFilterLow);
  else
    MakeCATMessageNumeric(eZZFS, GCatFilterLow);
  GFilterLowRecent = VRECENTTHRESHOLD;                            // set "recent"
  DisplayShowFilterLow(GCatFilterLow);                            // send to display

}


/////////////////////////////// FILTER HIGH ///////////////////////////////////////
//
// request 
// if recent data: send the recent data to the display
// if no recent data: sends a request message and sets a timeout
//

void CATRequestFilterHigh(void)
{
  if(GFilterHighTimeout == 0)
  {
    if (GConsoleVFOA == true)
      MakeCATMessageNoParam(eZZFH);
    else
      MakeCATMessageNoParam(eZZFR);
    GFilterHighTimeout = VGETTIMEOUT;
  }
}


//
// deal with an encoder turn: a CAT message has arrived so now we need to update the param and send back to CAT
// update & clip; send CAT; clear stored clicks; send to display
//
void SendFilterHighClicks(void)
{
  GCatFilterHigh += (GFilterHighClicks * VFILTERCLICKSTEP);         // update
  GCatFilterHigh = ClipParameter(GCatFilterHigh, eZZFH);            // clip to limits
  GFilterHighClicks = 0;                                            // clear the stored clicks
  if (GConsoleVFOA == true)                                         // send the right message
    MakeCATMessageNumeric(eZZFH, GCatFilterHigh);
  else
    MakeCATMessageNumeric(eZZFR, GCatFilterHigh);
  GFilterHighRecent = VRECENTTHRESHOLD;                             // set "recent"
  DisplayShowFilterHigh(GCatFilterHigh);                             // send to display
}




//
// reset the IF filters to sensible mode dependent defaults
// CWL, CWU are different - they incorporate the CW sidetone freq 
// (which was requested before this was called)
// we need to send 2 CAT commands; one for low, one for high
//
void ResetIFFilters(int Tone)
{
  int FilterPBLow, FilterPBHigh;                              // passband limits looked up

  GFilterReset = false;                                       // cancel the request now we've done it
  FilterPBLow = GFilterResetLowValues[(int)GCatStateMode];
  FilterPBHigh = GFilterResetHighValues[(int)GCatStateMode];
//
// for CW: use the low/high as freq either cide of CW sidetone  
//
  if (GCatStateMode == eCWL)
  {
    FilterPBLow = FilterPBLow - Tone;
    FilterPBHigh = FilterPBHigh - Tone;
  }
  else if (GCatStateMode == eCWU)
  {
    FilterPBLow = FilterPBLow + Tone;
    FilterPBHigh = FilterPBHigh + Tone;
  }
//
// finally send the messages
//
  if(GConsoleVFOA)
  {
    MakeCATMessageNumeric(eZZFL, FilterPBLow);
    MakeCATMessageNumeric(eZZFH, FilterPBHigh);
  }
  else
  {
    MakeCATMessageNumeric(eZZFS, FilterPBLow);
    MakeCATMessageNumeric(eZZFR, FilterPBHigh);
  }
}



/////////////////////////////// SQUELCH LEVEL ///////////////////////////////////////
//
// request 
// if recent data: send the recent data to the display
// if no recent data: sends a request message and sets a timeout
//
void CATRequestSquelchLevel(void)
{
  if(GSquelchLevelTimeout == 0)
  {
    if (GConsoleVFOA == true)
      MakeCATMessageNoParam(eZZSQ);
    else
      MakeCATMessageNoParam(eZZSX);
    GSquelchLevelTimeout = VGETTIMEOUT;
  }
}


//
// deal with an encoder turn: a CAT message has arrived so now we need to update the param and send back to CAT
// update & clip; send CAT; clear stored clicks; send to display
//
void SendSquelchLevelClicks(void)
{
  GCatSquelchLevel -= GSquelchLevelClicks;                              // update (NOTE SUBTRACT BECAUSE PARAMETER IS NEG BUT CAT VALUE POSITIVE)
  GCatSquelchLevel = ClipParameter(GCatSquelchLevel, eZZSQ);            // clip to limits
  GSquelchLevelClicks = 0;                                              // clear the stored clicks
  if (GConsoleVFOA == true)                                             // send the right message
    MakeCATMessageNumeric(eZZSQ, GCatSquelchLevel);
  else
    MakeCATMessageNumeric(eZZSX, GCatSquelchLevel);
  GSquelchLevelRecent = VRECENTTHRESHOLD;                               // set "recent"
}




/////////////////////////////// RX1  CHANNEL AF GAIN ///////////////////////////////////////
//
// request 
// if recent data: send the recent data to the display
// if no recent data: sends a request message and sets a timeout
//
void CATRequestRX1AFGain(void)
{
  if(GRX1AFGainTimeout == 0)
  {
    MakeCATMessageNoParam(eZZLA);
    GRX1AFGainTimeout = VGETTIMEOUT;
  }
}


//
// deal with an encoder turn: a CAT message has arrived so now we need to update the param and send back to CAT
// update & clip; send CAT; clear stored clicks; send to display
//
void SendRX1AFGainClicks(void)
{
  GCatRX1AFGain += GRX1AFGainClicks;                                // update
  GCatRX1AFGain = ClipParameter(GCatRX1AFGain, eZZLA);              // clip to limits
  GRX1AFGainClicks = 0;                                             // clear the stored clicks
  MakeCATMessageNumeric(eZZLA, GCatRX1AFGain);
  GRX1AFGainRecent = VRECENTTHRESHOLD;                              // set "recent"
}




/////////////////////////////// RX2  CHANNEL AF GAIN ///////////////////////////////////////
//
// request 
// if recent data: send the recent data to the display
// if no recent data: sends a request message and sets a timeout
//
void CATRequestRX2AFGain(void)
{
  if(GRX2AFGainTimeout == 0)
  {
    MakeCATMessageNoParam(eZZLE);
    GRX2AFGainTimeout = VGETTIMEOUT;
  }
}


//
// deal with an encoder turn: a CAT message has arrived so now we need to update the param and send back to CAT
// update & clip; send CAT; clear stored clicks; send to display
//
void SendRX2AFGainClicks(void)
{
  GCatRX2AFGain += GRX2AFGainClicks;                                // update
  GCatRX2AFGain = ClipParameter(GCatRX2AFGain, eZZLE);              // clip to limits
  GRX2AFGainClicks = 0;                                             // clear the stored clicks
  MakeCATMessageNumeric(eZZLE, GCatRX2AFGain);
  GRX2AFGainRecent = VRECENTTHRESHOLD;                              // set "recent"
}




/////////////////////////////// MASTER AF GAIN ///////////////////////////////////////
//
// request 
// if recent data: send the recent data to the display
// if no recent data: sends a request message and sets a timeout
//
void CATRequestMastAFGain(void)
{
  if(GMastAFGainTimeout == 0)
  {
    MakeCATMessageNoParam(eZZAG);
    GMastAFGainTimeout = VGETTIMEOUT;
  }
}


//
// deal with an encoder turn: a CAT message has arrived so now we need to update the param and send back to CAT
// update & clip; send CAT; clear stored clicks; send to display
//
void SendMastAFGainClicks(void)
{
  GCatMastAFGain += GMastAFGainClicks;                                // update
  GCatMastAFGain = ClipParameter(GCatMastAFGain, eZZAG);              // clip to limits
  GMastAFGainClicks = 0;                                              // clear the stored clicks
  MakeCATMessageNumeric(eZZAG, GCatMastAFGain);
  GMastAFGainRecent = VRECENTTHRESHOLD;                               // set "recent"
}




/////////////////////////////// DRIVE LEVEL ///////////////////////////////////////
//
// request 
// if recent data: send the recent data to the display
// if no recent data: sends a request message and sets a timeout
//
void CATRequestDriveLevel(void)
{
  if(GDriveLevelTimeout == 0)
  {
    MakeCATMessageNoParam(eZZPC);
    GDriveLevelTimeout = VGETTIMEOUT;
  }
}


//
// deal with an encoder turn: a CAT message has arrived so now we need to update the param and send back to CAT
// update & clip; send CAT; clear stored clicks; send to display
//
void SendDriveLevelClicks(void)
{
  GCatDriveLevel += GDriveLevelClicks;                              // update
  GCatDriveLevel = ClipParameter(GCatDriveLevel, eZZPC);            // clip to limits
  GDriveLevelClicks = 0;                                            // clear the stored clicks
  MakeCATMessageNumeric(eZZPC, GCatDriveLevel);
  GDriveLevelRecent = VRECENTTHRESHOLD;                             // set "recent"
}





/////////////////////////////// MIC GAIN ///////////////////////////////////////
//
// request 
// if recent data: send the recent data to the display
// if no recent data: sends a request message and sets a timeout
//
void CATRequestMicGain(void)
{
  if(GMicGainTimeout == 0)
  {
    MakeCATMessageNoParam(eZZMG);
    GMicGainTimeout = VGETTIMEOUT;
  }
}


//
// deal with an encoder turn: a CAT message has arrived so now we need to update the param and send back to CAT
// update & clip; send CAT; clear stored clicks; send to display
//
void SendMicGainClicks(void)
{
  GCatMicGain += GMicGainClicks;                              // update
  GCatMicGain = ClipParameter(GCatMicGain, eZZMG);            // clip to limits
  GMicGainClicks = 0;                                         // clear the stored clicks
  MakeCATMessageNumeric(eZZMG, GCatMicGain);
  GMicGainRecent = VRECENTTHRESHOLD;                          // set "recent"
}





/////////////////////////////// VOX GAIN ///////////////////////////////////////
//
// request 
// if recent data: send the recent data to the display
// if no recent data: sends a request message and sets a timeout
//
void CATRequestVoxGain(void)
{
  if(GVoxGainTimeout == 0)
  {
    MakeCATMessageNoParam(eZZVG);
    GVoxGainTimeout = VGETTIMEOUT;
  }
}


//
// deal with an encoder turn: a CAT message has arrived so now we need to update the param and send back to CAT
// update & clip; send CAT; clear stored clicks; send to display
//
void SendVoxGainClicks(void)
{
  GCatVoxGain += GVoxGainClicks;                              // update
  GCatVoxGain = ClipParameter(GCatVoxGain, eZZVG);            // clip to limits
  GVoxGainClicks = 0;                                         // clear the stored clicks
  MakeCATMessageNumeric(eZZVG, GCatVoxGain);
  GVoxGainRecent = VRECENTTHRESHOLD;                          // set "recent"
}





/////////////////////////////// VOX DELAY ///////////////////////////////////////
//
// request 
// if recent data: send the recent data to the display
// if no recent data: sends a request message and sets a timeout
//
void CATRequestVoxDelay(void)
{
  if(GVoxDelayTimeout == 0)
  {
    MakeCATMessageNoParam(eZZXH);
    GVoxDelayTimeout = VGETTIMEOUT;
  }
}


//
// deal with an encoder turn: a CAT message has arrived so now we need to update the param and send back to CAT
// update & clip; send CAT; clear stored clicks; send to display
//
void SendVoxDelayClicks(void)
{
  GCatVoxDelay += GVoxDelayClicks;                              // update
  GCatVoxDelay = ClipParameter(GCatVoxDelay, eZZXH);            // clip to limits
  GVoxDelayClicks = 0;                                          // clear the stored clicks
  MakeCATMessageNumeric(eZZXH, GCatVoxDelay);
  GVoxDelayRecent = VRECENTTHRESHOLD;                           // set "recent"
}




/////////////////////////////// CW SIDETONE ///////////////////////////////////////
//
// request 
// if recent data: send the recent data to the display
// if no recent data: sends a request message and sets a timeout
//
void CATRequestCWTone(void)
{
  if(GCWToneTimeout == 0)
  {
    MakeCATMessageNoParam(eZZCL);
    GCWToneTimeout = VGETTIMEOUT;
  }
}


//
// deal with an encoder turn: a CAT message has arrived so now we need to update the param and send back to CAT
// update & clip; send CAT; clear stored clicks; send to display
//
void SendCWToneClicks(void)
{
  GCatCWTone += GCWToneClicks;                              // update
  GCatCWTone = ClipParameter(GCatCWTone, eZZCL);            // clip to limits
  GCWToneClicks = 0;                                        // clear the stored clicks
  MakeCATMessageNumeric(eZZCL, GCatCWTone);
  GCWToneRecent = VRECENTTHRESHOLD;                         // set "recent"
}



/////////////////////////////// CW SPEED ///////////////////////////////////////
//
// request 
// if recent data: send the recent data to the display
// if no recent data: sends a request message and sets a timeout
//
void CATRequestCWSpeed(void)
{
  if(GCWSpeedTimeout == 0)
  {
    MakeCATMessageNoParam(eZZCS);
    GCWSpeedTimeout = VGETTIMEOUT;
  }
}


//
// deal with an encoder turn: a CAT message has arrived so now we need to update the param and send back to CAT
// update & clip; send CAT; clear stored clicks; send to display
//
void SendCWSpeedClicks(void)
{
  GCatCWSpeed += GCWSpeedClicks;                              // update
  GCatCWSpeed = ClipParameter(GCatCWSpeed, eZZCS);            // clip to limits
  GCWSpeedClicks = 0;                                         // clear the stored clicks
  MakeCATMessageNumeric(eZZCS, GCatCWSpeed);
  GCWSpeedRecent = VRECENTTHRESHOLD;                          // set "recent"
}



//////////////////////////////  DIVERSITY  ////////////////////////////////////////

#define VFASTGAINSTEP 25              // 0.025
#define VSLOWGAINSTEP 1               // 0.001
#define VFASTPHASESTEP 1000           // 10 degrees
#define VSLOWPHASESTEP 25             // 0.25 degrees

//
// helper to return the diversity gain increment, given a number of steps
// the increment is 0.025 if fast, 0.001 if slow
//
int GetDiversityGainIncrement(int Clicks)
{
  int Step;                               // step size
  
  if (GDiversityStepFast)
    Step = VFASTGAINSTEP;
  else
    Step = VSLOWGAINSTEP;
  
  return (Step*Clicks);
}

//
// helper to return the diversity phase increment, given a number of steps
// the increment is 10 degrees if fast, 0.25 if slow
//
long GetDiversityPhaseIncrement(int Clicks)
{
  int Step;                               // step size
  
  if (GDiversityStepFast)
    Step = VFASTPHASESTEP;
  else
    Step = VSLOWPHASESTEP;
  
  return (Step*Clicks);
}

//
// request reference Source
//
void CATRequestDiversityRefSource(void)
{
  if(GDiversitySourceTimeout == 0)
  {
    MakeCATMessageNoParam(eZZDB);
    GDiversitySourceTimeout = VGETTIMEOUT;
  }
}


//
// gain request 
// if no recent data: sends a request message and sets a timeout
//
void CATRequestDiversityGain(void)
{
  if(GDiversityGainTimeout == 0)
  {
    if (GCatDiversityRefSource)                 // if RX1 is the source, request RX
      MakeCATMessageNoParam(eZZDC);
    else
      MakeCATMessageNoParam(eZZDG);
    GDiversityGainTimeout = VGETTIMEOUT;
  }
}


//
// diversity gain
// deal with an encoder turn: a CAT message has arrived so now we need to update the param and send back to CAT
// update & clip; send CAT; clear stored clicks; send to display
//
void SendDiversityGainClicks(void)
{
  GCatDiversityGain += GetDiversityGainIncrement(GDiversityGainClicks);      // update
  GCatDiversityGain = ClipParameter(GCatDiversityGain, eZZDG);            // clip to limits
  GDiversityGainClicks = 0;                                            // clear the stored clicks
  if (GCatDiversityRefSource)                                  // send the right message: if RX1, set RX2
    MakeCATMessageNumeric(eZZDC, GCatDiversityGain);
  else
    MakeCATMessageNumeric(eZZDG, GCatDiversityGain);
  GDiversityGainRecent = VRECENTTHRESHOLD;                              // set "recent"
}



//
// phase request 
// if no recent data: sends a request message and sets a timeout
//
void CATRequestDiversityPhase(void)
{
  if(GDiversityPhaseTimeout == 0)
  {
    MakeCATMessageNoParam(eZZDD);
    GDiversityPhaseTimeout = VGETTIMEOUT;
  }
}


//
// diversity phase
// deal with an encoder turn: a CAT message has arrived so now we need to update the param and send back to CAT
// update & wrap; send CAT; clear stored clicks; send to display
//
void SendDiversityPhaseClicks(void)
{
  GCatDiversityPhase -= GetDiversityPhaseIncrement(GDiversityPhaseClicks);      // update
// now wraparound at 180 degrees
  if (GCatDiversityPhase > 18000)
    GCatDiversityPhase -= 36000;
  else if (GCatDiversityPhase < -18000)
    GCatDiversityPhase += 36000;
  GDiversityPhaseClicks = 0;                                            // clear the stored clicks
  MakeCATMessageNumeric(eZZDD, GCatDiversityPhase);
  GDiversityPhaseRecent = VRECENTTHRESHOLD;                              // set "recent"
}




/////////////////////////////// 10dB step ATTENUATION ///////////////////////////////////////


//
// lookup the attenuation value to sed to CAT from our emum value
// this must match the enum definition!
//
int GAttenLookup[] =
{
  1,  // e0dB,
  2,  // e10dB
  0,  // e20dB
  4   // e30dB
};

//
// send attenuation change request to CAT
//
void CATSetAttenuation(EAtten AttenValue)
{
  if (GConsoleVFOA == true)
    MakeCATMessageNumeric(eZZPA, GAttenLookup[(int)AttenValue]);
  else
    MakeCATMessageNumeric(eZZPB, GAttenLookup[(int)AttenValue]);
  GLiveRequestRXStatus = false;
}




////////////////////////////////// CAT HANDLER RX STRING   ////////////////////////////////////////
//
// handlers for received CAT commands
//
void HandleCATCommandStringParam(ECATCommands MatchedCAT, char* ParsedParam)
{
  int i, j;
  int len;
  bool Found = false;

  len=strlen(ParsedParam);
  
  switch(MatchedCAT)
  {
    case eZZFA:                          // set VFO A absolute freq (special case generate - string)
      if ((GLiveRequestFrequency) && (GConsoleVFOA))
      {
        GCatFrequency_Hz = atoi(ParsedParam);
        CatDisplayFreq(GCatFrequency_Hz);
      }
      break;
      
    case eZZFB:                          // set VFO B absolute freq (special case generate - string)
      if ((GLiveRequestFrequency) && (!GConsoleVFOA))
      {
        GCatFrequency_Hz = atoi(ParsedParam);
        CatDisplayFreq(GCatFrequency_Hz);
      }
      break;
      
    case eZZBS:                         // RX1 set/read band (special case parse - transverter may include non numeric values)
    case eZZBT:                         // RX2 set/read band (special case parse- transverter may include non numeric values)
      if(((GConsoleVFOA == true) && (MatchedCAT == eZZBS)) ||                     // respond to ZZBS if on VFO A, else ZZBT
         ((GConsoleVFOA == false) && (MatchedCAT == eZZBT)))
      {
        i=atoi(ParsedParam);
        GBandTimeout = 0;
        switch(i)
        {
          case 160: GCatStateBand = e160; break;
          case 80: GCatStateBand = e80; break;
          case 60: GCatStateBand = e60; break;
          case 40: GCatStateBand = e40; break;
          case 30: GCatStateBand = e30; break;
          case 20: GCatStateBand = e20; break;
          case 17: GCatStateBand = e17; break;
          case 15: GCatStateBand = e15; break;
          case 12: GCatStateBand = e12; break;
          case 10: GCatStateBand = e10; break;
          case 6: GCatStateBand = e6; break;
          case 888: GCatStateBand = eGen; break;
        }
        DisplayShowBand(GCatStateBand);
      }
      break;
      
    case eZZRM:                        // display TX metering (special case parse - response is param dependent)
      for (i=0; i < len; i++)
        if(toupper(ParsedParam[i]) == 'W')               // find the end of the power string
        {
          ParsedParam[i] = 0;                   // terminate string
          Found = true;
          break;
        }
      if(Found)
      {
        i = atoi(ParsedParam+1);
        DisplayShowTXPower(i);
      }
      break;
  }
}



/////////////////////////////////  CAT HANDLER RX   NUM   ////////////////////////////////////

void HandleCATCommandNumParam(ECATCommands MatchedCAT, int ParsedParam)
{
  int i;
  switch(MatchedCAT)
  {
    case eZZAG:                         // master AG gain
      GMastAFGainTimeout = 0;                                                   // clear timeout
      GCatMastAFGain = ParsedParam;                                             // store locally
      GMastAFGainRecent = VRECENTTHRESHOLD;                                     // set recent
      if (GMastAFGainClicks != 0)                                               // we want to send a new update with encoder clicks
        SendMastAFGainClicks();
      break;
      
    case eZZLA:                         // RX1 AF gain
      GRX1AFGainTimeout = 0;                                                   // clear timeout
      GCatRX1AFGain = ParsedParam;                                             // store locally
      GRX1AFGainRecent = VRECENTTHRESHOLD;                                     // set recent
      if (GRX1AFGainClicks != 0)                                               // we want to send a new update with encoder clicks
        SendRX1AFGainClicks();
      break;

    case eZZLE:                         // RX2 AF gain
      GRX2AFGainTimeout = 0;                                                   // clear timeout
      GCatRX2AFGain = ParsedParam;                                             // store locally
      GRX2AFGainRecent = VRECENTTHRESHOLD;                                     // set recent
      if (GRX2AFGainClicks != 0)                                               // we want to send a new update with encoder clicks
        SendRX2AFGainClicks();
      break;
            
    case eZZPA:                           // RX1 attenuation  (NO HANDLER NEEDED - WE NEVER "GET" THIS - use combined RX msg instead)
    case eZZPB:                           // RX2 attenuation  (NO HANDLER NEEDED - WE NEVER "GET" THIS - use combined RX msg instead)
      break;
      
    case eZZAR:                       // RX1 AGC threshold
    case eZZAS:                       // RX2 AGC threshold
      if(((GConsoleVFOA == true) && (MatchedCAT == eZZAR)) ||                     // respond to ZZAR if on VFO A, else ZZAS
         ((GConsoleVFOA == false) && (MatchedCAT == eZZAS)))
      {
        GAGCThresholdTimeout = 0;                                                 // clear timeout
        GCatAGCThreshold = ParsedParam;                                           // store locally
        GAGCThresholdRecent = VRECENTTHRESHOLD;                                   // set recent
        if (GAGCThresholdClicks != 0)                                             // we want to send a new update with encoder clicks
          SendAGCThresholdClicks();
        DisplayShowAGCThreshold(GCatAGCThreshold);                                // send to display
      }
      break;
      
    case eZZGT:                           // RX1 AGC speed  (NO HANDLER NEEDED - WE NEVER "GET" THIS - use combined RX msg instead)
    case eZZGU:                           // RX2 AGC speed  (NO HANDLER NEEDED - WE NEVER "GET" THIS - use combined RX msg instead)
      break;
      
    case eZZFL:                     // RX1 filter low cut
    case eZZFS:                     // RX2 filter low cut
      if(((GConsoleVFOA == true) && (MatchedCAT == eZZFL)) ||                     // respond to ZZFL if on VFO A, else ZZFS
         ((GConsoleVFOA == false) && (MatchedCAT == eZZFS)))
      {
        GFilterLowTimeout = 0;                                                    // clear timeout
        GCatFilterLow = ParsedParam;                                              // store locally
        GFilterLowRecent = VRECENTTHRESHOLD;                                      // set recent
        if (GFilterLowClicks != 0)                                                // we want to send a new update with encoder clicks
          SendFilterLowClicks();
        else
          DisplayShowFilterLow(GCatFilterLow);                                    // if there are clicks, the filterlowclicks code sends to display
      }
      break;
      
    case eZZFH:                     // RX1 filter high cut
    case eZZFR:                     // RX2 filter high cut
      if(((GConsoleVFOA == true) && (MatchedCAT == eZZFH)) ||                     // respond to ZZFH if on VFO A, else ZZFR
         ((GConsoleVFOA == false) && (MatchedCAT == eZZFR)))
      {
        GFilterHighTimeout = 0;                                                   // clear timeout
        GCatFilterHigh = ParsedParam;                                             // store locally
        GFilterHighRecent = VRECENTTHRESHOLD;                                     // set recent
        if (GFilterHighClicks != 0)                                               // we want to send a new update with encoder clicks
          SendFilterHighClicks();
        else
          DisplayShowFilterHigh(GCatFilterHigh);                                  // if there are clicks, the filterlowclicks code sends to display
      }
      break;
      
    case eZZPC:                         // TX drive
      GDriveLevelTimeout = 0;                                                     // clear timeout
      GCatDriveLevel = ParsedParam;                                               // store locally
      GDriveLevelRecent = VRECENTTHRESHOLD;                                       // set recent
      if (GDriveLevelClicks != 0)                                                 // we want to send a new update with encoder clicks
        SendDriveLevelClicks();
      break;
      
    case eZZMG:                        // TX mic gain
      GMicGainTimeout = 0;                                                        // clear timeout
      GCatMicGain = ParsedParam;                                                  // store locally
      GMicGainRecent = VRECENTTHRESHOLD;                                          // set recent
      if (GMicGainClicks != 0)                                                    // we want to send a new update with encoder clicks
        SendMicGainClicks();
      break;
      
    case eZZAE:                          // VFO A down n steps (never "GET" this, so this never gets a response)
    case eZZAF:                          // VFO A up n steps (never "GET" this, so this never gets a response)
    case eZZBE:                          // VFO B down n steps (never "GET" this, so this never gets a response)
    case eZZBF:                          // VFO B up n steps (never "GET" this, so this never gets a response)
      break;
      
    case eZZVG:                        // VOX gain
      GVoxGainTimeout = 0;                                                        // clear timeout
      GCatVoxGain = ParsedParam;                                                  // store locally
      GVoxGainRecent = VRECENTTHRESHOLD;                                          // set recent
      if (GVoxGainClicks != 0)                                                    // we want to send a new update with encoder clicks
        SendVoxGainClicks();
      break;
      
    case eZZXH:                        // VOX delay
      GVoxDelayTimeout = 0;                                                       // clear timeout
      GCatVoxDelay = ParsedParam;                                                 // store locally
      GVoxDelayRecent = VRECENTTHRESHOLD;                                         // set recent
      if (GVoxDelayClicks != 0)                                                   // we want to send a new update with encoder clicks
        SendVoxDelayClicks();
      break;
      
    case eZZCL:                       // CW sidetone
      GCWToneTimeout = 0;                                                         // clear timeout
      GCatCWTone = ParsedParam;                                                   // store locally
      GCWToneRecent = VRECENTTHRESHOLD;                                           // set recent
      if(GCWToneClicks != 0)                                                      // we want to send a new update with encoder clicks
        SendCWToneClicks();
      if(GFilterReset)                                                            // if filter reset flagged
        ResetIFFilters(GCatCWTone);                                               // update filter low and high
      break;
      
    case eZZCS:                          // CW speed
      GCWSpeedTimeout = 0;                                                        // clear timeout
      GCatCWSpeed = ParsedParam;                                                  // store locally
      GCWSpeedRecent = VRECENTTHRESHOLD;                                          // set recent
      if (GCWSpeedClicks != 0)                                                    // we want to send a new update with encoder clicks
        SendCWSpeedClicks();
      break;
      
    case eZZMD:                          // RX1 mode
      if ((GConsoleVFOA == true) && GLiveRequestMode)
        GCatStateMode = (EMode)ParsedParam;
        DisplayShowMode(GCatStateMode);
      break;
      
    case eZZME:                          // RX2 mode
      if ((GConsoleVFOA == false) && GLiveRequestMode)
        GCatStateMode = (EMode)ParsedParam;
        DisplayShowMode(GCatStateMode);
      break;
      
    case eZZSM:                         // display S meter (Special case parse - needs 0 or 1 digit inserting)
      if((GConsoleVFOA == true) && (ParsedParam < 1000))            // RX1
        DisplayShowSMeter(ParsedParam);
      else if ((GConsoleVFOA == false) && (ParsedParam > 1000))
        DisplayShowSMeter(ParsedParam - 1000);
      break;
      
    case eZZSQ:                         // RX1 squelch level
    case eZZSX:                         // RX2 squelch level
      if(((GConsoleVFOA == true) && (MatchedCAT == eZZSQ)) ||                     // respond to ZZSQ if on VFO A, else ZZSX
         ((GConsoleVFOA == false) && (MatchedCAT == eZZSX)))
      {
        GSquelchLevelTimeout = 0;                                                 // clear timeout
        GCatSquelchLevel = ParsedParam;                                           // store locally
        GSquelchLevelRecent = VRECENTTHRESHOLD;                                   // set recent
        if (GSquelchLevelClicks != 0)                                             // we want to send a new update with encoder clicks
          SendSquelchLevelClicks();
      }
      break;
      
    case eZZXN:                        // RX1 combined status
    case eZZXO:                        // RX2 combined status
      if((((GConsoleVFOA == true) && (MatchedCAT == eZZXN)) ||                     // respond to ZZXN if on VFO A, else ZZXO
      ((GConsoleVFOA == false) && (MatchedCAT == eZZXO))) && GLiveRequestRXStatus)
      {
        i = ParsedParam & 0b0000000000111;                  // get AGC speed
        if (i<5)
          GCatStateAGCSpd = (EAGCSpeed)i;                   // RX1/2 atten
        else
          GCatStateAGCSpd = eMedium;
        DisplayShowAGCSpeed(GCatStateAGCSpd);

        i = (ParsedParam >> 3) & 0b0000000000111;           // get atten
        switch(i)
        {
          case 0: GCatStateAtten = e20dB; break;
          case 1: GCatStateAtten = e0dB; break;
          case 2: GCatStateAtten = e10dB; break;
          case 3: GCatStateAtten = e20dB; break;
          case 4: GCatStateAtten = e30dB; break;
          case 5: GCatStateAtten = e20dB; break;
          case 6: GCatStateAtten = e20dB; break;
          case 7: GCatStateAtten = e20dB; break;
        }
        DisplayShowAtten(GCatStateAtten);
        
        GCatStateSquelch = ((ParsedParam & 0b0000001000000) != 0)?true:false;                          // RX1/2 squelch
        GCatStateSNB = ((ParsedParam & 0b0100000000000) != 0)?true:false;                              // RX1/2 SNB
        GCatStateANF = ((ParsedParam & 0b1000000000000) != 0)?true:false;                              // RX1/2 ANF
        DisplayShowSNBState(GCatStateSNB);
        DisplayShowANFState(GCatStateANF);

        i = (ParsedParam >> 7) & 0b11;                      // 2 NB bits
        switch(i)
        {
          case 0: GCatStateNB = eNBOff; break;
          case 1: GCatStateNB = eNB1; break;
          case 2: GCatStateNB = eNB2; break;
          case 3: GCatStateNB = eNBOff; break;
        }
        DisplayShowNBState(GCatStateNB);

        i = (ParsedParam >> 9) & 0b11;                      // 2 NR bits
        switch(i)
        {
          case 0: GCatStateNR = eNROff; break;
          case 1: GCatStateNR = eNR1; break;
          case 2: GCatStateNR = eNR2; break;
          case 3: GCatStateNR = eNROff; break;
        }
        DisplayShowNRState(GCatStateNR);
      }
      break;
      
    case eZZXV:                          // VFO combined status
      if (GLiveRequestVFOStatus)
      {
        GCatStateTX =     ((ParsedParam & 0b01000000) != 0)?true:false;             // true if CAT has reported TX
        GCatStateTune =   ((ParsedParam & 0b10000000) != 0)?true:false;             // true if CAT has reported Tune
        GCatStateRIT =    ((ParsedParam & 0b00000001) != 0)?true:false;             // true if CAT reported RIT is on
        GCatStateALock =  ((ParsedParam & 0b00000010) != 0)?true:false;             // true if VFO A lock is on
        GCatStateBLock =  ((ParsedParam & 0b00000100) != 0)?true:false;             // true if VFO B lock is on
        GCatStateSplit =  ((ParsedParam & 0b00001000) != 0)?true:false;             // true if SPLIT state set
        GCatStateACTune = ((ParsedParam & 0b00010000) != 0)?true:false;             // true if VFO A CTUNE is on
        GCatStateBCTune = ((ParsedParam & 0b00100000) != 0)?true:false;             // true if VFO B CTUNE is on
        DisplayShowTXState(GCatStateTX, GCatStateTune);
        DisplayShowSplit(GCatStateSplit);
        DisplayShowRITState(GCatStateRIT);

        if (GConsoleVFOA)
          DisplayShowLockState(GCatStateALock);
        else
          DisplayShowLockState(GCatStateBLock);
      }
      break;

    case eZZAC:
      GVFOStepSize = GStepLookup[ParsedParam];
      break;

    case eZZDC:                                                       // diversity RX2 gain
    case eZZDG:                                                       // diversity RX1 gain
      GDiversityGainTimeout = 0;                                                   // clear timeout
      GCatDiversityGain = ParsedParam;                                             // store locally
      GDiversityGainRecent = VRECENTTHRESHOLD;                                     // set recent
      if (GDiversityGainClicks != 0)                                               // we want to send a new update with encoder clicks
        SendDiversityGainClicks();
      break;

    case eZZDD:                                                       // diversity phase
      GDiversityPhaseTimeout = 0;                                                   // clear timeout
      GCatDiversityPhase = ParsedParam;                                             // store locally
      GDiversityPhaseRecent = VRECENTTHRESHOLD;                                     // set recent
      if (GDiversityPhaseClicks != 0)                                               // we want to send a new update with encoder clicks
        SendDiversityPhaseClicks();
      break;

    case eZZDH:                                                       // diversity receiver source
      break;

    case eZZRX:                                                       // RX1 step atten
      break;

    case eZZRY:                                                       // RX2 step atten
      break;

    case eZZCT:                                                       // compander threshold
      break;
    
  }
}



///////////////////////////// CAT HANDLER RX BOOL   /////////////////////////////////////


void HandleCATCommandBoolParam(ECATCommands MatchedCAT, bool ParsedParam)
{
  switch(MatchedCAT)
  {
    case eZZVE:                          // VOX on/off
      if (GToggleVoxOnOff)
      {
        CATSetVoxOnOff(!ParsedParam);
        GToggleVoxOnOff = false;
      }
      break;
      
    case eZZTX:                          // MOX state (NO HANDLER - never "get" - polled using ZZXV)
    case eZZTU:                          // TUNE state (NO HANDLER - never "get" - polled using ZZXV)
    case eZZRT:                          // RIT state (NO HANDLER - never "get" - polled using ZZXV)
    case eZZSP:                          // SPLIT state (NO HANDLER - never "get" - polled using ZZXV)
    case eZZCN:                          // RX1 click tune (NO HANDLER - never "get" - polled using ZZXV)
    case eZZCO:                          // RX2 click tune (NO HANDLER - never "get" - polled using ZZXV)
    case eZZUX:                          // VFO A LOCK state (NO HANDLER - never "get" - polled using ZZXV)
    case eZZUY:                          // VFO B LOCK state (NO HANDLER - never "get" - polled using ZZXV)
      break;
      
    case eZZNR:                          // RX1 NR mode (NO HANDLER - never "get" - polled using ZZXN)
    case eZZNS:                          // RX1 enhanced NR mode (NO HANDLER - never "get" - polled using ZZXN)
    case eZZNV:                          // RX2 NR mode (NO HANDLER - never "get" - polled using ZZXO)
    case eZZNW:                          // RX2 enhanced NR mode (NO HANDLER - never "get" - polled using ZZXO)
    case eZZNA:                          // RX1 NB mode (NO HANDLER - never "get" - polled using ZZXN)
    case eZZNB:                          // RX1 enhanced NB mode (NO HANDLER - never "get" - polled using ZZXN)
    case eZZNC:                          // RX2 NB mode (NO HANDLER - never "get" - polled using ZZXO)
    case eZZND:                          // RX2 enhanced NB mode (NO HANDLER - never "get" - polled using ZZXO)
    case eZZNN:                          // RX1 SNB (NO HANDLER - never "get" - polled using ZZXN)
    case eZZNO:                          // RX2 SNB (NO HANDLER - never "get" - polled using ZZXO)
    case eZZNT:                          // RX1 ANF (NO HANDLER - never "get" - polled using ZZXN)
    case eZZNU:                          // RX2 ANF (NO HANDLER - never "get" - polled using ZZXO)
      break;
      
    case eZZMA:                          // RX1 mute
    case eZZMB:                          // RX2 mute
      if(((GConsoleVFOA == true) && (MatchedCAT == eZZMA)) ||                     // respond to ZZMA if on VFO A, else ZZMB
        ((GConsoleVFOA == false) && (MatchedCAT == eZZMB)))
      {
        if (GToggleMute)
        {
          CATSetMute(!ParsedParam);
          GToggleMute = false;
        }
      }
      break;
      
    case eZZPS:                          // radio START
      if (GToggleRadioOnOff)
      {
        CATSetRadioOnOff(!ParsedParam);
        GToggleRadioOnOff = false;
      }
      break;
      
    case eZZSO:                          // RX1 squelch on/off  (NO HANDLER - never "get" - polled using ZZXN)
      break;
      
    case eZZSV:                          // RX2 squelch on/off  (NO HANDLER - never "get" - polled using ZZXO)
      break;

    case eZZDB:                          // diversity reference source 
      GCatDiversityRefSource = ParsedParam;
      GDiversitySourceTimeout = 0;
      CATRequestDiversityGain();
      break;

    case eZZSW:
      GCatStateABTX = ParsedParam;
      break;
      
    case eZZDE:                          // diversity enable
      break;

    case eZZCP:                         // compander enable
      break;

    case eZZLI:                         // puresignal enable
      break;

    case eZZUT:                         // puresignal 2 tone test
      break;

    case eZZMO:                         // MON on/off
      break;
  }      
}

// are there any of these?
void HandleCATCommandNoParam(ECATCommands MatchedCAT)
{
  Serial.println("no parameter");
}


