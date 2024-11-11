/****************************************************************************

  Header file for RocketHeightServos
  based on the Gen 2 Events and Services Framework
 * 
 * This module controls a servo to lock and release the rocket
 * 
 * Edit ServoPulseLengths in RocketHeightServos.c to change the range of the servos' movement
 * 
 * Events this service responds to:
 **** ES_ROCKET_HEIGHT
 **** ES_INIT (published to itself during initialization)
 * Events this service posts:
 **** None
 ****************************************************************************/

#ifndef ROCKET_HEIGHT_SERVOS_H
#define ROCKET_HEIGHT_SERVOS_H

#include "ES_Types.h"

// Public Function Prototypes
/**
 * This service will start by setting up pwm for the servos and moving the servoS to the open angles
 * @param Priority
 * @return 
 */
bool InitRocketHeightServos(uint8_t Priority);
/**
 * The RocketReleaseServo service responds to 1 Event.
 * ES_ROCKET_HEIGHT will OPEN or BLOCK servos based on the parameter. 0 = all blocked, 1 = first open, 2 = bottom 2 open, 3 = all open
 * @param ThisEvent
 * @return 
 */
bool PostRocketHeightServos(ES_Event_t ThisEvent);
ES_Event_t RunRocketHeightServos(ES_Event_t ThisEvent);

#endif // ROCKET_HEIGHT_SERVOS_H

