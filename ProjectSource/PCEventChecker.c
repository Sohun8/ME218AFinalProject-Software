/****************************************************************************
 Module
   PCEventChecker.c

 Revision
   1.0.1

 Description
   Implementation of the poker chip insertion event checker 

 History
 When           Who     What/Why
 -------------- ---     --------
 10/25/24 22:14 sjp     initial version
****************************************************************************/

// this will pull in the symbolic definitions for events, which we will want
// to post in response to detecting events
#include "ES_Configure.h"
// This gets us the prototype for ES_PostAll
#include "ES_Framework.h"
// this will get us the structure definition for events, which we will need
// in order to post events in response to detecting events
#include "ES_Events.h"
// if you want to use distribution lists then you need those function
// definitions too.
#include "ES_PostList.h"
// This include will pull in all of the headers from the service modules
// providing the prototypes for all of the post functions
#include "ES_ServiceHeaders.h"
// this test harness for the framework references the serial routines that
// are defined in ES_Port.c
#include "ES_Port.h"
// include own prototypes to insure consistency between header &
// actual function definitions
#include "PCEventChecker.h"

#include "PIC32PortHAL.h"


/*---------------------------- Module Variables ---------------------------*/
#define PIN_PORT _Port_B
#define PIN_NUM _Pin_4
#define PIN_READ PORTBbits.RB4;

static bool LastState;

/****************************************************************************
 Function
   InitPCSensorStatus
 Parameters
   None
 Returns
   Nothing
 Description
   Initializes port line for sensor input and 
   inititalizes LastState
 Author
   Sohun Patel, 11/08/24, 15:56
****************************************************************************/
void InitPCSensorStatus(void)
{
  // initialize port line as a digital input with a pullup
  PortSetup_ConfigureDigitalInputs(PIN_PORT, PIN_NUM);
  //PortSetup_ConfigurePullUps(_Port_B, _Pin_13);
  // initialize LastButtonState
  LastState = PIN_READ;
}


/****************************************************************************
 Function
    CheckPCDetectionEvents

 Parameters
    None

 Returns
    bool, true if the state of the poker chip sensor has changed
    and indicates a chip insertion

 Description
    Event checker that reads the current state of the poker chip sensor
    line and posts an event to the RocketLaunchGame state machine
    if the line has changed. 
 Author
   Sohun Patel, 11/08/24, 15:56
****************************************************************************/
bool CheckPCDetectionEvents(void)
{
  bool ReturnVal = false;
  // read from port pin
  const bool CurrentState = PIN_READ;
  // if the sensor state has changed and signals being blocked by a poker chip
  if (CurrentState != LastState && CurrentState == 0){
        ReturnVal = true;
        ES_Event_t ThisEvent;
        ThisEvent.EventType = ES_PC_INSERTED;
    // post to RocketLaunchGame state machine
    PostRocketLaunchGameFSM(ThisEvent);
  }
  LastState = CurrentState;
  return ReturnVal;
}

