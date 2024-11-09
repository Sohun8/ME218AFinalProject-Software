/****************************************************************************
 Module
   RocketLaunchGameFSM.c

 Revision
   1.0.1

 Description
   This implements the RocketLaunchGame flat state machines under the
   Gen2 Events and Services Framework.

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 01/15/12 11:12 jec      revisions for Gen2 framework
 11/07/11 11:26 jec      made the queue static
 10/30/11 17:59 jec      fixed references to CurrentEvent in RunTemplateSM()
 10/23/11 18:20 jec      began conversion from SMTemplate.c (02/20/07 rev)
 ****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
 */
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "RocketLaunchGameFSM.h"
#include "PCEventChecker.h"
#include "terminal.h"
#include "dbprintf.h"

/*----------------------------- Module Defines ----------------------------*/
#define SCROLL_DURATION 250 // milliseconds
/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine.They should be functions
   relevant to the behavior of this state machine
 */

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well.
// type of state variable should match htat of enum in header file
static RocketLaunchGameState_t CurrentState;

// with the introduction of Gen2, we need a module level Priority var as well
static uint8_t MyPriority;

static char* pMessage; // pointer to message string
/*------------------------------ Module Code ------------------------------*/

/****************************************************************************
 Function
     InitRocketLaunchGameFSM

 Parameters
     uint8_t : the priority of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, sets up the initial transition and does any
     other required initialization for this state machine
 Notes

 Author
     J. Edward Carryer, 10/23/11, 18:55
 ****************************************************************************/
bool InitRocketLaunchGameFSM(uint8_t Priority) {
  ES_Event_t ThisEvent;

  MyPriority = Priority;
  // put us into the Initial PseudoState
  CurrentState = InitPState;

  // initialize event checkers
  InitPCSensorStatus(); // Poker Chip Detection Event Checker
  DB_printf("\nInitializing RocketLaunchGameFSM ");
  // post the initial transition event
  ThisEvent.EventType = ES_INIT;
  if (ES_PostToService(MyPriority, ThisEvent) == true) {
    return true;
  } else {
    return false;
  }
}

/****************************************************************************
 Function
     PostRocketLaunchGameFSM

 Parameters
     EF_Event_t ThisEvent , the event to post to the queue

 Returns
     boolean False if the Enqueue operation failed, True otherwise

 Description
     Posts an event to this state machine's queue
 Notes

 Author
     J. Edward Carryer, 10/23/11, 19:25
 ****************************************************************************/
bool PostRocketLaunchGameFSM(ES_Event_t ThisEvent) {
  return ES_PostToService(MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunRocketLaunchGameFSM

 Parameters
   ES_Event_t : the event to process

 Returns
   ES_Event_t, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   add your description here
 Notes
   uses nested switch/case to implement the machine.
 Author
   J. Edward Carryer, 01/15/12, 15:23
 ****************************************************************************/
ES_Event_t RunRocketLaunchGameFSM(ES_Event_t ThisEvent) {
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors

  switch (CurrentState) {
    case Initializing: // If current state is initial Pseudo State
    {
      if (ThisEvent.EventType == ES_INIT) {
        CurrentState = Welcoming;
        DM_ClearDisplayBuffer();
        pMessage = "Welcome! Please Insert 2 Poker Chips to Begin.";

        ES_Timer_InitTimer(SCROLL_MESSAGE_TIMER, SCROLL_DURATION);
      }
    }
      break;

    case Welcoming:
    {
      switch (ThisEvent.EventType) {
        case ES_TIMEOUT:
        {
          ES_Event_t CharEvent;
          CharEvent.EventType = ES_NEW_CHAR;
          CharEvent.EventParam = *pMessage;
          PostLEDFSM(CharEvent);
          pMessage++;
          if (*pMessage != '\0') {
            ES_Timer_InitTimer(SCROLL_MESSAGE_TIMER, SCROLL_DURATION);
          }
        }
          break;

        case ES_PC_INSERTED: //If poker chip is inserted
        {
          CurrentState = _1CoinInserted;
          DM_ClearDisplayBuffer();
          pMessage = "Chips Inserted: 1";
          ES_Timer_InitTimer(SCROLL_MESSAGE_TIMER, SCROLL_DURATION);

          DB_printf("Poker Chip 1 Detected"); // Print detection status for debugging
        }
          break;

        default:
          ;
      }
    }
      break;

    case _1CoinInserted:
    {
      switch (ThisEvent.EventType) {
        case ES_TIMEOUT:
        {
          ES_Event_t CharEvent;
          CharEvent.EventType = ES_NEW_CHAR;
          CharEvent.EventParam = *pMessage;
          PostLEDFSM(CharEvent);
          pMessage++;
          if (*pMessage != '\0') {
            ES_Timer_InitTimer(SCROLL_MESSAGE_TIMER, SCROLL_DURATION);
          }
        }
          break;

        case ES_PC_INSERTED: //If poker chip is inserted
        {
          CurrentState = _2CoinsInserted;
          DM_ClearDisplayBuffer();
          pMessage = "Chips Inserted: 2";

          ES_Event_t NextEvent;
          NextEvent.EventType = ES_PROMPT_TO_PLAY;
          PostRocketLaunchGameFSM(NextEvent);
          ES_Timer_InitTimer(SCROLL_MESSAGE_TIMER, SCROLL_DURATION);

          DB_printf("Poker Chip 2 Detected"); // Print detection status for debugging
        }
          break;

        default:
          ;
      }
    }
      break;

    case _2CoinsInserted:
    {
      switch (ThisEvent.EventType) {
        case ES_TIMEOUT:
        {
          ES_Event_t CharEvent;
          CharEvent.EventType = ES_NEW_CHAR;
          CharEvent.EventParam = *pMessage;
          PostLEDFSM(CharEvent);
          pMessage++;
          if (*pMessage != '\0') {
            ES_Timer_InitTimer(SCROLL_MESSAGE_TIMER, SCROLL_DURATION);
          }
        }
          break;
        case ES_PROMPT_TO_PLAY:
        {

        }
          break;

        default:
          ;
      }
    }
      break;


    default:
      ;
  } // end switch on Current State
  return ReturnEvent;
}

/****************************************************************************
 Function
     QueryRocketLaunchGameSM

 Parameters
     None

 Returns
     TemplateState_t The current state of the Template state machine

 Description
     returns the current state of the Template state machine
 Notes

 Author
     J. Edward Carryer, 10/23/11, 19:21
 ****************************************************************************/
RocketLaunchGameState_t QueryRocketLaunchGameSM(void) {
  return CurrentState;
}

/***************************************************************************
 private functions
 ***************************************************************************/

