/****************************************************************************
 Module
   ButtonDebounceFSM.c

 
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "RedButtonFSM.h"
#include "PIC32PortHAL.h"
#include "RocketLaunchGameFSM.h"
#include "dbprintf.h"

/*----------------------------- Module Defines ----------------------------*/
#define PIN_PORT _Port_B
#define PIN_NUM _Pin_10
#define PIN_READ PORTBbits.RB10;
#define DEBOUNCE_TIME 5 // milliseconds
/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine.They should be functions
   relevant to the behavior of this state machine
*/


/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well.
// type of state variable should match that of enum in header file
static RedButtonState_t CurrentState;

// with the introduction of Gen2, we need a module level Priority var as well
static uint8_t MyPriority;

static bool LastState;


bool InitRedButtonFSM(uint8_t Priority)
{
  ES_Event_t ThisEvent;

  MyPriority = Priority;
  CurrentState = InitRedButton;
  InitRedButtonState(); // Initialize LastState variable
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


bool PostRedButtonFSM(ES_Event_t ThisEvent)
{
  return ES_PostToService(MyPriority, ThisEvent);
}



ES_Event_t RunRedButtonFSM(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors

  switch (CurrentState)
  {
    case InitRedButton:      
    {
      if (ThisEvent.EventType == ES_INIT) 
      {
        CurrentState = RedWaiting;
      }
    }
    break;
    
    case RedWaiting:      
    {
        switch (ThisEvent.EventType)
        {
            case ES_BUTTON_DOWN:
            {
                if (ThisEvent.EventParam == 'R'){
                    CurrentState = RedDebouncingRise;
                    ES_Timer_InitTimer(RED_BUTTON_DEBOUNCE_TIMER, DEBOUNCE_TIME);
                }
            }
            break;
            
            case ES_BUTTON_UP:
            {
                if (ThisEvent.EventParam == 'R'){
                    CurrentState = RedDebouncingFall;
                    ES_Timer_InitTimer(RED_BUTTON_DEBOUNCE_TIMER, DEBOUNCE_TIME);
                }
            }
        }
    }
    break;
    
    case RedDebouncingRise:      
    {
        switch (ThisEvent.EventType)
        {
            case ES_TIMEOUT:
            {
                ES_Event_t Event2Post;
                Event2Post.EventType = ES_BUTTON_PRESS;
                Event2Post.EventParam = 'R';
                PostRocketLaunchGameFSM(Event2Post);
                CurrentState = RedWaiting;
                //DB_printf("\nRed Button Pressed\n");
            }
            break;
            
            case ES_BUTTON_UP:
            {
                CurrentState = RedWaiting;
            }
            break;
        }
    }
    break;
    
    case RedDebouncingFall:      
    {
        switch (ThisEvent.EventType)
        {
            case ES_TIMEOUT:
            {
                CurrentState = RedWaiting;
            }
            break;
            
            case ES_BUTTON_DOWN:
            {
                CurrentState = RedWaiting;
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


RedButtonState_t QueryRedButtonSM(void)
{
  return CurrentState;
}

/***************************************************************************
 private functions
 ***************************************************************************/
void InitRedButtonState(void){
  // initialize port line as a digital input with a pullup
  PortSetup_ConfigureDigitalInputs(PIN_PORT, PIN_NUM);
  // initialize LastState
  LastState = PIN_READ;
}

bool CheckRedButton(void){
  bool ReturnVal = false;
  // read from port pin
  const bool CurrentState = PIN_READ;
  if (CurrentState != LastState){
    ReturnVal = true;
    ES_Event_t ThisEvent;
    ThisEvent.EventParam = 'R';
    if (CurrentState == 1){
        ThisEvent.EventType = ES_BUTTON_DOWN;
    }
    else{
        ThisEvent.EventType = ES_BUTTON_UP;      
    }
    PostRedButtonFSM(ThisEvent);
    LastState = CurrentState;
  }
  return ReturnVal;    
}