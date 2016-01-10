#pragma once
#include <pebble.h>

#define MS_IN_SEC 1000
#define ANIM_FPS 30
#define ANIM_FRAMES ANIM_FPS*ANIM_DURATION
#define ANIM_DELAY MS_IN_SEC/ANIM_FPS
#define GRAVITY 20
//static int s_animateFrames = 0;

#define SUB_PIXEL 100

#define BOARD_PIECES_X 9
//#if defined(PBL_RECT)
  #define BOARD_PIECES_Y 9
//#elif defined(PBL_ROUND)
//  #define BOARD_PIECES_Y 8
//#endif
#define PIECE_PIXELS 15
#define PIECE_SUB_PIXELS PIECE_PIXELS*SUB_PIXEL
#define BOARD_PIECES BOARD_PIECES_X*BOARD_PIECES_Y
#define BOARD_SIZE_X BOARD_PIECES_X*PIECE_PIXELS
#define BOARD_SIZE_Y BOARD_PIECES_Y*PIECE_PIXELS

// max is 7
#define N_COLOURS 10
typedef enum {
  kNONE,
  kRed,
  kYellow,
  kBlue,
  kGreen,
  kPurple,
  kOrange,
  kPink,
  kWhite,
  kBlack
} Colour_t;

typedef enum {
  kUnmatched,
  kMatchedOnce,
  kMatchedTwice,
  kExploded
} MatchState_t;

typedef enum {
  kIdle,
  kAwaitingDirection,
  kCheckMove,
    kNudgeAnimate,
    kSwapAnimate,
  kFindMatches,
  kFlashRemoved,
  kRemoveAndReplace,
  kSettleBoard,
  kFindNextMove,
  kCheckLives,
  kGameOver
} GameState_t;

typedef enum {
  kWait,
  kNewPoints,
  kApplyPoints,
  kCheckNewLevel
} ScoreState_t;

typedef enum {
  kRow,
  kColumn,
  kCross,
  kBOOM,
  kColourBoom,
  kMiniBoom,
  kMatch3, // Only used in scoring
  kMatch4, // Only used in scoring
  kMatchT // Only used in scoring
} MatchType_t;


#define N_CARDINAL 4
typedef enum {
  kN, kE, kS, kW
} Cardinal_t;

typedef struct {
  Colour_t colour;
  GPoint loc;
  MatchState_t match;
  Colour_t promoteFlag;
  int v;
} Piece_t;

typedef struct {
  GPoint first;
  GPoint second;
} Switch_t;

typedef struct {
  int pointsToNextLevel;
  int level; // how many points are needed to get to next level
  int bestLevel;
  int lives;
  int pointBuffer; // points buffered before awarding
  int points; // points after awarded
  int hintOn;
  int nColoursActive;
} Score_t;

void initGlobals();
void destroyGlobals();

GPath* getShape(int n);
GPath* getArrow(int n);
