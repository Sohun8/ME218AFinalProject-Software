/****************************************************************************

  Header file for timer servo SM
  based on the Gen2 Events and Services Framework

 ****************************************************************************/

#ifndef TimerServoFSM_H
#define TimerServoFSM_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */

// typedefs for the states
// State definitions for use with the query function

typedef enum {
    TS_InitPState, TS_Timing, TS_Waiting
} TimerServoState_t;

// Public Function Prototypes

bool InitTimerServoFSM(uint8_t Priority);
bool PostTimerServoFSM(ES_Event_t ThisEvent);
ES_Event_t RunTimerServoFSM(ES_Event_t ThisEvent);
TimerServoState_t QueryTimerServoFSM(void);

#endif /* FSMTemplate_H */

