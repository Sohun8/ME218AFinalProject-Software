/****************************************************************************
 Module
   TimerServoFSM.c

 Description
   This is a state machine that runs the game timer and displays the time using a servo

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 01/15/12 11:12 jec      revisions for Gen2 framework
 11/07/11 11:26 jec      made the queue static
 10/30/11 17:59 jec      fixed references to CurrentEvent in RunTemplateSM()
 10/23/11 18:20 jec      began conversion from SMTemplate.c (02/20/07 rev)
 2024-11-15     jlp      used templateFSM to make TimerServoFSM
 ****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
 */
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "TimerServoFSM.h"
#include "dbprintf.h"
#include "PWM_PIC32.h"
#include <stdint.h>
#include "PIC32PortHAL.h"
#include "RocketLaunchGameFSM.h"

/*----------------------------- Module Defines ----------------------------*/
static const int16_t TimerStartServoVal = 2.4 * TICS_PER_MS;
static const int16_t TimerStopServoVal = 0.56 * TICS_PER_MS;

static const uint8_t Timer_Servo_PWM_Channel = 5;
static const PWM_PinMap_t Timer_Servo_Pin = PWM_RPA4;
// no ansel on RPA4

static WhichTimer_t Servos_Timer_For_PWM = _Timer2_;

static uint8_t timerVal;

static const uint8_t maxTime = 50;

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine.They should be functions
   relevant to the behavior of this state machine
 */
void SetupTimerServo();
void SetServoTime(uint8_t time);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well.
// type of state variable should match htat of enum in header file
static TimerServoState_t CurrentState;

// with the introduction of Gen2, we need a module level Priority var as well
static uint8_t MyPriority;

/*------------------------------ Module Code ------------------------------*/

/****************************************************************************
 Function
     InitTimerServoFSM
 ****************************************************************************/
bool InitTimerServoFSM(uint8_t Priority) {
  ES_Event_t ThisEvent;

  MyPriority = Priority;
  // put us into the Initial PseudoState
  CurrentState = TS_InitPState;
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
     PostTimerServoFSM
 ****************************************************************************/
bool PostTimerServoFSM(ES_Event_t ThisEvent) {
  return ES_PostToService(MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunTimerServoFSM
 ****************************************************************************/
ES_Event_t RunTimerServoFSM(ES_Event_t ThisEvent) {
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors

  if (ThisEvent.EventType == ES_NEW_KEY) {
    if (ThisEvent.EventParam == '=') {
      ES_Event_t newEvent = {ES_START_GAME_TIMER, 0};
      PostTimerServoFSM(newEvent);
    }
    if (ThisEvent.EventParam == '-') {
      ES_Event_t newEvent = {ES_RESET_GAME_TIMER, 0};
      PostTimerServoFSM(newEvent);
    }
  }

  switch (CurrentState) {
    case InitPState: // If current state is initial Psedudo State
    {
      if (ThisEvent.EventType == ES_INIT) {
        // now put the machine into the actual initial state
        CurrentState = TS_Waiting;
        SetupTimerServo();
        timerVal = 0;
        SetServoTime(0);
      }
    }
      break;

    case TS_Waiting: // If current state is state one
    {
      switch (ThisEvent.EventType) {
        case ES_START_GAME_TIMER:
        {
          ES_Timer_InitTimer(TIMER_SERVO_TIMER, 1000);
          timerVal = 0;
          SetServoTime(0);
          CurrentState = TS_Timing;
        }
          break;
      }
    }
      break;

    case TS_Timing:
    {
      switch (ThisEvent.EventType) {
        case ES_TIMEOUT:
        {
          if (ThisEvent.EventParam == TIMER_SERVO_TIMER) {
            timerVal++;
            SetServoTime(timerVal);
            if (timerVal >= maxTime) {
              CurrentState = TS_Waiting;

              DB_printf("GAME OVER!!!\n");

              ES_Event_t NewEvent;
              NewEvent.EventType = ES_GAME_OVER;
              PostRocketLaunchGameFSM(NewEvent);

            } else {
              ES_Timer_InitTimer(TIMER_SERVO_TIMER, 1000);
            }
          }
        }
          break;

        case ES_RESET_GAME_TIMER:
        {
          ES_Timer_StopTimer(TIMER_SERVO_TIMER);
          timerVal = 0;
          CurrentState = TS_Waiting;
        }
          break;

      }
    }
      break;

  } // end switch on Current State
  return ReturnEvent;
}

/****************************************************************************
 Function
     QueryTimerServoFSM
 ****************************************************************************/
TimerServoState_t QueryTimerServoFSM(void) {
  return CurrentState;
}

/***************************************************************************
 private functions
 ***************************************************************************/
void SetupTimerServo() {
  PWMSetup_AssignChannelToTimer(Timer_Servo_PWM_Channel, Servos_Timer_For_PWM);
  PWMSetup_MapChannelToOutputPin(Timer_Servo_PWM_Channel, Timer_Servo_Pin);
}

void SetServoTime(uint8_t time) {
  int32_t servoVal = time * (TimerStopServoVal - TimerStartServoVal) / maxTime + TimerStartServoVal;
  //  DB_printf("set timer servo val %d\n", servoVal);
  PWMOperate_SetPulseWidthOnChannel(servoVal, Timer_Servo_PWM_Channel);
}
