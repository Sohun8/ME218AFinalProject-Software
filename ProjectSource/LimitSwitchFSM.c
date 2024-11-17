/****************************************************************************
 Module
   ButtonDebounceFSM.c

 
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "LimitSwitchFSM.h"
#include "PIC32PortHAL.h"
#include "RocketLaunchGameFSM.h"
#include "dbprintf.h"

/*----------------------------- Module Defines ----------------------------*/
#define PIN_PORT _Port_B
#define PIN_NUM _Pin_9
#define PIN_READ PORTBbits.RB9;
#define DEBOUNCE_TIME 5 // milliseconds
/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine.They should be functions
   relevant to the behavior of this state machine
*/


/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well.
// type of state variable should match that of enum in header file
static LimitSwitchState_t CurrentState;

// with the introduction of Gen2, we need a module level Priority var as well
static uint8_t MyPriority;

static bool LastState;


bool InitLimitSwitchFSM(uint8_t Priority)
{
  ES_Event_t ThisEvent;

  MyPriority = Priority;
  CurrentState = InitLimitSwitch;
  InitLimitSwitchState(); // Initialize LastState variable
  // post the initial transition event
  ThisEvent.EventType = ES_INIT;
  if (ES_PostToService(MyPriority, ThisEvent) == true)
  {
    return true;
  }
  else
  {
    return false;
  }
}


bool PostLimitSwitchFSM(ES_Event_t ThisEvent)
{
  return ES_PostToService(MyPriority, ThisEvent);
}



ES_Event_t RunLimitSwitchFSM(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors

  switch (CurrentState)
  {
    case InitLimitSwitch:      
    {
      if (ThisEvent.EventType == ES_INIT) 
      {
        CurrentState = LimitSwitchWaiting;
      }
    }
    break;
    
    case LimitSwitchWaiting:      
    {
        switch (ThisEvent.EventType)
        {
            case ES_BUTTON_DOWN:
            {
                if (ThisEvent.EventParam == 'L'){
                    CurrentState = LimitSwitchDebouncingRise;
                    ES_Timer_InitTimer(LIMIT_SWITCH_DEBOUNCE_TIMER, DEBOUNCE_TIME);
                }
            }
            break;
            
            case ES_BUTTON_UP:
            {
                if (ThisEvent.EventParam == 'L'){
                    CurrentState = LimitSwitchDebouncingFall;
                    ES_Timer_InitTimer(LIMIT_SWITCH_DEBOUNCE_TIMER, DEBOUNCE_TIME);
                }
            }
        }
    }
    break;
    
    case LimitSwitchDebouncingRise:      
    {
        switch (ThisEvent.EventType)
        {
            case ES_TIMEOUT:
            {
                ES_Event_t Event2Post;
                Event2Post.EventType = ES_LIMIT_SWITCH;
                PostRocketLaunchGameFSM(Event2Post);
                CurrentState = LimitSwitchWaiting;
                DB_printf("\nLimit Switch Pressed\n");
            }
            break;
            
            case ES_BUTTON_UP:
            {
                CurrentState = LimitSwitchWaiting;
            }
            break;
        }
    }
    break;
    
    case LimitSwitchDebouncingFall:      
    {
        switch (ThisEvent.EventType)
        {
            case ES_TIMEOUT:
            {
                CurrentState = LimitSwitchWaiting;
            }
            break;
            
            case ES_BUTTON_DOWN:
            {
                CurrentState = LimitSwitchWaiting;
            }
            break;
        }
    }
    break;    
    
    default:
      ;
  }                                   // end switch on Current State
  return ReturnEvent;
}


LimitSwitchState_t QueryLimitSwitchSM(void)
{
  return CurrentState;
}

/***************************************************************************
 private functions
 ***************************************************************************/
void InitLimitSwitchState(void){
  // initialize port line as a digital input 
  PortSetup_ConfigureDigitalInputs(PIN_PORT, PIN_NUM);
  // initialize LastState
  LastState = PIN_READ;
}

bool CheckLimitSwitch(void){
  bool ReturnVal = false;
  // read from port pin
  const bool CurrentState = PIN_READ;
  if (CurrentState != LastState){
    ReturnVal = true;
    ES_Event_t ThisEvent;
    ThisEvent.EventParam = 'L';
    if (CurrentState == 1){
        ThisEvent.EventType = ES_BUTTON_DOWN;
    }
    else{
        ThisEvent.EventType = ES_BUTTON_UP;      
    }
    PostLimitSwitchFSM(ThisEvent);
    LastState = CurrentState;
  }
  return ReturnVal;    
}