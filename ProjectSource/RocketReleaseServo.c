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
#include "RocketReleaseServo.h"
#include "PWM_PIC32.h"
#include "dbprintf.h"
#include "PIC32PortHAL.h"
#include <stdint.h>

#define TEST // uncomment to test servo (press 1 and 2 on the keyboard to move the servo)

/*----------------------------- Module Defines ----------------------------*/
const int16_t RocketLockServoVal = 0.6 * TICS_PER_MS;
const int16_t RocketLaunchServoVal = 2.2 * TICS_PER_MS;

const uint8_t Num_PWM_Channels_To_Configure = 5;

const uint8_t Rocket_Launch_Servo_PWM_Channel = 1;
const PWM_PinMap_t Rocket_Launch_Servo_Pin = PWM_RPB3;
#define ROCKET_LAUNCH_SERVO_PIN_ANSELBIT ANSELBbits.ANSB3

WhichTimer_t Servos_Timer_For_PWM = _Timer2_;

const uint16_t Servos_Timer_Frequency = 50;

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
 */
void SetServoLaunched();
void SetServoLocked();
void SetupServo();

/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;

/*------------------------------ Module Code ------------------------------*/

bool InitRocketReleaseServo(uint8_t Priority) {
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

bool PostRocketReleaseServo(ES_Event_t ThisEvent) {
  return ES_PostToService(MyPriority, ThisEvent);
}

ES_Event_t RunRocketReleaseServo(ES_Event_t ThisEvent) {
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
  switch (ThisEvent.EventType) {
    case ES_INIT:
    {
      SetupServo();
    }
      break;
    case ES_ROCKET_RELEASE_SERVO_LAUNCH:
    {
      SetServoLaunched();
    }
      break;
    case ES_ROCKET_RELEASE_SERVO_LOCK:
    {
      SetServoLocked();
    }
      break;
#ifdef TEST
    case ES_NEW_KEY: // FOR TESTING press 1 and 2
    {
      ES_Event_t NewEvent;
      if (ThisEvent.EventParam == '2') {
        NewEvent.EventType = ES_ROCKET_RELEASE_SERVO_LAUNCH;
        PostRocketReleaseServo(NewEvent);
      }
      if (ThisEvent.EventParam == '1') {
        NewEvent.EventType = ES_ROCKET_RELEASE_SERVO_LOCK;
        PostRocketReleaseServo(NewEvent);
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
void SetServoLaunched() {
  PWMOperate_SetPulseWidthOnChannel(RocketLaunchServoVal, Rocket_Launch_Servo_PWM_Channel);
}

void SetServoLocked() {
  PWMOperate_SetPulseWidthOnChannel(RocketLockServoVal, Rocket_Launch_Servo_PWM_Channel);
}

void SetupServo() {
  ROCKET_LAUNCH_SERVO_PIN_ANSELBIT = 0;
  PWMSetup_AssignChannelToTimer(Rocket_Launch_Servo_PWM_Channel, Servos_Timer_For_PWM);
  PWMSetup_MapChannelToOutputPin(Rocket_Launch_Servo_PWM_Channel, Rocket_Launch_Servo_Pin);
  PWMOperate_SetPulseWidthOnChannel(RocketLaunchServoVal, Rocket_Launch_Servo_PWM_Channel);
}

/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/
