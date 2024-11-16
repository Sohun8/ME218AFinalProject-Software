/****************************************************************************
 Module
   RocketLaunchGameFSM.c

 Revision
   1.0.1

 Description
   This implements the RocketLaunchGame flat state machines under the
   Gen2 Events and Services Framework.

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 01/15/12 11:12 jec      revisions for Gen2 framework
 11/07/11 11:26 jec      made the queue static
 10/30/11 17:59 jec      fixed references to CurrentEvent in RunTemplateSM()
 10/23/11 18:20 jec      began conversion from SMTemplate.c (02/20/07 rev)
 ****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
 */
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "RocketLaunchGameFSM.h"
#include "PCEventChecker.h"
#include "IRLaunchEventChecker.h"
#include "terminal.h"
#include "dbprintf.h"
#include "PIC32_AD_Lib.h"
#include "PIC32PortHAL.h"
#include "LEDDisplayService.h"
#include "AudioService.h"
#include "TimerServoFSM.h"
#include "RocketHeightServos.h"
#include "RocketReleaseServo.h"
#include <string.h>

/*----------------------------- Module Defines ----------------------------*/
#define TESTGAME // uncomment to remove testing with keyboard events
#define SCROLL_DURATION 100 // milliseconds
#define HOLD_SEQUENCE_DURATION 4000
#define SCORE_FOR_RIGHT_ENTRY 5
#define NUM_OF_DIFFICULTIES 5
#define MAX_SEQUENCE_LENGTH 8

#define MAX_ROUNDS 6 //TODO increase to 8 or 10 so the game doesn't end before 50 seconds
#define MAX_SEED 8
#define POT_PIN_PORT _Port_B
#define POT_PIN_NUM _Pin_2
#define POT_PIN_BITS BIT4HI
#define POT_PIN_READ PORTBbits.RB2;


/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine.They should be functions
   relevant to the behavior of this state machine
 */
void SendMessage(LED_ID_t whichMsg, LED_Instructions_t whichInst);
void readPot(void);
void sendSequenceToDisplay(char* result, const char* input, int numSpaces);
void setGameOver(void);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well.
// type of state variable should match htat of enum in header file
static RocketLaunchGameState_t CurrentState;
static uint8_t gameDifficulty = 0;
static char* playerEntry;
static uint8_t roundNumber;
char customBuffer[100];
static uint8_t randomSeed;
static char* gameSequences[MAX_ROUNDS];
static uint8_t currentGuess;
static char userInput[MAX_SEQUENCE_LENGTH + 1]; //+1 for null character at end of strings
static char currentSequence[MAX_SEQUENCE_LENGTH + 1];
static uint32_t totalScore;

const uint8_t NUM_SPACES[NUM_OF_DIFFICULTIES] = {
  4,
  3,
  2,
  1,
  0
};

const uint8_t NUM_LETTERS_IN_SEQUENCE[NUM_OF_DIFFICULTIES] = {
  4,
  5,
  6,
  7,
  8
};

