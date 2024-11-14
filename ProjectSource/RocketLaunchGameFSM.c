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
#include "IRLaunchEventChecker.h"
#include "terminal.h"
#include "dbprintf.h"
#include "PIC32_AD_Lib.h"
#include "PIC32PortHAL.h"
#include "LEDDisplayService.h"

/*----------------------------- Module Defines ----------------------------*/
#define TESTGAME // uncomment to remove testing with keyboard events
#define SCROLL_DURATION 100 // milliseconds
#define POT_PIN_PORT _Port_B
#define POT_PIN_NUM _Pin_2
#define POT_PIN_BITS BIT4HI
#define POT_PIN_READ PORTBbits.RB2;


/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine.They should be functions
   relevant to the behavior of this state machine
 */
void SendMessage(LED_ID_t whichMsg, LED_Instructions_t whichInst);
void readPot(void);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well.
// type of state variable should match htat of enum in header file
static RocketLaunchGameState_t CurrentState;
static uint8_t gameDifficulty = 0;
static char* playerEntry;
static char* currentSequence;
static uint8_t roundNumber;
char customBuffer[100];

const uint8_t NUM_SPACES[5] = {
    4,
    3,
    2,
    1,
    0
};

const uint8_t NUM_LETTERS_IN_SEQUENCE[5] = {
    4,
    5,
    6,
    7,
    8
};

// with the introduction of Gen2, we need a module level Priority var as well
static uint8_t MyPriority;

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
  InitIRLaunchSensorStatus(); // IR Launch Sensor Event Checker
  //PortSetup_ConfigureAnalogInputs(POT_PIN_PORT, POT_PIN_NUM);
  ADC_ConfigAutoScan(POT_PIN_BITS);
  DB_printf("\nInitializing RocketLaunchGameFSM");
  DB_printf("\nkeyboard events for testing: ");
  DB_printf("\n 0,1,2,3=rocket height");
  DB_printf("\n 8=lock rocket 9=launch rocket");
  DB_printf("\n p=coin r=red button s =limit switch\n\n");
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
        static uint8_t roundNumber;
        // Send Scrolling Welcome Message to display
        SendMessage(MSG_STARTUP, SCROLL_REPEAT);
      }
    }
      break;

    case Welcoming:
    {
      switch (ThisEvent.EventType) {
        case ES_PC_INSERTED: //If poker chip is inserted
        {
          CurrentState = _1CoinInserted;
          SendMessage(MSG_CHIPCOUNT1, DISPLAY_HOLD);
          DB_printf("Poker Chip 1 Detected"); // Print detection status for debugging
        }
          break;

#ifdef TESTGAME
        case ES_NEW_KEY:
        {
          if (ThisEvent.EventParam == 'p') {
            ES_Event_t NewEvent;
            NewEvent.EventType = ES_PC_INSERTED;
            PostRocketLaunchGameFSM(NewEvent);
          }
        }
          break;
#endif /* TESTGAME */
      }
    }
      break;

    case _1CoinInserted:
    {
      switch (ThisEvent.EventType) {
        case ES_PC_INSERTED: //If poker chip is inserted
        {
          CurrentState = PromptingToPlay;
          SendMessage(MSG_PROMPT2PLAY, SCROLL_REPEAT_SLOW);
          DB_printf("Poker Chip 2 Detected"); // Print detection status for debugging
        }
          break;

#ifdef TESTGAME
        case ES_NEW_KEY:
        {
          if (ThisEvent.EventParam == 'p') {
            ES_Event_t NewEvent;
            NewEvent.EventType = ES_PC_INSERTED;
            PostRocketLaunchGameFSM(NewEvent);
          }
        }
          break;
#endif /* TESTGAME */
      }
    }
      break;
