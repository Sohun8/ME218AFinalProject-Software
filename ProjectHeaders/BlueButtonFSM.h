/****************************************************************************

  Header file for template Flat Sate Machine
  based on the Gen2 Events and Services Framework

 ****************************************************************************/

#ifndef BlueButtonFSM_H
#define BlueButtonFSM_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */

// typedefs for the states
// State definitions for use with the query function
typedef enum
{
  InitBlueButton, BlueWaiting, BlueDebouncingFall, BlueDebouncingRise
}BlueButtonState_t;

// Public Function Prototypes

bool InitBlueButtonFSM(uint8_t Priority);
bool PostBlueButtonFSM(ES_Event_t ThisEvent);
ES_Event_t RunBlueButtonFSM(ES_Event_t ThisEvent);
BlueButtonState_t QueryBlueButtonSM(void);

// Event Checker Functions
void InitBlueButtonState(void);
bool CheckBlueButton(void);

#endif /* BlueButtonFSM_H */