/* Index order: Difficulty, RandomSeed, Round# */
char *SEQUENCES[NUM_OF_DIFFICULTIES][MAX_SEED][MAX_ROUNDS] = {
  {
    {"BBRG", "BRBR", "RBRB", "GGBG", "BBBB", "GGRR"},
    {"GRRB", "GGRG", "RGGR", "RBBG", "BBRB", "RBBG"},
    {"GGRR", "BGRG", "GRBG", "RBBG", "BRGB", "RGGG"},
    {"RGBR", "GRBB", "GBRR", "BBRB", "BRBG", "RGRG"},
    {"GBRR", "RBBB", "BRRB", "GGRR", "GGRB", "BRGG"},
    {"RBBR", "BBGG", "GRGG", "BGBG", "GGBR", "GRBR"},
    {"RBBB", "BBGG", "RRGB", "RRBG", "GRBG", "BRGR"},
    {"RGRG", "BRBG", "BGGR", "RRGR", "BRBG", "BBRG"}
  },
  {
    {"BBGRG", "RGRRR", "BGGGB", "BBBGG", "RRBRG", "GGBRB"},
    {"RRBBR", "BRGGB", "BBGBR", "BGBBB", "BGBGG", "GRGGB"},
    {"BBRGR", "RGBRG", "GBRGB", "RBGBG", "BRRRB", "RRGGG"},
    {"RBGGB", "GBGGB", "BRBBG", "BBGGR", "BRGRB", "GRGGR"},
    {"RRRBB", "RRGGG", "BBGBB", "GBGGG", "GBGRB", "BGRGB"},
    {"RGBBR", "BRGGR", "BRBRB", "RBBBR", "GGGRR", "GBBGG"},
    {"RGRGR", "GGBBG", "GRRGR", "RBBBB", "GGGGB", "BBBRB"},
    {"BRRRB", "RRRBB", "RRRGR", "RBRRB", "RBRRG", "GBRGR"}
  },
  {
    {"BGRGGB", "GRGBBG", "RGBGGG", "BGGBBB", "RRBBBR", "RGGBRR"},
    {"RGRRRG", "RRBRRB", "BRGRRR", "GBBGBR", "BBGRRG", "GBBGGB"},
    {"GBGRGR", "GBGBRR", "BGBGRG", "BBBGBR", "GBRBBR", "GRRRGR"},
    {"GGBBGB", "GRBGRG", "RRGGGB", "RBBRRG", "BBRBBB", "RBRBRR"},
    {"GRBBGR", "RBBRRG", "RRBBBB", "BGGGBR", "GRBRBG", "GGGGGB"},
    {"BBGRBB", "GGRBGR", "GRGBGG", "GRBGRR", "GGRGGR", "RBRBRG"},
    {"GRBBRR", "RBGGRR", "BRBRGB", "GGRRBB", "RRBGRB", "GRRGBR"},
    {"GBBBGR", "RBRRRR", "RGGRGR", "RGGGBR", "RGGBGG", "GGBBGB"}
  },
  {
    {"RRGBGRG", "BBGGGGB", "GBRRRGG", "GGBBRBR", "BRBGRBR", "RGGGRRG"},
    {"GGBGRGB", "RGBBGGR", "GGBBGBG", "GRGRRBB", "BRGGRBG", "BGRBRBB"},
    {"BBRRGGB", "GBRBBBR", "GRRRBBG", "BBGBGRG", "GRGRRGB", "BGGBBGG"},
    {"BBRBRRR", "RRGGGRB", "RRGBGBR", "GBRGBRB", "BBRGGRB", "BBBRGRR"},
    {"GRGRGRR", "RRBGGBB", "RRBBGRG", "GBRGBBR", "BRRBBRB", "RGRRBBB"},
    {"GBGGGBB", "RBRBGBR", "GGRBRRB", "RBBGRGG", "GBBGGRB", "RRBGBGG"},
    {"BRBRBRB", "BGBBBBB", "RBRBGGB", "RGRGBGG", "BRGBGBR", "BBBBGBR"},
    {"BBRBGGB", "BBGRBRB", "GBGBRBR", "GBRGRGR", "BGBGBBB", "BRGGGRR"}
  },
  {
    {"GBRGGBRB", "BGRBRBGG", "GRRBRBBR", "BGGBRBBG", "BGGRRGRG", "RRRBGRBB"},
    {"RBRRBRGR", "RBRGGBGB", "GBBBGGRB", "GGBBBGRG", "RBGGGBGR", "BRBBBBRR"},
    {"BGGGRRRB", "BRBBBGBB", "GRBRRGRG", "GRRRBGGG", "RBRGBGBG", "RRRBRBGB"},
    {"BRGBBGRG", "GGBRRGBB", "RBBGGBRB", "GRRGBGGR", "BRGRGGGB", "GGBRGRBG"},
    {"GGRBRBBR", "RRBBGRRB", "RGGBBGBB", "GRBRGBBR", "BBRGRBGG", "RBBGGGGB"},
    {"BGRGGGGG", "RBRGGBGG", "GGRGRBRB", "BGGRGRGR", "RGGRRGRB", "BGRRGRBB"},
    {"RGGBGBGG", "GGGBRBBB", "GRRRGBGB", "RRRBBGRB", "BGGRRRRR", "RRGRGRBG"},
    {"GGBGRRBR", "BBRBGBBB", "BBGGBGBB", "GGGRRGGB", "BRGBBBRR", "RRBBRRBG"}
  }
};

