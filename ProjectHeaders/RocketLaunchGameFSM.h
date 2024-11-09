/****************************************************************************

  Header file for the RocketLaunchGame Flat Sate Machine
  based on the Gen2 Events and Services Framework

 ****************************************************************************/

#ifndef RocketLaunchGameFSM_H
#define RocketLaunchGameFSM_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */

// typedefs for the states
// State definitions for use with the query function
typedef enum
{
  InitPState, Welcoming, _1CoinInserted, _2CoinsInserted,
}RocketLaunchGameState_t;

// Public Function Prototypes

bool InitRocketLaunchGameFSM(uint8_t Priority);
bool PostRocketLaunchGameFSM(ES_Event_t ThisEvent);
ES_Event_t RunRocketLaunchGameFSM(ES_Event_t ThisEvent);
RocketLaunchGameState_t QueryRocketLaunchGameSM(void);

#endif /* RocketLaunchGameFSM_H */

