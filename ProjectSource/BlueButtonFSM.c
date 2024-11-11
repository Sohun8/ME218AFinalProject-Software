/****************************************************************************
 Module
   BlueButtonFSM.c

 
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "BlueButtonFSM.h"
#include "PIC32PortHAL.h"
#include "RocketLaunchGameFSM.h"
#include "dbprintf.h"

/*----------------------------- Module Defines ----------------------------*/
#define PIN_PORT _Port_B
#define PIN_NUM _Pin_12
#define PIN_READ PORTBbits.RB12
#define DEBOUNCE_TIME 5 // milliseconds
/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine.They should be functions
   relevant to the behavior of this state machine
*/


/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well.
// type of state variable should match that of enum in header file
static BlueButtonState_t CurrentState;

// with the introduction of Gen2, we need a module level Priority var as well
static uint8_t MyPriority;

static bool LastState;


bool InitBlueButtonFSM(uint8_t Priority)
{
  ES_Event_t ThisEvent;

  MyPriority = Priority;
  CurrentState = InitBlueButton;
  InitBlueButtonState(); // Initialize LastState variable
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


bool PostBlueButtonFSM(ES_Event_t ThisEvent)
{
  return ES_PostToService(MyPriority, ThisEvent);
}



ES_Event_t RunBlueButtonFSM(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors

  switch (CurrentState)
  {
    case InitBlueButton:      
    {
      if (ThisEvent.EventType == ES_INIT) 
      {
        CurrentState = BlueWaiting;
      }
    }
    break;
    
    case BlueWaiting:      
    {
        switch (ThisEvent.EventType)
        {
            case ES_BUTTON_DOWN:
            {
                if (ThisEvent.EventParam == 'B'){
                    CurrentState = BlueDebouncingRise;
                    ES_Timer_InitTimer(BLUE_BUTTON_DEBOUNCE_TIMER, DEBOUNCE_TIME);
                }
            }
            break;
            
            case ES_BUTTON_UP:
            {
                if (ThisEvent.EventParam == 'B'){
                    CurrentState = BlueDebouncingFall;
                    ES_Timer_InitTimer(BLUE_BUTTON_DEBOUNCE_TIMER, DEBOUNCE_TIME);
                }
            }
        }
    }
    break;
    
    case BlueDebouncingRise:      
    {
        switch (ThisEvent.EventType)
        {
            case ES_TIMEOUT:
            {
                ES_Event_t Event2Post;
                Event2Post.EventType = ES_BUTTON_PRESS;
                Event2Post.EventParam = 'B';
                PostRocketLaunchGameFSM(Event2Post);
                CurrentState = BlueWaiting;
                DB_printf("\nBlue Button Pressed\n");
            }
            break;
            
            case ES_BUTTON_UP:
            {
                CurrentState = BlueWaiting;
            }
            break;
        }
    }
    break;
    
    case BlueDebouncingFall:      
    {
        switch (ThisEvent.EventType)
        {
            case ES_TIMEOUT:
            {
                CurrentState = BlueWaiting;
            }
            break;
            
            case ES_BUTTON_DOWN:
            {
                CurrentState = BlueWaiting;
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


BlueButtonState_t QueryBlueButtonSM(void)
{
  return CurrentState;
}

/***************************************************************************
 private functions
 ***************************************************************************/
void InitBlueButtonState(void){
  // initialize port line as a digital input with a pullup
  PortSetup_ConfigureDigitalInputs(PIN_PORT, PIN_NUM);
  // initialize LastState
  LastState = PIN_READ;
}

bool CheckBlueButton(void){
  bool ReturnVal = false;
  // read from port pin
  const bool CurrentState = PIN_READ;
  if (CurrentState != LastState){
    ReturnVal = true;
    ES_Event_t ThisEvent;
    ThisEvent.EventParam = 'B';
    if (CurrentState == 1){
        ThisEvent.EventType = ES_BUTTON_DOWN;
    }
    else{
        ThisEvent.EventType = ES_BUTTON_UP;      
    }
    PostBlueButtonFSM(ThisEvent);
    LastState = CurrentState;
  }
  return ReturnVal;    
}