char* USER_INPUTS[NUM_OF_DIFFICULTIES] = {
  "____",
  "_____",
  "______",
  "_______",
  "________"
};

// with the introduction of Gen2, we need a module level Priority var as well
static uint8_t MyPriority;

/*------------------------------ Module Code ------------------------------*/

/****************************************************************************
 Function
     InitRocketLaunchGameFSM

 Parameters
     uint8_t : the priority of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, sets up the initial transition and does any
     other required initialization for this state machine
 Notes

 Author
     J. Edward Carryer, 10/23/11, 18:55
 ****************************************************************************/
bool InitRocketLaunchGameFSM(uint8_t Priority) {
  ES_Event_t ThisEvent;

  MyPriority = Priority;
  // put us into the Initial PseudoState
  CurrentState = Initializing;

  // initialize event checkers
  InitPCSensorStatus(); // Poker Chip Detection Event Checker
  InitIRLaunchSensorStatus(); // IR Launch Sensor Event Checker
  //PortSetup_ConfigureAnalogInputs(POT_PIN_PORT, POT_PIN_NUM);
  ADC_ConfigAutoScan(POT_PIN_BITS);
  DB_printf("\nInitializing RocketLaunchGameFSM");
  DB_printf("\nkeyboard events for testing: ");
  DB_printf("\n 0,1,2,3=rocket height");
  DB_printf("\n 8=lock rocket 9=launch rocket");
  DB_printf("\n 4=music, 5=correct, 6=wrong");
  DB_printf("\n p=coin R=red button, G= green button, B= blue button s =limit switch\n\n");
  DB_printf("\nInitializing RocketLaunchGameFSM ");

  // post the initial transition event
  ThisEvent.EventType = ES_INIT;
  if (ES_PostToService(MyPriority, ThisEvent) == true) {
    return true;
  } else {
    return false;
  }
}

/****************************************************************************
 Function
     PostRocketLaunchGameFSM

 Parameters
     EF_Event_t ThisEvent , the event to post to the queue

 Returns
     boolean False if the Enqueue operation failed, True otherwise

 Description
     Posts an event to this state machine's queue
 Notes

 Author
     J. Edward Carryer, 10/23/11, 19:25
 ****************************************************************************/
