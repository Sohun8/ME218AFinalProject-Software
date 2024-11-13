/****************************************************************************

  Header file for LEDDisplayService
  based on the Gen 2 Events and Services Framework
 * 
 * This module takes Events from the RocketLaunchGameFSM to repeatedly scroll,
 * scroll once, or display (without scrolling) a pre-set message
 * 
 * 
 * Events this service responds to:
 **** ES_NEW_MESSAGE, ES_CLEAR_MESSAGE, ES_KEEP_UDPATING (Internal), ES_TIMEOUT (Internal)
 * Events this service posts:
 **** ES_FINISHED_SCROLLING 
 ****************************************************************************/

#ifndef LED_DISPLAY_SERVICE_H
#define LED_DISPLAY_SERVICE_H

#include "ES_Types.h"

/* putting this variable here and making it global will allow us to send "RGB" sequences that aren't 
predetermined at the start of the game to this service 
*/
extern char* currentMessage; // pointer to the current message string

/* Enums to represent msgID and dispInstructions values
 * Allows for uniform communications between RocketLaunchGameFSM and this service
*/
typedef uint8_t LED_ID_t;
enum{
  MSG_STARTUP = 0,
  MSG_CHIPCOUNT1,
  MSG_CHIPCOUNT2,
  MSG_PROMPT2PLAY,
  MSG_INSTRUCTIONS,
  // Insert New IDs here:
  MSG_RGB_SEQUENCE
  // DO NOT INSERT HERE
};

/**************************************************************************** */
typedef uint8_t LED_Instructions_t;
enum{
    DISPLAY_HOLD = 0,
    SCROLL_ONCE,
    SCROLL_REPEAT
};


// union to represent EventParam of an ES_NEW_MESSAGE Event sent to this service
typedef union {
  uint16_t fullParam; // full 16-bit parameter value
  struct{
    LED_ID_t msgID; // segment corresponding to message identifier (Lower 8 bits)
    LED_Instructions_t dispInstructions; // segment corresponding to display instructions (Upper 8 bits)
  };
} paramUnion;


// Public Function Prototypes
bool InitLEDDisplayService(uint8_t Priority);
bool PostLEDDisplayService(ES_Event_t ThisEvent);
ES_Event_t RunLEDDisplayService(ES_Event_t ThisEvent);

#endif // LED_DISPLAY_SERVICE_H

