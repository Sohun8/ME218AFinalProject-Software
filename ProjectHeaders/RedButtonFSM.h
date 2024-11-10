/****************************************************************************

  Header file for template Flat Sate Machine
  based on the Gen2 Events and Services Framework

 ****************************************************************************/

#ifndef RedButtonFSM_H
#define RedButtonFSM_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */

// typedefs for the states
// State definitions for use with the query function
typedef enum
{
  InitRedButton, RedWaiting, RedDebouncingFall, RedDebouncingRise
}RedButtonState_t;

// Public Function Prototypes

bool InitRedButtonFSM(uint8_t Priority);
bool PostRedButtonFSM(ES_Event_t ThisEvent);
ES_Event_t RunRedButtonFSM(ES_Event_t ThisEvent);
RedButtonState_t QueryRedButtonSM(void);

// Event Checker Functions
void InitRedButtonState(void);
bool CheckRedButton(void);

#endif /* RedButtonFSM_H */


