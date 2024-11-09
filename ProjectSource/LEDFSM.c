/****************************************************************************
 Module
   LEDFSM.c

 Revision
   1.0.1

 Description
   This implements the LED Display Character Entry flat state machine under the
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
#include "LEDFSM.h"
#include "PIC32_SPI_HAL.h"
#include "DM_Display.h"
#include <xc.h>
#include "ES_DeferRecall.h"

/*----------------------------- Module Defines ----------------------------*/

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine.They should be functions
   relevant to the behavior of this state machine
*/

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well.
// type of state variable should match htat of enum in header file
static LEDState_t CurrentState;

// with the introduction of Gen2, we need a module level Priority var as well
static uint8_t MyPriority;
// add a deferral queue for up to 3 pending deferrals +1 to allow for overhead
static ES_Event_t DeferralQueue[3 + 1];

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitTemplateFSM

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, sets up the initial transition and does any
     other required initialization for this state machine
 Notes

 Author
     J. Edward Carryer, 10/23/11, 18:55
****************************************************************************/
bool InitLEDFSM(uint8_t Priority)
{
  ES_Event_t ThisEvent;

  MyPriority = Priority;
  // put us into the Initial PseudoState
  CurrentState = InitPState;
  
  // SPI Initialization
  SPISetup_BasicConfig(SPI_SPI1);
  SPISetup_SetLeader(SPI_SPI1, SPI_SMP_MID);
  SPISetup_SetBitTime(SPI_SPI1, 10000);
  SPISetup_MapSSOutput(SPI_SPI1, SPI_RPA0);
  SPISetup_MapSDOutput(SPI_SPI1, SPI_RPA1);
  SPISetup_SetClockIdleState(SPI_SPI1, SPI_CLK_LO);
  SPISetup_SetActiveEdge(SPI_SPI1, SPI_FIRST_EDGE);
  SPISetup_SetXferWidth(SPI_SPI1, SPI_16BIT);
  SPISetEnhancedBuffer(SPI_SPI1, true);
  SPISetup_EnableSPI(SPI_SPI1);

  while (!DM_TakeInitDisplayStep()); // Initialize Display
  
  // initialize deferral queue for ES_NEW_CHAR events
  ES_InitDeferralQueueWith(DeferralQueue, ARRAY_SIZE(DeferralQueue));
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

/****************************************************************************
 Function
     PostTemplateFSM

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
bool PostLEDFSM(ES_Event_t ThisEvent)
{
  return ES_PostToService(MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunTemplateFSM

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
ES_Event_t RunLEDFSM(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors

  switch (CurrentState)
  {
    case InitPState:        // If current state is initial Psedudo State
    {
      if (ThisEvent.EventType == ES_INIT)    // only respond to ES_Init
      {
        CurrentState = Waiting;
      }
    }
    break;

    case Waiting:        // If current state is state one
    {
        if (ThisEvent.EventType == ES_NEW_CHAR) // only respond to ES_NEW_CHAR
        {
            unsigned char entry = ThisEvent.EventParam; // retrieve entered char
            DM_ScrollDisplayBuffer(4); // Scroll buffer by 4 columns
            DM_AddChar2DisplayBuffer(entry); // Add character to buffer
            CurrentState = Updating;
            ES_Event_t NextEvent;
            NextEvent.EventType = ES_KEEP_UPDATING;
            PostLEDFSM(NextEvent);
        }
    }
    break;
    
    case Updating:
    {
        switch (ThisEvent.EventType)
        {
            // if update is complete
            case ES_UPDATE_COMPLETE:
            {
                CurrentState = Waiting; // go to waiting state
                // recall any deferred character events
                ES_RecallEvents(MyPriority, DeferralQueue); 
            }
            break;
            
            // if still updating
            case ES_KEEP_UPDATING:
            {
                ES_Event_t NextEvent;
                // take update step and if finished updating, post event
                if (DM_TakeDisplayUpdateStep() == true){
                    NextEvent.EventType = ES_UPDATE_COMPLETE;
                    PostLEDFSM(NextEvent);
                }
                // else post event to keep updating
                else{
                    NextEvent.EventType = ES_KEEP_UPDATING;
                    PostLEDFSM(NextEvent);                    
                }
            }
            break;
            
            // if additional character to display is sent
            case ES_NEW_CHAR:
            {
                // defer the event and return an error if queue is full
                if (!ES_DeferEvent(DeferralQueue, ThisEvent)){
                    ReturnEvent.EventType = ES_ERROR;
                    ReturnEvent.EventParam = MyPriority;
                }
            }
            break;
            
            default:
                break;         
        }
    }
    break;
    // repeat state pattern as required for other states
    default:
      ;
  }                                   // end switch on Current State
  return ReturnEvent;
}

/****************************************************************************
 Function
     QueryTemplateSM

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
LEDState_t QueryLEDFSM(void)
{
  return CurrentState;
}

/***************************************************************************
 private functions
 ***************************************************************************/

