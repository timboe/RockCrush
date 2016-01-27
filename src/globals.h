#pragma once
#include <pebble.h>

#define MS_IN_SEC 1000
#define ANIM_FPS 24
#define ANIM_FRAMES ANIM_FPS*ANIM_DURATION
#define ANIM_DELAY MS_IN_SEC/ANIM_FPS
#define GRAVITY 35
//static int s_animateFrames = 0;

#define SUB_PIXEL 100

#define BOARD_PIECES_X 9
#ifdef PBL_RECT
  // Square - 9 deep
  #define BOARD_PIECES_Y 9
#else
  // Round - 8 deep
  #define BOARD_PIECES_Y 9
#endif
#define PIECE_PIXELS 15
#define PIECE_SUB_PIXELS PIECE_PIXELS*SUB_PIXEL
#define BOARD_PIECES BOARD_PIECES_X*BOARD_PIECES_Y
#define BOARD_SIZE_X BOARD_PIECES_X*PIECE_PIXELS
#define BOARD_SIZE_Y BOARD_PIECES_Y*PIECE_PIXELS

#define TEXT_COLOUR_L GColorWindsorTan
#define TEXT_COLOUR_U GColorOrange
#define TEXT_COLOUR_R GColorRajah
#define TEXT_COLOUR_D GColorRajah
#define TEXT_COLOUR_C GColorYellow

// max is 7
#define N_COLOURS 11
typedef enum {
  kNONE,
  kRed,
  kYellow,
  kBlue,
  kGreen,
  kPurple,
  kOrange,
  kPink,
  kBrown,
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
  int pointsToNextLevel; // how many points are needed to get to next level
  int level;
  int lives;
  int pointBuffer; // points buffered before awarding
  int points; // points after awarded
  int hintOn;
  int nColoursActive;
} Score_t;

//extern const char* const COLOUR_TEXT[N_COLOURS];
extern GColor COLOURS[N_COLOURS];

void initGlobals();
void destroyGlobals();

void draw3DText(GContext *ctx, GRect loc, GFont f, const char* buffer, uint8_t offset, GTextAlignment al, bool BWMode, GColor BWFg, GColor BWBg);

GPath* getShape(int n);
GPath* getArrow(int n);
