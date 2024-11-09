/****************************************************************************

  Header file for template Flat Sate Machine
  based on the Gen2 Events and Services Framework

 ****************************************************************************/

#ifndef LEDFSM_H
#define LEDFSM_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */

// typedefs for the states
// State definitions for use with the query function
typedef enum
{
  InitPState, Waiting, Updating
}LEDState_t;

// Public Function Prototypes

bool InitLEDFSM(uint8_t Priority);
bool PostLEDFSM(ES_Event_t ThisEvent);
ES_Event_t RunLEDFSM(ES_Event_t ThisEvent);
LEDState_t QueryLEDFSM(void);

#endif /* FSMTemplate_H */

