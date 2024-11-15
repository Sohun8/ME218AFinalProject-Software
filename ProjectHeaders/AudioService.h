/****************************************************************************

  Header file for template service
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/

#ifndef AudioService_H
#define AudioService_H

#include "ES_Types.h"

#define AUDIO_PLAY_MUSIC 1
#define AUDIO_PLAY_CORRECT 2
#define AUDIO_PLAY_WRONG 3

// Public Function Prototypes

bool InitAudioService(uint8_t Priority);
bool PostAudioService(ES_Event_t ThisEvent);
ES_Event_t RunAudioService(ES_Event_t ThisEvent);

#endif /* AudioService_H */

