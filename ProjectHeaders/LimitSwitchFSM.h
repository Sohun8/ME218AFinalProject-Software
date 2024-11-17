/****************************************************************************

  Header file for template Flat Sate Machine
  based on the Gen2 Events and Services Framework

 ****************************************************************************/

#ifndef LimitSwitchFSM_H
#define LimitSwitchFSM_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */

// typedefs for the states
// State definitions for use with the query function
typedef enum
{
  InitLimitSwitch, LimitSwitchWaiting, LimitSwitchDebouncingFall, LimitSwitchDebouncingRise
}LimitSwitchState_t;

// Public Function Prototypes

bool InitLimitSwitchFSM(uint8_t Priority);
bool PostLimitSwitchFSM(ES_Event_t ThisEvent);
ES_Event_t RunLimitSwitchFSM(ES_Event_t ThisEvent);
LimitSwitchState_t QueryLimitSwitchSM(void);

// Event Checker Functions
void InitLimitSwitchState(void);
bool CheckLimitSwitch(void);

#endif /* LimitSwitchFSM_H */