/*
    case _2CoinsInserted:
    {
      switch (ThisEvent.EventType) {
        case ES_FINISHED_SCROLLING:
        {
          // hold coin status for 1 second
          ES_Timer_InitTimer(HOLD_MESSAGE_TIMER, 1000);
        }
        break;

        case ES_TIMEOUT:
        {
          if (ThisEvent.EventParam == HOLD_MESSAGE_TIMER){
            CurrentState = PromptingToPlay;
            SendMessage(MSG_PROMPT2PLAY, SCROLL_REPEAT);
          }
        }
        break;
      }
    }
      break;
*/
    case PromptingToPlay:
    {
      switch (ThisEvent.EventType) {
        case ES_LIMIT_SWITCH:
        {
            CurrentState = DisplayingInstructions;
            SendMessage(MSG_INSTRUCTIONS, SCROLL_ONCE);
            /* TO DO */
            // POST TO SERVO ONE
            /******************/
            
        }
          break;

#ifdef TESTGAME
        case ES_NEW_KEY:
        {
          if (ThisEvent.EventParam == 's') {
            ES_Event_t NewEvent;
            NewEvent.EventType = ES_LIMIT_SWITCH;
            PostRocketLaunchGameFSM(NewEvent);
          }
        }
          break;
#endif /* TESTGAME */        
      }
    }
      break;

    case DisplayingInstructions:
    {
      switch (ThisEvent.EventType) {
        case ES_FINISHED_SCROLLING:
        {
            // hold end of message for one second
            ES_Timer_InitTimer(HOLD_MESSAGE_TIMER, 1000);
            DB_printf("\n In Finished scrolling \n");
        }
        break;
        case ES_TIMEOUT:
        {
          if (ThisEvent.EventParam == HOLD_MESSAGE_TIMER){
            CurrentState = ChoosingDifficulty;
            SendMessage(MSG_CHOOSE_DIFF, SCROLL_REPEAT);
            // Set time to choose difficulty
            ES_Timer_InitTimer(HOLD_MESSAGE_TIMER, 5000);
          }
        }
        break;
      }
    }
      break;

    case ChoosingDifficulty:
    {
        switch (ThisEvent.EventType) {
            case ES_TIMEOUT:
            {
               if (ThisEvent.EventParam == HOLD_MESSAGE_TIMER){
                   readPot();
                   CurrentState = RoundInit;
                   DB_printf("\n Game Difficulty: %d\n", gameDifficulty);
                   roundNumber++;
                   sprintf(customBuffer, "Round %d", roundNumber);
                   currentMessage = customBuffer;
                   SendMessage(MSG_CUSTOM, DISPLAY_HOLD);
                   
                   ES_Timer_InitTimer(HOLD_MESSAGE_TIMER, 2000);
                   CurrentState = RoundInit;
               } 
            }
            break;
        }
    }
    break;
 
    case RoundInit:
    {
        switch (ThisEvent.EventType) {
            case ES_TIMEOUT:
            {
               if (ThisEvent.EventParam == HOLD_MESSAGE_TIMER){
                   currentMessage = " ";
                   SendMessage(MSG_CUSTOM, DISPLAY_HOLD);
               } 
            }
            break;
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

// sends ES_NEW_MESSAGE event to LEDDisplayService depending on given message id and display instructions
// see LEDDisplayService.h for type definitions 
void SendMessage(LED_ID_t whichMsg, LED_Instructions_t whichInst){
  ES_Event_t MessageEvent;
  MessageEvent.EventType = ES_NEW_MESSAGE;

  paramUnion msgParams;
  msgParams.msgID = whichMsg;
  msgParams.dispInstructions = whichInst;

  MessageEvent.EventParam = msgParams.fullParam;
  PostLEDDisplayService(MessageEvent);
}

void readPot(void) {
    uint32_t adcResults[1] = {0};
    ADC_MultiRead(adcResults);
    gameDifficulty = (adcResults[0] * 5) / 1000;
    DB_printf("\nAnalog Val: %d:    ", adcResults[0]);
    DB_printf("Difficulty: %d\n",gameDifficulty);
}
