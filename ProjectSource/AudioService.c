/****************************************************************************
 Module
   AudioService.c

 ****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
 */
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "ES_DeferRecall.h"
#include "AudioService.h"
#include "dbprintf.h"

/*----------------------------- Module Defines ----------------------------*/
#define TEST_AUDIO_SERVICE
/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
 */

void SetPins(int8_t value);

/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;

static ES_Event_t DeferralQueue[10 + 1];

static bool SignalingInProgressState;
static bool SignalingAfterDelayState;

/*------------------------------ Module Code ------------------------------*/

/****************************************************************************
 Function
     InitAudioService

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, and does any
     other required initialization for this service
 Notes

 Author
     J. Edward Carryer, 01/16/12, 10:00
 ****************************************************************************/
bool InitAudioService(uint8_t Priority) {
  ES_Event_t ThisEvent;

  MyPriority = Priority;
  /********************************************
   in here you write your initialization code
   *******************************************/
  ES_InitDeferralQueueWith(DeferralQueue, ARRAY_SIZE(DeferralQueue));

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
     PostAudioService

 Parameters
     EF_Event_t ThisEvent ,the event to post to the queue

 Returns
     bool false if the Enqueue operation failed, true otherwise

 Description
     Posts an event to this state machine's queue
 Notes

 Author
     J. Edward Carryer, 10/23/11, 19:25
 ****************************************************************************/
bool PostAudioService(ES_Event_t ThisEvent) {
  return ES_PostToService(MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunTemplateService

 Parameters
   ES_Event_t : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   add your description here
 Notes

 Author
   J. Edward Carryer, 01/15/12, 15:23
 ****************************************************************************/
ES_Event_t RunAudioService(ES_Event_t ThisEvent) {
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
  /********************************************
   in here you write your service code
   *******************************************/
  switch (ThisEvent.EventType) {
    case ES_INIT:
    {
      ANSELBCLR = BIT8HI | BIT15HI;
      TRISBCLR = BIT8HI | BIT15HI; //output
      SignalingInProgressState = false;
      SignalingAfterDelayState = false;
    }
      break;
#ifdef TEST_AUDIO_SERVICE
    case ES_NEW_KEY:
    {
      if (ThisEvent.EventParam == '4' || ThisEvent.EventParam == '5' || ThisEvent.EventParam == '6') {
        ES_Event_t NewEvent;
        NewEvent.EventType = ES_AUDIO_PLAY;
        if (ThisEvent.EventParam == '4') {
          NewEvent.EventParam = AUDIO_PLAY_MUSIC;
        }
        if (ThisEvent.EventParam == '5') {
          NewEvent.EventParam = AUDIO_PLAY_CORRECT;
        }
        if (ThisEvent.EventParam == '6') {
          NewEvent.EventParam = AUDIO_PLAY_WRONG;
        }
        PostAudioService(NewEvent);
      }
    }
      break;
#endif
    case ES_TIMEOUT:
    {
      if (ThisEvent.EventParam == AUDIO_SERVICE_TIMER) {
        if(SignalingAfterDelayState){
            SignalingAfterDelayState=false;
            ES_RecallEvents(MyPriority, DeferralQueue);
        }else{
            SetPins(0);
            SignalingInProgressState = false;
            SignalingAfterDelayState = true;
            ES_Timer_InitTimer(AUDIO_SERVICE_TIMER, 50);
        }
      }
    }
      break;
    case ES_AUDIO_PLAY:
    {
      if (ThisEvent.EventParam == AUDIO_PLAY_MUSIC || ThisEvent.EventParam == AUDIO_PLAY_WRONG || ThisEvent.EventParam == AUDIO_PLAY_CORRECT) {
        if (SignalingInProgressState||SignalingAfterDelayState) {
          if (!ES_DeferEvent(DeferralQueue, ThisEvent)) {
            ReturnEvent.EventType = ES_ERROR;
            ReturnEvent.EventParam = MyPriority;
          }
        } else {
          SetPins(ThisEvent.EventParam);
          SignalingInProgressState = true;
          ES_Timer_InitTimer(AUDIO_SERVICE_TIMER, 50);
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

void SetPins(int8_t value) {
  LATBbits.LATB8 = value & 0b1;
  LATBbits.LATB15 = (value & 0b10) >> 1;
}

/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/

