/****************************************************************************

  Header file for template Flat Sate Machine
  based on the Gen2 Events and Services Framework

 ****************************************************************************/

#ifndef GreenButtonFSM_H
#define GreenButtonFSM_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */

// typedefs for the states
// State definitions for use with the query function
typedef enum
{
  InitGreenButton, GreenWaiting, GreenDebouncingFall, GreenDebouncingRise
}GreenButtonState_t;

// Public Function Prototypes

bool InitGreenButtonFSM(uint8_t Priority);
bool PostGreenButtonFSM(ES_Event_t ThisEvent);
ES_Event_t RunGreenButtonFSM(ES_Event_t ThisEvent);
GreenButtonState_t QueryGreenButtonSM(void);

// Event Checker Functions
void InitGreenButtonState(void);
bool CheckGreenButton(void);

#endif /* GreenButtonFSM_H */


