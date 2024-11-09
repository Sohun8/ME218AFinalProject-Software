/****************************************************************************

  Header file for RocketReleaseServo
  based on the Gen 2 Events and Services Framework
 * 
 * This module controls a servo to lock and release the rocket
 * 
 * Edit RocketLockServoVal and RocketLaunchServoVal in RocketReleaseServo.c to change the range of the servo's movement
 * 
 * Events this service responds to:
 **** ES_ROCKET_RELEASE_SERVO_LAUNCH
 **** ES_ROCKET_RELEASE_SERVO_LOCK
 **** ES_INIT (published to itself during initialization)
 * Events this service posts:
 **** None
 ****************************************************************************/

#ifndef ROCKET_RELEASE_SERVO_H
#define ROCKET_RELEASE_SERVO_H

#include "ES_Types.h"

// Public Function Prototypes
/**
 * This service will start by setting up pwm for the servo and moving the servo to the LAUNCH angle
 * @param Priority
 * @return 
 */
bool InitRocketReleaseServo(uint8_t Priority);
/**
 * The RocketReleaseServo service responds to two Events.
 * ES_ROCKET_RELEASE_SERVO_LAUNCH will move the servo to an angle that releases the rocket and allows the rocket to be pushed back down
 * ES_ROCKET_RELEASE_SERVO_LOCK will move the servo to an angle that holds the rocket but prevents the rocket from being pushed down
 * @param ThisEvent
 * @return 
 */
bool PostRocketReleaseServo(ES_Event_t ThisEvent);
ES_Event_t RunRocketReleaseServo(ES_Event_t ThisEvent);

#endif // ROCKET_RELEASE_SERVO_H

