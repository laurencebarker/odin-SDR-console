/////////////////////////////////////////////////////////////////////////
//
// Odin SDR Console by Laurence Barker G8NJJ
//
// this sketch provides a knob and switch interface through USB and CAT
//
// globalinclude.h
// this file holds #defines for conditional compilation
/////////////////////////////////////////////////////////////////////////
#ifndef __globalinclude_h
#define __globalinclude_h

//
// define NODISPLAY to compile with no attacked nextion display
//
// #define NODISPLAY 1


//
// define V2HARDWARE to compile for Kjell's Andromeda prototype PCB
// design proving initally using Odin software
//
#define V2HARDWARE 1


//
// define SENSORDEBUG to make pushbuttons and encoders send debug strings not operate CAT commands
// also periodic CAT requests and CAT parser disabled too.
//
#define SENSORDEBUG 1




//
// if V2 hardware, automatically select no display because there isn't one
//
#ifdef V2HARDWARE
#define NODISPLAY 1
#endif




#endif      // file sentry
