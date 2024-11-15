/****************************************************************************
 Module
   RocketReleaseServo.c

 ****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
 */
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "RocketHeightServos.h"
#include "PWM_PIC32.h"
#include "dbprintf.h"
#include "PIC32PortHAL.h"
#include <stdint.h>

#define TEST_ROCKET_HEIGHT_SERVOS // uncomment to test servos (press 0,1,2,3 on the keyboard to set height)

/*----------------------------- Module Defines ----------------------------*/
static const uint8_t NumServos = 3;

static const uint8_t OPEN = 0;
static const uint8_t BLOCKED = 1;

// OPEN, BLOCKED
static const int16_t ServoPulseLengths[][2] = {
  {0.6 * TICS_PER_MS, 2.2 * TICS_PER_MS},
  {0.6 * TICS_PER_MS, 2.2 * TICS_PER_MS},
  {0.6 * TICS_PER_MS, 2.2 * TICS_PER_MS}
};

static const uint8_t ServoPwmChannels[] = {2, 3, 4};

// these pins don't have analog so don't worry about ANSEL
static const PWM_PinMap_t ServoPins[] = {PWM_RPB5, PWM_RPA3, PWM_RPA2};

static WhichTimer_t Servos_Timer_For_PWM = _Timer2_;


/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
 */
void SetServos(uint8_t height);
void SetupServos();

/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;

/*------------------------------ Module Code ------------------------------*/

bool InitRocketHeightServos(uint8_t Priority) {
  ES_Event_t ThisEvent;

  MyPriority = Priority;
  // post the initial transition event
  ThisEvent.EventType = ES_INIT;
  if (ES_PostToService(MyPriority, ThisEvent) == true) {
    return true;
  } else {
    return false;
  }
}

bool PostRocketHeightServos(ES_Event_t ThisEvent) {
  return ES_PostToService(MyPriority, ThisEvent);
}

ES_Event_t RunRocketHeightServos(ES_Event_t ThisEvent) {
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
  switch (ThisEvent.EventType) {
    case ES_INIT:
    {
      SetupServos();
    }
      break;
    case ES_ROCKET_SERVO_HEIGHT:
    {
      if (ThisEvent.EventParam <= NumServos) {
        SetServos(ThisEvent.EventParam);
      }
    }
      break;
#ifdef TEST_ROCKET_HEIGHT_SERVOS
    case ES_NEW_KEY: // FOR TESTING press 1 and 2
    {
      ES_Event_t NewEvent;
      NewEvent.EventType = ES_ROCKET_SERVO_HEIGHT;
      if (ThisEvent.EventParam == '0') {
        NewEvent.EventParam = 0;
        PostRocketHeightServos(NewEvent);
      }
      if (ThisEvent.EventParam == '1') {
        NewEvent.EventParam = 1;
        PostRocketHeightServos(NewEvent);
      }
      if (ThisEvent.EventParam == '2') {
        NewEvent.EventParam = 2;
        PostRocketHeightServos(NewEvent);
      }
      if (ThisEvent.EventParam == '3') {
        NewEvent.EventParam = 3;
        PostRocketHeightServos(NewEvent);
      }
    }
      break;
#endif
  }
  return ReturnEvent;
}

/***************************************************************************
 private functions
 ***************************************************************************/
void SetServos(uint8_t height) {
  for (uint8_t servoNum = 0; servoNum < NumServos; servoNum++) {
    uint8_t openOrBlocked = BLOCKED;
    if (height > servoNum) {
      openOrBlocked = OPEN;
    }
    PWMOperate_SetPulseWidthOnChannel(ServoPulseLengths[servoNum][openOrBlocked], ServoPwmChannels[servoNum]);
  }
}

void SetupServos() {
  for (uint8_t servoNum = 0; servoNum < NumServos; servoNum++) {
    PWMSetup_AssignChannelToTimer(ServoPwmChannels[servoNum], Servos_Timer_For_PWM);
    PWMSetup_MapChannelToOutputPin(ServoPwmChannels[servoNum], ServoPins[servoNum]);
    PWMOperate_SetPulseWidthOnChannel(ServoPulseLengths[servoNum][OPEN], ServoPwmChannels[servoNum]);
  }
}

/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/