bool PostRocketLaunchGameFSM(ES_Event_t ThisEvent) {
  return ES_PostToService(MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunRocketLaunchGameFSM

 Parameters
   ES_Event_t : the event to process

 Returns
   ES_Event_t, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   add your description here
 Notes
   uses nested switch/case to implement the machine.
 Author
   J. Edward Carryer, 01/15/12, 15:23
 ****************************************************************************/
ES_Event_t RunRocketLaunchGameFSM(ES_Event_t ThisEvent) {
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors

  bool humanInteracted = false;

  if (ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == TIMEOUT_TIMER) {
    // special case for the 20 second timer which can timeout anything and reset through Initializing to Welcoming
    CurrentState = Initializing;
    ES_Event_t NewEvent;
    NewEvent.EventType = ES_INIT;
    PostRocketLaunchGameFSM(NewEvent);
  } else {
    switch (CurrentState) {
      case Initializing: // If current state is initial Pseudo State
      {
        if (ThisEvent.EventType == ES_INIT) {
          CurrentState = Welcoming;
          roundNumber = 0;
          totalScore = 0;
          // Send Scrolling Welcome Message to display
          SendMessage(MSG_STARTUP, SCROLL_REPEAT);

          ES_Event_t NewEvent;
          NewEvent.EventType = ES_RESET_GAME_TIMER;
          PostTimerServoFSM(NewEvent);

        }
      }
        break;

      case Welcoming:
      {
        switch (ThisEvent.EventType) {
          case ES_PC_INSERTED: //If poker chip is inserted
          {
            humanInteracted = true;
            CurrentState = _1CoinInserted;
            SendMessage(MSG_CHIPCOUNT1, DISPLAY_HOLD);
            DB_printf("Poker Chip 1 Detected"); // Print detection status for debugging
          }
            break;

#ifdef TESTGAME
          case ES_NEW_KEY:
          {
            if (ThisEvent.EventParam == 'p') {
              ES_Event_t NewEvent;
              NewEvent.EventType = ES_PC_INSERTED;
              PostRocketLaunchGameFSM(NewEvent);
            }
          }
            break;
#endif /* TESTGAME */
        }
      }
        break;

      case _1CoinInserted:
      {
        switch (ThisEvent.EventType) {
          case ES_PC_INSERTED: //If poker chip is inserted
          {
            humanInteracted = true;
            CurrentState = PromptingToPlay;
            SendMessage(MSG_PROMPT2PLAY, SCROLL_REPEAT_SLOW);
            DB_printf("Poker Chip 2 Detected"); // Print detection status for debugging

            ES_Event_t NewEvent;
            NewEvent.EventType = ES_AUDIO_PLAY;
            NewEvent.EventParam = AUDIO_PLAY_MUSIC;
            PostAudioService(NewEvent);

            NewEvent.EventType = ES_START_GAME_TIMER;
            PostTimerServoFSM(NewEvent);

          }
            break;

#ifdef TESTGAME
          case ES_NEW_KEY:
          {
            if (ThisEvent.EventParam == 'p') {
              ES_Event_t NewEvent;
              NewEvent.EventType = ES_PC_INSERTED;
              PostRocketLaunchGameFSM(NewEvent);
            }
          }
            break;
#endif /* TESTGAME */
        }
      }
        break;

      case PromptingToPlay:
      {
        switch (ThisEvent.EventType) {
          case ES_LIMIT_SWITCH:
          {
            humanInteracted = true;
            CurrentState = DisplayingInstructions;
            randomSeed = ES_Timer_GetTime() % 8;
            SendMessage(MSG_INSTRUCTIONS, SCROLL_ONCE);

            ES_Event_t NewEvent;
            NewEvent.EventType = ES_ROCKET_RELEASE_SERVO_LOCK;
            DB_printf("rocket ready\n");
            PostRocketReleaseServo(NewEvent);

          }
            break;

#ifdef TESTGAME
          case ES_NEW_KEY:
          {
            if (ThisEvent.EventParam == 's') {
              ES_Event_t NewEvent;
              NewEvent.EventType = ES_LIMIT_SWITCH;
              PostRocketLaunchGameFSM(NewEvent);
            }
          }
            break;
#endif /* TESTGAME */        
        }
      }
        break;

      case DisplayingInstructions:
      {
        switch (ThisEvent.EventType) {
          case ES_FINISHED_SCROLLING:
          {
            // hold end of message for one second
            ES_Timer_InitTimer(HOLD_MESSAGE_TIMER, 1000);
            DB_printf("\n In Finished scrolling \n");
          }
            break;
          case ES_TIMEOUT:
          {
            if (ThisEvent.EventParam == HOLD_MESSAGE_TIMER) {
              CurrentState = ChoosingDifficulty;
              SendMessage(MSG_CHOOSE_DIFF, SCROLL_REPEAT);
              // Set time to choose difficulty
              ES_Timer_InitTimer(HOLD_MESSAGE_TIMER, 5000);
            }
          }
            break;
        }
      }
        break;

      case ChoosingDifficulty:
      {
        switch (ThisEvent.EventType) {
          case ES_TIMEOUT:
          {
            if (ThisEvent.EventParam == HOLD_MESSAGE_TIMER) {
              readPot(); // get and store difficulty
              CurrentState = RoundInit;
              DB_printf("\n Game Difficulty: %d\n", gameDifficulty);
              roundNumber++;
              //gameSequences = SEQUENCES[gameDifficulty-1][randomSeed];

              /* Send round message */
              sprintf(customBuffer, "    Round %d    ", roundNumber);
              currentMessage = customBuffer;
              SendMessage(MSG_CUSTOM, DISPLAY_HOLD);

              // Timer for round message 
              ES_Timer_InitTimer(HOLD_MESSAGE_TIMER, 2000);
              CurrentState = RoundInit;
            }
          }
            break;
        }
      }
        break;

      case RoundInit:
      {
        switch (ThisEvent.EventType) {
          case ES_GAME_OVER:
          {
            setLaunchRocket();
          }
            break;

          case ES_TIMEOUT:
          {
            if (ThisEvent.EventParam == HOLD_MESSAGE_TIMER) {
              //currentSequence = gameSequences[roundNumber-1];
              for (uint8_t i = 0; i < MAX_SEQUENCE_LENGTH + 1; i++) {
                userInput[i] = 0;
                currentSequence[i] = 0;
              }
              strncpy(userInput, USER_INPUTS[gameDifficulty - 1], NUM_LETTERS_IN_SEQUENCE[gameDifficulty - 1]);
              strncpy(currentSequence, SEQUENCES[gameDifficulty - 1][randomSeed][roundNumber - 1], NUM_LETTERS_IN_SEQUENCE[gameDifficulty - 1]);

              sendSequenceToDisplay(customBuffer, currentSequence, NUM_SPACES[gameDifficulty - 1]);

              currentGuess = 0;
              ES_Timer_InitTimer(HOLD_MESSAGE_TIMER, HOLD_SEQUENCE_DURATION);
              CurrentState = WaitForButton;
            }
          }
            break;
        }
      }
        break;

      case WaitForButton:
      {
        switch (ThisEvent.EventType) {
          case ES_TIMEOUT:
          {
            if (ThisEvent.EventParam = HOLD_MESSAGE_TIMER) {
              sendSequenceToDisplay(customBuffer, userInput, NUM_SPACES[gameDifficulty - 1]);
            }
          }
            break;

          case ES_GAME_OVER:
          {
            setLaunchRocket();
          }
            break;

          case ES_BUTTON_PRESS:
          {
            if (ThisEvent.EventParam == 'R' || ThisEvent.EventParam == 'G' || ThisEvent.EventParam == 'B') {
              humanInteracted = true;
              userInput[currentGuess] = ThisEvent.EventParam;
              if (userInput[currentGuess] == currentSequence[currentGuess]) {

                ES_Event_t NewEvent;
                NewEvent.EventType = ES_AUDIO_PLAY;
                NewEvent.EventParam = AUDIO_PLAY_CORRECT;
                PostAudioService(NewEvent);

                totalScore += SCORE_FOR_RIGHT_ENTRY;
              } else {//wrong
                ES_Event_t NewEvent;
                NewEvent.EventType = ES_AUDIO_PLAY;
                NewEvent.EventParam = AUDIO_PLAY_WRONG;
                PostAudioService(NewEvent);
              }
              currentGuess++;
              sendSequenceToDisplay(customBuffer, userInput, NUM_SPACES[gameDifficulty - 1]);
            }
            // If Round over
            if (currentGuess >= NUM_LETTERS_IN_SEQUENCE[gameDifficulty - 1]) {
              CurrentState = RoundInit;
              DB_printf("\n total score: %d \n", totalScore);

              roundNumber++;
              // if not at max number of rounds yet
              if (roundNumber <= MAX_ROUNDS) {

                /* Send round message */
                sprintf(customBuffer, "   Round %d    ", roundNumber);
                currentMessage = customBuffer;
                SendMessage(MSG_CUSTOM, DISPLAY_HOLD);

                // Timer for round message 
                ES_Timer_InitTimer(HOLD_MESSAGE_TIMER, 2000);
                CurrentState = RoundInit;
              } else {
              }
            }
          }
            break;

#ifdef TESTGAME
          case ES_NEW_KEY:
          {
            if (ThisEvent.EventParam == 'R' || ThisEvent.EventParam == 'G' || ThisEvent.EventParam == 'B') {
              ES_Event_t NewEvent;
              NewEvent.EventType = ES_BUTTON_PRESS;
              NewEvent.EventParam = ThisEvent.EventParam;
              PostRocketLaunchGameFSM(NewEvent);
            }
          }
            break;
#endif /* TESTGAME */
        }
      }
        break;

      case LaunchRocket:
      {
        switch (ThisEvent.EventType) {
          case ES_IR_LAUNCH:
          {
            humanInteracted = true;
            setGameOver();
          }
        }
      }
        break;

      case GameOver:
      {
        switch (ThisEvent.EventType) {
          case ES_PC_INSERTED:
          {
            //TODO: it would be great if entering a coin leaves the game over screen early
          }
        }
        // will time out after 20 seconds just like all states do
      }
        break;

      default:
        ;
    } // end switch on Current State

    if (humanInteracted) {
      ES_Timer_InitTimer(TIMEOUT_TIMER, 20000);
    }

  }

  return ReturnEvent;
}

/****************************************************************************
 Function
     QueryRocketLaunchGameSM

 Parameters
     None

 Returns
     TemplateState_t The current state of the Template state machine

 Description
     returns the current state of the Template state machine
 Notes

 Author
     J. Edward Carryer, 10/23/11, 19:21
 ****************************************************************************/
RocketLaunchGameState_t QueryRocketLaunchGameSM(void) {
  return CurrentState;
}

/***************************************************************************
 private functions
 ***************************************************************************/

// sends ES_NEW_MESSAGE event to LEDDisplayService depending on given message id and display instructions
// see LEDDisplayService.h for type definitions 

void SendMessage(LED_ID_t whichMsg, LED_Instructions_t whichInst) {
  ES_Event_t MessageEvent;
  MessageEvent.EventType = ES_NEW_MESSAGE;

  paramUnion msgParams;
  msgParams.msgID = whichMsg;
  msgParams.dispInstructions = whichInst;

  MessageEvent.EventParam = msgParams.fullParam;
  PostLEDDisplayService(MessageEvent);
}

void readPot(void) {
  uint32_t adcResults[1];
  ADC_MultiRead(adcResults);
  gameDifficulty = (adcResults[0] * 4) / 1000 + 1;
  DB_printf("\nAnalog Val: %d:    ", adcResults[0]);
  DB_printf("Difficulty: %d\n", gameDifficulty);
}

void setLaunchRocket() {
  ES_Event_t NewEvent;
  NewEvent.EventType = ES_RESET_GAME_TIMER;
  PostTimerServoFSM(NewEvent);

  sprintf(customBuffer, "LAUNCH ROCKET!  ");
  currentMessage = customBuffer;
  SendMessage(MSG_CUSTOM, SCROLL_REPEAT_SLOW);
  CurrentState = LaunchRocket;

  NewEvent.EventType = ES_ROCKET_SERVO_HEIGHT;
  NewEvent.EventParam = totalScore * 4 / 210; //TODO: improve score scaling
  DB_printf("rocketHeight=%d\n", NewEvent.EventParam);
  PostRocketHeightServos(NewEvent); //TODO: start timer to let servos finish moving?
}

void setGameOver() {
  ES_Event_t NewEvent;
  NewEvent.EventType = ES_ROCKET_RELEASE_SERVO_LAUNCH;
  PostRocketReleaseServo(NewEvent);

  sprintf(customBuffer, "LIFTOFF!  Total Score: %d    ", totalScore);
  currentMessage = customBuffer;
  SendMessage(MSG_CUSTOM, SCROLL_REPEAT_SLOW);
  CurrentState = GameOver;

  DB_printf("\n Game over! Total Score: %d \n", totalScore);
}

// adds spaces to the sequence to display it to the LED matrix

void sendSequenceToDisplay(char* result, const char* input, int numSpaces) {
  // Add spaces at the beginning
  for (int i = 0; i < numSpaces; i++) {
    result[i] = ' ';
  }

  // Add the characters from input with spaces between them
  int j = numSpaces;
  for (int i = 0; i < strlen(input); i++) {
    result[j++] = input[i];
    if (i < strlen(input) - 1) {
      result[j++] = ' '; // Add space between characters
    }
  }

  // Add spaces at the end
  for (int i = 0; i < numSpaces; i++) {
    result[j++] = ' ';
  }

  // Null-terminate the result string
  result[j] = '\0';

  currentMessage = result;
  SendMessage(MSG_CUSTOM, DISPLAY_HOLD);
}