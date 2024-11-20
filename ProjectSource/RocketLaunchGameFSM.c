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
//#define TESTGAME // uncomment to remove testing with keyboard events
#define SCROLL_DURATION 100 // milliseconds
#define HOLD_SEQUENCE_DURATION 3500
#define TIMEOUT_DURATION 20000
#define NUM_OF_DIFFICULTIES 5
#define MAX_SEQUENCE_LENGTH 8

#define MAX_ROUNDS 10
#define MAX_SEED 8
#define POT_PIN_PORT _Port_B
#define POT_PIN_NUM _Pin_2
#define POT_PIN_BITS BIT4HI
#define POT_PIN_READ PORTBbits.RB2;

#define ALTITUDE_2_SCORE 50
#define ALTITUDE_3_SCORE 75
#define ALTITUDE_4_SCORE 100

#define SCORE_FOR_RIGHT_ENTRY_FORMULA 6.0 - knobAnalogReadVal / 250.0

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
// type of state variable should match that of enum in header file
static float SCORE_FOR_RIGHT_ENTRY = 0;
static float totalScore;

static RocketLaunchGameState_t CurrentState;
static uint8_t gameDifficulty = 0;
static uint32_t difficultyKnobVal = 0;
static uint16_t knobAnalogReadVal = 0;
static uint32_t lastDifficultyKnobVal = 0;
static char* playerEntry;
static uint8_t roundNumber;
char customBuffer[100];
static uint8_t randomSeed;
static char* gameSequences[MAX_ROUNDS];
static uint8_t currentGuess;
static char userInput[MAX_SEQUENCE_LENGTH + 1]; //+1 for null character at end of strings
static char currentSequence[MAX_SEQUENCE_LENGTH + 1];

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
    {"BGRB", "BRGG", "BBGG", "RGRG", "RRRB", "RBRB", "GGRB", "BRBR", "GGBB", "BRRR"},
    {"BBGR", "BGGB", "GGBB", "GGGG", "BRRB", "GBBB", "RRBB", "RRBR", "BBGR", "GGGG"},
    {"GGBB", "BBRR", "RRRG", "GGBG", "BGRB", "RGGR", "GRGG", "BBBB", "RBRB", "GRBG"},
    {"BGRB", "GGRR", "GGGG", "RRRG", "RBBR", "GBGR", "GRGB", "GBRG", "RGBB", "GGGG"},
    {"BRRB", "BRBG", "RGGG", "GRGB", "RGRG", "GBBG", "GGRR", "BBRB", "BGGR", "BGRG"},
    {"GBGG", "GBRG", "RRBG", "BRRB", "GGBG", "BBGG", "GBBG", "RRRB", "GBBB", "BRRB"},
    {"GRBG", "GGBR", "GRRR", "BRBB", "RRGR", "RRRG", "RGGB", "GBGR", "BBRB", "GBGG"},
    {"BBRR", "GRRB", "GGRB", "RGBB", "BGRR", "BRRG", "RRBR", "GRRB", "BBRR", "BGRR"}
  },
  {
    {"GGRRB", "BGRGB", "BGBBG", "BGRRG", "BRRBG", "RBGRR", "BRBGG", "GBRRR", "GGRBG", "BBRRG"},
    {"BBRBR", "GGRBB", "GBRRG", "RBRGR", "GBBGR", "RRBGG", "RBRGR", "BBRRR", "GRRBR", "RBBRR"},
    {"RRGBR", "BBGGR", "RRRRG", "RBRGB", "RRGGB", "BBRRB", "GRGGG", "BRGGR", "RGRBG", "GGRRB"},
    {"GRGRR", "BGBBG", "GGBRB", "GRRGB", "BGRBB", "RBRRG", "GBBRG", "BRGGG", "RRBGB", "GGBRR"},
    {"RRRRR", "BBBGB", "RBRRG", "BBGRB", "BRBGR", "BRGGB", "GGGRG", "BGRGR", "BBGGG", "RBBRR"},
    {"BGRGB", "BBRGG", "BRBBB", "RGRBG", "BBBRB", "RRRBG", "GRGGG", "RGGGR", "RRRRB", "GGGRB"},
    {"GBRGR", "BGBBR", "BRGBR", "GRRBR", "GGGBR", "GGBBG", "BBGBB", "GBRRR", "RRRRG", "RBBGB"},
    {"RRRRG", "RRRGB", "GGRGR", "GRRBB", "RGRBR", "BGRBG", "GGGBR", "GRBBR", "GBBRB", "RBRRG"}
  },
  {
    {"RGBRGR", "RRGRGG", "GGBRBG", "RGRBGR", "RGBRBR", "BBBGBG", "BRGGGB", "RGRRBG", "RGBBBB", "RRBBRG"},
    {"BGRBRR", "RGRBGB", "RGGGBG", "BBRRGR", "RGGGBR", "BBGRBG", "BGRRGR", "RRRBRR", "BBGGGR", "BRGBRB"},
    {"RBRGRR", "BBRRBB", "RRRRRG", "BGBGBR", "BRGGGR", "BGRGGR", "GRGGBG", "GRGRRG", "RRBRRB", "RBGGRR"},
    {"GGRBBB", "RBGBGR", "RRBBGG", "RGBRRB", "BRRGGR", "GBGRBG", "RRRRBB", "BGBRBG", "RGBRGB", "RRGBRB"},
    {"GRBRRG", "BRBBGB", "GGRBGG", "BBGGGG", "GBGBRR", "GGGGBR", "RBRGRB", "GBRRRR", "BGBGRR", "RBGBRR"},
    {"GRGBBG", "GRRRRG", "RRGGBB", "GGGGBG", "RGRGGR", "BBGRGG", "BRRGBB", "BRBRBR", "BBRGBB", "BGBRGB"},
    {"BRGRBR", "BRGGGG", "RGBRGR", "GGRBRB", "BRGGRG", "BRRGRG", "GGGRGB", "BBBRGG", "BBGRBG", "GRRBGG"},
    {"RBBBRG", "BGBRBB", "GBBBBG", "GGGGBG", "GBRBRR", "BBRGGR", "GRRGRG", "RGBRBG", "GBBBGG", "BBBRBG"}
  },
  {
    {"GBRBGGR", "RBRRRGG", "GBRRBGG", "GRRGRBG", "GGGRRRB", "GRRGRRG", "RRRBGGR", "BRRGRBG", "BGRRRGB", "GGGBBGB"},
    {"RBRBBGG", "RRGGBBR", "RGRGGRB", "RRRRRBR", "BGRRBRG", "RRBRBBR", "GBGRRRB", "GRGRGGR", "RGGBGRR", "BBGBBBB"},
    {"RGGGBRR", "RRGGBGB", "BBRGBGB", "RGBBGRG", "BRRGRGB", "BBGRGRB", "GBBGGBR", "BGBBRRG", "RGRRRRR", "GRGBBRB"},
    {"RGGRBGB", "RRBBGBB", "BGRGGRR", "BRGGGBR", "RBGBBRB", "BBBBBGB", "GBRBGBG", "BGGRGBB", "RRRGRRB", "BRRRRRG"},
    {"RGRRRBG", "RGBGBGR", "RRGBGGG", "GBBBRGR", "RBBBGGB", "RGGBBGR", "GGRGRBB", "RBGBRRR", "BBRRBRR", "RRRGRRB"},
    {"RBRBGBB", "BGRRGGR", "BRBBGRB", "BGGGRRG", "GBBBGRR", "GGRGGGG", "GRRBBBB", "BGRRBRB", "GRRRRGG", "GRBBBRG"},
    {"RRBGGGG", "GBRGGGR", "RGBBRGB", "BGRGGRB", "BBRBRRG", "RRBBGBR", "GGBRGRR", "GRGGGRR", "BBGBRRR", "RGBGGRG"},
    {"GBRRBBG", "BRGGRRR", "GBRBBGB", "RGBGGGB", "RRRBRBG", "GGRGGGG", "BBRRGRG", "BRBBGGR", "RBBBGGG", "BRBRGBR"}
  },
  {
    {"GBGBRBRR", "BGRBBRGG", "RBRRBGGB", "BBRGBGRG", "RGBGBGRB", "GBRBGGBG", "RGRBGRGR", "RGGBGRRR", "RRGBGGBR", "GRBBBGBB"},
    {"GRGBRGBG", "BBGRRBRR", "BRRBBRGG", "GGBBGRGB", "RBBRBRRR", "GBBRRRGB", "GBGRRGGG", "RGRBGRBB", "BRBBGRRB", "RGRGRBRG"},
    {"BBGBRRGB", "BRGRGGGB", "GRRRBBBB", "BRBGGRGR", "GGGGRGBB", "BRRGBRRR", "GRBGRRRB", "RBBGBBGR", "BGBBBBRG", "GBBGRRGR"},
    {"GGBGRGBG", "RGRRBBRG", "RBBRRBGB", "RBRGBGGR", "GBBGRBBG", "RRRRBRBB", "RGBBGRGB", "BBGRBBRB", "RGRRBGRG", "BBRGRGBB"},
    {"GGRBRBGB", "RGBGGRRR", "RBBRRGBR", "BGGBRGRR", "RGBGRGBR", "RRGBRRBR", "GRRBGGGG", "BGGBGRRR", "RRRRRBBG", "RGBBBGRB"},
    {"BBRGBBRR", "GRGBRBBR", "BBGBGRRG", "BGGBBRRG", "BGGBRBBR", "GBBBGRBB", "BGRRGBRG", "GRRGBGGR", "GBRBBRBR", "BRBRGRRR"},
    {"BBBRGRBG", "RBBRGRBB", "GRGRBRGB", "GRRRGRBR", "GRRGBRBB", "RRRBBRRG", "GBRBBGBB", "RBBGGRBB", "BBRGBRBR", "BRGBBBGR"},
    {"RGRGRBRB", "RGBGBGBG", "GGRBBRBR", "GRRBBGRG", "BBRBBGBR", "BBRGBBRG", "GBGRBBRG", "RRGGBBGB", "RBBGRRBB", "GBRGBBRG"}
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
    // special case for the 20 second timer which can timeout anything and send error message to display
    // stop game timer
    ES_Event_t NewEvent;
    NewEvent.EventType = ES_RESET_GAME_TIMER;
    PostTimerServoFSM(NewEvent);
    // Goes to state for displaying timeout message
    CurrentState = DisplayingTimeout;
    SendMessage(MSG_TIMEOUT, SCROLL_ONCE_SLOW);
  } else {
    switch (CurrentState) {
      case Initializing: // If current state is initial Pseudo State
      {
        if (ThisEvent.EventType == ES_INIT) {
          CurrentState = Welcoming;
          ES_Timer_StopTimer(TIMEOUT_TIMER);
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
            randomSeed = ES_Timer_GetTime() % 8;
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
              SendMessage(MSG_CHOOSE_DIFF, DISPLAY_HOLD);
              // Set time to choose difficulty
              ES_Timer_InitTimer(HOLD_MESSAGE_TIMER, 6000);
              ES_Timer_InitTimer(CHOOSE_DIFFICULTY_TIMER, 200);
              readPot();
              lastDifficultyKnobVal = knobAnalogReadVal;
            }
          }
            break;
        }
      }
        break;

      case ChoosingDifficulty:
      {
        switch (ThisEvent.EventType) {
          case ES_BUTTON_PRESS:
          {
            if (ThisEvent.EventParam == 'R' || ThisEvent.EventParam == 'G' || ThisEvent.EventParam == 'B'){
              ES_Timer_StopTimer(CHOOSE_DIFFICULTY_TIMER);
              ES_Timer_StopTimer(HOLD_MESSAGE_TIMER);
              readPot(); // get and store difficulty
              SCORE_FOR_RIGHT_ENTRY = SCORE_FOR_RIGHT_ENTRY_FORMULA;
              DB_printf("\n score per letter x1000: %d\n", (int32_t) (SCORE_FOR_RIGHT_ENTRY * 1000));

              DB_printf("\n Game Difficulty: %d\n", gameDifficulty);
              roundNumber++;
              //gameSequences = SEQUENCES[gameDifficulty-1][randomSeed];

              /* Send round message */
              sprintf(customBuffer, "    Round %d    ", roundNumber);
              currentMessage = customBuffer;
              SendMessage(MSG_CUSTOM, DISPLAY_HOLD);

              // Timer for round message 
              ES_Timer_InitTimer(HOLD_MESSAGE_TIMER, 1000);
              CurrentState = RoundInit;
            }
          }
          break;
          
          case ES_TIMEOUT:
          {
            if (ThisEvent.EventParam == CHOOSE_DIFFICULTY_TIMER) {
              readPot();

              if (abs(knobAnalogReadVal - lastDifficultyKnobVal) > 3) {
                humanInteracted = true;
                lastDifficultyKnobVal = knobAnalogReadVal;
                sprintf(customBuffer, "Difficulty: %d", difficultyKnobVal);
                currentMessage = customBuffer;
                SendMessage(MSG_CUSTOM, DISPLAY_HOLD);
              }

              ES_Timer_InitTimer(CHOOSE_DIFFICULTY_TIMER, 200);
            }
            if (ThisEvent.EventParam == HOLD_MESSAGE_TIMER) {
              ES_Timer_StopTimer(CHOOSE_DIFFICULTY_TIMER);
              readPot(); // get and store difficulty
              SCORE_FOR_RIGHT_ENTRY = SCORE_FOR_RIGHT_ENTRY_FORMULA;
              DB_printf("\n score per letter x1000: %d\n", (int32_t) (SCORE_FOR_RIGHT_ENTRY * 1000));

              DB_printf("\n Game Difficulty: %d\n", gameDifficulty);
              roundNumber++;
              //gameSequences = SEQUENCES[gameDifficulty-1][randomSeed];

              /* Send round message */
              sprintf(customBuffer, "    Round %d    ", roundNumber);
              currentMessage = customBuffer;
              SendMessage(MSG_CUSTOM, DISPLAY_HOLD);

              // Timer for round message 
              ES_Timer_InitTimer(HOLD_MESSAGE_TIMER, 1000);
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

              roundNumber++;
              // if not at max number of rounds yet
              if (roundNumber <= MAX_ROUNDS) {

                /* Send round message */
                sprintf(customBuffer, "   Round %d    ", roundNumber);
                currentMessage = customBuffer;
                SendMessage(MSG_CUSTOM, DISPLAY_HOLD);

                // Timer for round message 
                ES_Timer_InitTimer(HOLD_MESSAGE_TIMER, 1000);
                CurrentState = RoundInit;
              } else {
                setLaunchRocket();
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
            break;
          case ES_TIMEOUT:
          {
            if (ThisEvent.EventParam == HOLD_MESSAGE_TIMER) {
              currentMessage = "WAVE TO LAUNCH ROCKET!   ";
              SendMessage(MSG_CUSTOM, SCROLL_REPEAT_SLOW);
            }
          }
            break;
        }
      }
        break;

      case GameOver:
      {
        switch (ThisEvent.EventType) {
          case ES_FINISHED_SCROLLING:
          {
            // Hold message for 5 s
            ES_Timer_InitTimer(HOLD_MESSAGE_TIMER, 5000);
          }
            break;

          case ES_TIMEOUT:
          {
            if (ThisEvent.EventParam == HOLD_MESSAGE_TIMER) {
              CurrentState = Initializing;
              ES_Event_t NewEvent;
              NewEvent.EventType = ES_INIT;
              PostRocketLaunchGameFSM(NewEvent);
            }
          }
            break;
        }
      }
        break;

      case DisplayingTimeout:
      {
        switch (ThisEvent.EventType) {
          case ES_FINISHED_SCROLLING:
          {
            // hold end of error message for a second
            ES_Timer_InitTimer(HOLD_MESSAGE_TIMER, 1000);
          }
            break;
          case ES_TIMEOUT:
          {
            if (ThisEvent.EventParam == HOLD_MESSAGE_TIMER) {
              CurrentState = Initializing;
              ES_Event_t NextEvent;
              NextEvent.EventType = ES_INIT;
              PostRocketLaunchGameFSM(NextEvent);
            }
          }
            break;
        }
      }
        break;

      default:
        ;
    } // end switch on Current State

    if (humanInteracted) {
      ES_Timer_InitTimer(TIMEOUT_TIMER, TIMEOUT_DURATION);
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

  DB_printf("sendMessage: %s| with instructions %d\n", currentMessage, whichInst);

  paramUnion msgParams;
  msgParams.msgID = whichMsg;
  msgParams.dispInstructions = whichInst;

  ES_Timer_StopTimer(SCROLL_MESSAGE_TIMER);

  MessageEvent.EventParam = msgParams.fullParam;
  PostLEDDisplayService(MessageEvent);
}

void readPot(void) {
  uint32_t adcResults[1];
  ADC_MultiRead(adcResults);
  knobAnalogReadVal = adcResults[0];
  difficultyKnobVal = knobAnalogReadVal * 99 / 1023 + 1;
  gameDifficulty = (adcResults[0] * 4) / 1000 + 1;
  DB_printf("\nAnalog Val: %d:    ", adcResults[0]);
  DB_printf("Difficulty: %d\n", gameDifficulty);
}

void setLaunchRocket() {
  //currentMessage = "LAUNCH ROCKET!  ";
  //SendMessage(MSG_CUSTOM, SCROLL_REPEAT_SLOW);
  CurrentState = LaunchRocket;

  ES_Event_t NewEvent;
  NewEvent.EventType = ES_RESET_GAME_TIMER;
  PostTimerServoFSM(NewEvent);

  NewEvent.EventType = ES_ROCKET_SERVO_HEIGHT;
  if (totalScore >= ALTITUDE_4_SCORE) {
    NewEvent.EventParam = 3;
  } else if (totalScore >= ALTITUDE_3_SCORE) {
    NewEvent.EventParam = 2;
  } else if (totalScore >= ALTITUDE_2_SCORE) {
    NewEvent.EventParam = 1;
  } else {
    NewEvent.EventParam = 0;
  }
  PostRocketHeightServos(NewEvent);

  ES_Timer_InitTimer(HOLD_MESSAGE_TIMER, 100);
}

void setGameOver() {
  ES_Event_t NewEvent;
  NewEvent.EventType = ES_ROCKET_RELEASE_SERVO_LAUNCH;
  PostRocketReleaseServo(NewEvent);

  sprintf(customBuffer, "LIFTOFF!  Total Score: %d", (uint32_t) totalScore);
  currentMessage = customBuffer;
  SendMessage(MSG_CUSTOM, SCROLL_ONCE_SLOW);
  CurrentState = GameOver;

  DB_printf("\n Game over! Total Score: %d \n", (uint32_t) totalScore);
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