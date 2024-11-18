/****************************************************************************
 Module
   LEDDisplayService.c

 ****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
 */
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "LEDDisplayService.h"
#include "dbprintf.h"
#include "PIC32PortHAL.h"
#include "DM_Display.h"
#include <stdint.h>


/*----------------------------- Module Defines ----------------------------*/
#define SCROLL_DURATION 100 // milliseconds
#define SCROLL_DURATION_SLOW 200

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
 */
void ScrollMessage(void);

/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;
const uint8_t numSpaces[5] = {
    4,
    3,
    2,
    1,
    0
};
/*************************************************************************** */
/**** PUT NEW PRESET MESSAGES HERE AND UPDATE THE LED_ID_t ENUM TO MATCH INDEX *****/
char* MESSAGES[] = {
    "Welcome! Please Insert 2 Poker Chips to Begin.    ",
    "1 Chip Inserted",
    "Lower Rocket to Play.    ",
    "Repeat sequence by pressing RGB buttons ",
    "Set Difficulty",
    "NO USER INPUT. RESETTING.",
    // Insert new messages here
};
/* REMEMBER TO GO TO HEADER FILE AND UPDATE LED_ID_t */
/************************************************************************** */

char* currentMessage;// = MESSAGES[MSG_STARTUP]; // global variable defined here because it has to be defined somewhere
static char* pMessage; // pointer to message string (iterates)

static LED_Instructions_t currentInstructions;

/*------------------------------ Module Code ------------------------------*/

bool InitLEDDisplayService(uint8_t Priority) {
  ES_Event_t ThisEvent;

  MyPriority = Priority;
  // post the initial transition event
  currentMessage = MESSAGES[MSG_STARTUP];
  ThisEvent.EventType = ES_INIT;
  if (ES_PostToService(MyPriority, ThisEvent) == true) {
    return true;
  } else {
    return false;
  }
}


bool PostLEDDisplayService(ES_Event_t ThisEvent) {
  return ES_PostToService(MyPriority, ThisEvent);
}


ES_Event_t RunLEDDisplayService(ES_Event_t ThisEvent) {
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
  switch (ThisEvent.EventType) {
    case ES_INIT:
    {
    }
    break;

    case ES_CLEAR_MESSAGE:
    {
      DM_ClearDisplayBuffer();
    }
    break;

    case ES_NEW_MESSAGE:
    {
      paramUnion msgParams;
      msgParams.fullParam = ThisEvent.EventParam;
      if (msgParams.msgID != MSG_CUSTOM){
        currentMessage = MESSAGES[msgParams.msgID];
      }
      // else currentMessage is already set to RGB sequence by RocketLaunchGameFSM

      pMessage = currentMessage;

      switch (msgParams.dispInstructions){
        case DISPLAY_HOLD:
        {
          DB_printf("In Display hold\n");
          currentInstructions = DISPLAY_HOLD;
          DM_ClearDisplayBuffer();          
          ES_Event_t NextEvent;
          NextEvent.EventType = ES_KEEP_UPDATING;
          PostLEDDisplayService(NextEvent);
          ES_Timer_StopTimer(SCROLL_MESSAGE_TIMER);
        }
        break;

        case SCROLL_ONCE:
        {
          currentInstructions = SCROLL_ONCE;
          DM_ClearDisplayBuffer();
          ES_Timer_InitTimer(SCROLL_MESSAGE_TIMER, SCROLL_DURATION);
        }
        break;
        
        case SCROLL_ONCE_SLOW:
        {
          currentInstructions = SCROLL_ONCE_SLOW;
          DM_ClearDisplayBuffer();
          ES_Timer_InitTimer(SCROLL_MESSAGE_TIMER, SCROLL_DURATION_SLOW);
        }
        break;

        case SCROLL_REPEAT:
        {
          ES_Timer_InitTimer(SCROLL_MESSAGE_TIMER, 1000);
          currentInstructions = SCROLL_REPEAT;
          DM_ClearDisplayBuffer();
          ES_Timer_InitTimer(SCROLL_MESSAGE_TIMER, SCROLL_DURATION);
        }
        break;
        
        case SCROLL_REPEAT_SLOW:
        {
          ES_Timer_InitTimer(SCROLL_MESSAGE_TIMER, 1000);
          currentInstructions = SCROLL_REPEAT_SLOW;
          DM_ClearDisplayBuffer();
          ES_Timer_InitTimer(SCROLL_MESSAGE_TIMER, SCROLL_DURATION_SLOW);
        }
        break;
      }
    }
    break;

    // This event is for Display holding only
    case ES_KEEP_UPDATING:
    {
      ES_Event_t NextEvent;
      NextEvent.EventType = ES_KEEP_UPDATING;
      // if entire message isn't added to buffer yet
      if (*pMessage != '\0'){
        Add2DisplayBuffer();
        PostLEDDisplayService(NextEvent);
      }
      // else keep updating the display until done 
      else{
        bool done = DM_TakeDisplayUpdateStep();
        if (done == false){
          PostLEDDisplayService(NextEvent);
        }
      }
    }
    break;

    // This  ES_TIMEOUT:event is for scrolling messages only
    case ES_TIMEOUT:
    {
      if (ThisEvent.EventParam == SCROLL_MESSAGE_TIMER) {
        ScrollMessage();
        if (*pMessage == '\0') {
          if (currentInstructions == SCROLL_REPEAT || currentInstructions == SCROLL_REPEAT_SLOW) {
            pMessage = currentMessage; // Reset pointer for repeating
          } 
          else if (currentInstructions == SCROLL_ONCE || currentInstructions == SCROLL_ONCE_SLOW) {
            ES_Timer_StopTimer(SCROLL_MESSAGE_TIMER);
            // Tell PostRocketLaunchGame that scrolling is done
            ES_Event_t Event2Post;
            Event2Post.EventType = ES_FINISHED_SCROLLING;
            PostRocketLaunchGameFSM(Event2Post);
          }
        }
      }
    }
    break;

  }
  return ReturnEvent;
}

/***************************************************************************
 private functions
 ***************************************************************************/
void ScrollMessage(void) {
  ES_Event_t CharEvent;
  CharEvent.EventType = ES_NEW_CHAR;
  CharEvent.EventParam = *pMessage;
  PostLEDFSM(CharEvent);
  pMessage++;
  if (currentInstructions == SCROLL_ONCE || currentInstructions == SCROLL_REPEAT){
    ES_Timer_InitTimer(SCROLL_MESSAGE_TIMER, SCROLL_DURATION);
  }
  else{
    ES_Timer_InitTimer(SCROLL_MESSAGE_TIMER, SCROLL_DURATION_SLOW);
  }
}

void Add2DisplayBuffer(void) {
    DM_ScrollDisplayBuffer(4);
    DM_AddChar2DisplayBuffer(*pMessage);
    pMessage++;
}
/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/
