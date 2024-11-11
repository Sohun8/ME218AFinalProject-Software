/****************************************************************************
 Module
   IRLaunchEventChecker.c

 Revision
   1.0.1

 Description
   Implementation of the IR Launch Sensor event checker 

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
#include "IRLaunchEventChecker.h"

#include "PIC32PortHAL.h"
#include "dbprintf.h"


/*---------------------------- Module Variables ---------------------------*/
#define PIN_PORT _Port_B
#define PIN_NUM _Pin_13
#define PIN_READ PORTBbits.RB13;

static bool LastState;

void InitIRLaunchSensorStatus(void)
{
  // initialize port line as a digital input
  PortSetup_ConfigureDigitalInputs(PIN_PORT, PIN_NUM);

  // initialize LastButtonState
  LastState = PIN_READ;
}


bool CheckIRLaunchEvents(void)
{
  bool ReturnVal = false;
  // read from port pin
  const bool CurrentState = PIN_READ;
  // if the sensor state has changed and signals blocking of IR sensor
  if (CurrentState != LastState && CurrentState == 0){
        ReturnVal = true;
        ES_Event_t ThisEvent;
        ThisEvent.EventType = ES_IR_LAUNCH;
    // post to RocketLaunchGame state machine
    PostRocketLaunchGameFSM(ThisEvent);
    DB_printf("\nIR Sensor Activated\n");
  }
  LastState = CurrentState;
  return ReturnVal;
}

