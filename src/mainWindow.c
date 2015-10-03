#include <pebble.h>
#include "mainWindow.h"



static Layer* s_mainWindowLayer = NULL;
static Layer* s_boardLayer = NULL;
static StatusBarLayer* s_statusBar = NULL;

static GPoint s_cursor = {0,0};
static GPoint s_motionCursor = {0,0};

#define MS_IN_SEC 1000
#define ANIM_FPS 25
#define ANIM_FRAMES ANIM_FPS*ANIM_DURATION
#define ANIM_DELAY MS_IN_SEC/ANIM_FPS
#define GRAVITY 17
static int s_frame = 0;
//static int s_animateFrames = 0;

#define SUB_PIXEL 100

#define BOARD_PIECES_X 9
#if defined(PBL_RECT)
  #define BOARD_PIECES_Y 9
#elif defined(PBL_ROUND)
  #define BOARD_PIECES_Y 8
#endif
#define PIECE_PIXELS 15
#define PIECE_SUB_PIXELS PIECE_PIXELS*SUB_PIXEL
#define BOARD_PIECES BOARD_PIECES_X*BOARD_PIECES_Y
#define BOARD_SIZE_X BOARD_PIECES_X*PIECE_PIXELS
#define BOARD_SIZE_Y BOARD_PIECES_Y*PIECE_PIXELS

// max is 7
#define N_PRIMARY_COLOURS 4
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
  kSettleBoard
} GameState_t;
GameState_t s_gameState = kIdle;

typedef enum {
  kRow,
  kColumn,
  kCross,
  kBOOM,
  kColourBoom,
  kMiniBoom
} ExplosionType_t;

#define N_CARDINAL 4
typedef enum {
  kN,
  kE,
  kS,
  kW
} Cardinal_t;

static GPath* s_shapes[N_COLOURS] = {NULL};
static GPath* s_arrows[N_CARDINAL] = {NULL};

static const GPathInfo shapeRed = {
  .num_points = 7,
  .points = (GPoint []) {{2, 12}, {13, 13}, {11, 2}, {9, 7}, {7, 2}, {5, 9}, {3, 2}}
};

static const GPathInfo shapePink = {
  .num_points = 10,
  .points = (GPoint []) {{4, 13}, {11, 13}, {13, 8}, {13, 6}, {9, 2}, {6, 3}, {8, 6}, {7, 8}, {4, 5}, {2, 7}}
};

static const GPathInfo shapeOrange = {
  .num_points = 8,
  .points = (GPoint []) {{2, 7}, {6, 5}, {8, 2}, {10, 6}, {13, 8}, {10, 9}, {7, 13}, {6, 9}}
};

static const GPathInfo shapeGreen = {
  .num_points = 5,
  .points = (GPoint []) {{2, 3}, {5, 13}, {7, 13}, {13, 9}, {11, 2}}
};

static const GPathInfo shapeYellow = {
  .num_points = 4,
  .points = (GPoint []) {{2, 10}, {11, 2}, {13, 6}, {4, 13}}
};

static const GPathInfo shapeBlue = {
  .num_points = 5,
  .points = (GPoint []) {{2, 2}, {7, 6}, {13, 3}, {9, 13}, {5, 11}}
};

static const GPathInfo shapePurple = {
  .num_points = 3,
  .points = (GPoint []) {{2, 2}, {13, 10}, {6, 13}}
};

static const GPathInfo shapeWhite = {
  .num_points = 8,
  .points = (GPoint []) {{5, 2}, {10, 2}, {13, 5}, {13, 10}, {10, 13}, {5, 13}, {2, 10}, {2, 5}}
};

static const GPathInfo shapeBlack = {
  .num_points = 14,
  .points = (GPoint []) {{2, 2}, {5, 5}, {7, 2}, {8, 7}, {11, 2}, {13, 7}, {11, 11}, {13, 13}, {10, 13}, {8, 11}, {4, 13}, {2, 11}, {4, 8}, {2, 5}}
};

static const GPathInfo shapeN = {
  .num_points = 4,
  .points = (GPoint []) {{0, -1}, {15, -1}, {9, -8}, {6, -8}}
};

static const GPathInfo shapeE = {
  .num_points = 4,
  .points = (GPoint []) {{16, 0}, {16, 15}, {23, 9}, {23, 6}}
};

static const GPathInfo shapeS = {
  .num_points = 4,
  .points = (GPoint []) {{0, 16}, {15, 16}, {9, 23}, {6, 23}}
};

static const GPathInfo shapeW = {
  .num_points = 4,
  .points = (GPoint []) {{-1, 0}, {-1, 15}, {-8, 9}, {-8, 6}}
};

typedef struct {
  Colour_t colour;
  GPoint loc;
  MatchState_t match;
  Colour_t promoteFlag;
  int v;
} Piece_t;
Piece_t s_pieces[BOARD_PIECES_X * BOARD_PIECES_Y];

typedef struct {
  GPoint first;
  GPoint second;
} Switch_t;
Switch_t s_switch;

int XY(int x, int y) { return x + (BOARD_PIECES_X * y); }

int XYp(GPoint p) { return XY(p.x,p.y); }

void redraw() {
  layer_mark_dirty(s_mainWindowLayer);
};


bool checkTriplet(Colour_t a, Colour_t b, Colour_t switchColour) {
    if (a == kNONE || b == kNONE || switchColour == kNONE) return false;
    if (switchColour == kBlack) { APP_LOG(APP_LOG_LEVEL_DEBUG, "Y-black"); return true; }// Always valid

    int nWhite = 0;
    if (a == kWhite) ++nWhite;
    if (b == kWhite) ++nWhite;
    if (switchColour == kWhite) ++nWhite;

    if (nWhite > 1) {
      return true; //must be WcW for WWc or cWW will auto-fire
    } else if (nWhite == 1) {
      if (a == b || b == switchColour || switchColour == a) return true; // White fills the gap
    } else {
      if (a == b && b  == switchColour) return true;  // No white, all three must match
    }

    return false;
}

void switchColours(GPoint a, GPoint b) {
  Colour_t temp = s_pieces[ XYp(a) ].colour;
  s_pieces[ XYp(a) ].colour = s_pieces[ XYp(b) ].colour;
  s_pieces[ XYp(b) ].colour = temp;
}

void switchPieces() {
  Piece_t temp = s_pieces[ XYp(s_switch.first) ];
  s_pieces[ XYp(s_switch.first) ] = s_pieces[ XYp(s_switch.second) ];
  s_pieces[ XYp(s_switch.second) ] = temp;
}

// See if the given location has a match
bool checkLocation(GPoint location) {
  for (int check=0; check < 6; ++check) {
    int xA = 0, xB = 0, yA = 0, yB = 0;
    if        (check == 0 && location.x >= 2) { // X -ve
      xA = -2; xB = -1;
    } else if (check == 1 && location.x >= 1 && location.x <= BOARD_PIECES_X-2) { // X mid
      xA = -1; xB = 1;
    } else if (check == 2  && location.x <= BOARD_PIECES_X-3) { // X +ve
      xA = 1; xB = 2;
    } else if (check == 3 && location.y >= 2) { // Y -ve
      yA = -2; yB = -1;
    } else if (check == 4 && location.y >= 1 && location.y <= BOARD_PIECES_Y-2) { // Y mid
      yA = -1; yB = 1;
    } else if (check == 5 && location.y <= BOARD_PIECES_Y-3) { // Y +ve
      yA = 1; yB = 2;
    } else {
      continue;
    }

    APP_LOG(APP_LOG_LEVEL_DEBUG, "Check #%i", check);
    bool isValid = checkTriplet( s_pieces[XY(location.x+xA, location.y+yA)].colour,
      s_pieces[XY(location.x+xB, location.y+yB)].colour,
      s_pieces[XYp(location)].colour);
    if (isValid) return true;
  }
  return false;
}

// Need to look two away on either side 0000CCSCC0000. 0=ignore, C=check, S=swap
void checkMove() {

  // we switch the colours temporarily
  switchColours(s_switch.first, s_switch.second);
  bool isValid = false;

  for (int p=0; p<2; ++p) {
    // Look in location A with location B's colour and visa versa
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Check first/second=%i", p);
    if (p == 0) isValid = checkLocation(s_switch.first);
    if (p == 1) isValid = checkLocation(s_switch.second);
    if (isValid) break;
  }

  if (isValid) s_gameState = kSwapAnimate;
  else s_gameState = kNudgeAnimate;

  // switch back! we will animate the actual pieces into their new positions
  switchColours(s_switch.first, s_switch.second);
}

void updateSwitchPiecePhysics(int v) {
  if (s_switch.first.x > s_switch.second.x){
    s_pieces[XYp(s_switch.first)].loc.x  -= v;
    s_pieces[XYp(s_switch.second)].loc.x += v;
  } else if (s_switch.second.x > s_switch.first.x) {
    s_pieces[XYp(s_switch.first)].loc.x  += v;
    s_pieces[XYp(s_switch.second)].loc.x -= v;
  } else if (s_switch.first.y > s_switch.second.y){
    s_pieces[XYp(s_switch.first)].loc.y  -= v;
    s_pieces[XYp(s_switch.second)].loc.y += v;
  } else{
    s_pieces[XYp(s_switch.first)].loc.y  += v;
    s_pieces[XYp(s_switch.second)].loc.y -= v;
  }
}

void nudgeAnimate() {
  static int v = 0;
  static int mode = 0;

  if (mode == 0 && v >= 50) mode = 1;
  else if (mode == 1 && v <= -50) mode = 2;
  else if (mode == 2 && v >= 0) mode = 3;

  if (mode == 0 || mode == 2) v += GRAVITY;
  else if (mode == 1) v -= GRAVITY;

  updateSwitchPiecePhysics(v);

  if (mode == 3) {
    mode = 0;
    v = 0;
    s_gameState = kIdle;
  }

  redraw();
}

void swapAnimate() {
  static int v = 0, mode = 0, travel = 0;
  static GPoint firstStart;
  static GPoint secondStart;

  if (mode == 0) {
    firstStart = s_pieces[ XYp(s_switch.first) ].loc;
    secondStart = s_pieces[ XYp(s_switch.second) ].loc;
    mode = 1;
    travel = 0;
    v = 0;
  } else if (mode == 1) {
    v += GRAVITY*2;
    travel += v;
    updateSwitchPiecePhysics(v); //why-?
    if (travel / SUB_PIXEL >= PIECE_PIXELS/2)  {
      mode = 2;
      travel = 0;
    }
  } else if (mode == 2) {
    travel += v;
    v -= GRAVITY*2;
    updateSwitchPiecePhysics(v);
    if (travel / SUB_PIXEL >= PIECE_PIXELS/2)  {
      mode = 0;
      s_gameState = kFindMatches;
      // The animation leaves the pices in around the right location, but not exact! Snap them back to the grid
      //APP_LOG(APP_LOG_LEVEL_DEBUG,"start %i %i fin %i %i", firstStart.x, firstStart.y, s_pieces[ XYp(s_switch.second) ].loc.x, s_pieces[ XYp(s_switch.second) ].loc.y);
      s_pieces[ XYp(s_switch.first) ].loc = secondStart; // Make switch sub-pixel perfect
      s_pieces[ XYp(s_switch.second) ].loc = firstStart;
      switchPieces(); // Do a full copy
      //APP_LOG(APP_LOG_LEVEL_DEBUG,"start %i %i fin %i %i", firstStart.x, firstStart.y, s_pieces[ XYp(s_switch.second) ].loc.x, s_pieces[ XYp(s_switch.second) ].loc.y);
    }
  }

  redraw();
}

// Return true if the coordinates point to a switched piece
bool isSwitch(int x, int y) {
  if (s_switch.first.x  == -1) return false;
  if (s_switch.first.x  == x && s_switch.first.y == y) return true;
  if (s_switch.second.x == x && s_switch.second.y == y) return true;
  return false;
}

// Explosion always takes preference at the moment. May rebalance?
void explode(ExplosionType_t dir, int x, int y, Colour_t c) {
  if (dir == kRow) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "EXPLODE ROW %i", y);
    for (int X = 0; X < BOARD_PIECES_X; ++X) s_pieces[XY(X,y)].match = kExploded;
  } else if (dir == kColumn){
    APP_LOG(APP_LOG_LEVEL_DEBUG, "EXPLODE COLUMN %i", x);
    for (int Y = 0; Y < BOARD_PIECES_Y; ++Y) s_pieces[XY(x,Y)].match = kExploded;
  } else if (dir == kCross){
    APP_LOG(APP_LOG_LEVEL_DEBUG, "EXPLODE CROSS %i %i", x, y);
    for (int Y = 0; Y < BOARD_PIECES_Y; ++Y) s_pieces[XY(x,Y)].match = kExploded;
    for (int X = 0; X < BOARD_PIECES_X; ++X) s_pieces[XY(0,y)].match = kExploded;
  } else if (dir == kBOOM) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "EXPLODE BIG %i %i", x, y);
    for (int X = x-2; X <= x+2; ++X) {
      for (int Y = y-2; Y <= y+2; ++Y) {
        if (X < 0 || X >= BOARD_PIECES_X) continue;
        if (Y < 0 || Y >= BOARD_PIECES_Y) continue;
        if (X == x-2 && Y == y-2) continue;
        if (X == x+2 && Y == y+2) continue;
        if (X == x-2 && Y == y+2) continue;
        if (X == x+2 && Y == y-2) continue;
        s_pieces[XY(X,Y)].match = kExploded;
      }
    }
  } else if (dir == kColourBoom) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "EXPLODE COLOUR %i", c);
    for (int X = 0; X < BOARD_PIECES_X; ++X) {
      for (int Y = 0; Y < BOARD_PIECES_Y; ++Y) {
        if (s_pieces[XY(X,Y)].colour == c) {
          s_pieces[XY(X,Y)].match = kExploded;
        }
      }
    }
  } else if (dir == kMiniBoom) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "EXPLODE SMALL %i %i col:%i", x, y, c);
    for (int X = x-2; X <= x+2; ++X) {
      for (int Y = y-2; Y <= y+2; ++Y) {
        if (X < 0 || X >= BOARD_PIECES_X) continue;
        if (Y < 0 || Y >= BOARD_PIECES_Y) continue;
        if (s_pieces[XY(X,Y)].colour == c) {
          s_pieces[XY(X,Y)].match = kExploded;
        }
      }
    }
  }
}

/**
 * Iterate the game board and markup for found combinations
 **/
void findMatches() {
  // One look for explosions
  APP_LOG(APP_LOG_LEVEL_DEBUG, "F M");

  bool matchesFound = false;


  // Double or singe black? Must be a switch
  if (s_switch.first.x != -1) {
    // double black
    if (s_pieces[XYp(s_switch.first)].colour == kBlack && s_pieces[XYp(s_switch.second)].colour == kBlack) {
      explode(kBOOM, s_switch.second.x, s_switch.second.y, 0);
      matchesFound = true;
    } else if (s_pieces[XYp(s_switch.first)].colour == kWhite && s_pieces[XYp(s_switch.second)].colour == kBlack) {
      explode(kCross, s_switch.second.x, s_switch.second.y, 0);
      matchesFound = true;
    } else if (s_pieces[XYp(s_switch.first)].colour == kBlack && s_pieces[XYp(s_switch.second)].colour == kWhite) {
      explode(kCross, s_switch.second.x, s_switch.second.y, 0);
      matchesFound = true;
    } else if (s_pieces[XYp(s_switch.first)].colour == kBlack) { // Single black A
      explode(kMiniBoom, s_switch.second.x, s_switch.second.y, s_pieces[XYp(s_switch.second)].colour);
      s_pieces[XYp(s_switch.first)].match = kExploded;
      matchesFound = true;
    } else if (s_pieces[XYp(s_switch.second)].colour == kBlack) { // Single black B
      explode(kMiniBoom, s_switch.first.x, s_switch.first.y, s_pieces[XYp(s_switch.first)].colour);
      s_pieces[XYp(s_switch.second)].match = kExploded;
      matchesFound = true;
    }
  }

  // // Double white?
  // for (int x=0; x < BOARD_PIECES_X; ++x) {
  //   for (int y=0; y < BOARD_PIECES_Y; ++y) { // Loop to third from bottom
  //     if (s_pieces[XY(x,y)].colour != kWhite) continue;
  //     if      (x+1 < BOARD_PIECES_X && s_pieces[XY(x+1,y)].colour == kWhite) { matchesFound = true; explode(kColumn,0,y,0); }
  //     else if (x-1 > 0              && s_pieces[XY(x-1,y)].colour == kWhite) { matchesFound = true; explode(kColumn,0,y,0); }
  //     else if (y+1 < BOARD_PIECES_Y && s_pieces[XY(x,y+1)].colour == kWhite) { matchesFound = true; explode(kRow,x,0,0); }
  //     else if (y-1 > 0              && s_pieces[XY(x,y-1)].colour == kWhite) { matchesFound = true; explode(kRow,x,0,0); }
  //   }
  // }
  //



  for (int dir = 0; dir < 2; ++dir) {

    int xStop = 0, yStop = 0, max = 0;
    if (dir == 0) {
      yStop = 2;
      max = BOARD_PIECES_Y;
    } else {
      xStop = 2;
      max = BOARD_PIECES_X;
    }

    for (int x=0; x < BOARD_PIECES_X - xStop; ++x) {
      for (int y=0; y < BOARD_PIECES_Y - yStop; ++y) {
        if (s_pieces[XY(x,y)].match == kExploded) continue; // Don't check if s'ploded

        // Find a run
        int runSize = 1, nextX, nextY;
        if (dir == 0) {
          nextX = x;
          nextY = y+1;
        } else {
          nextX = x+1;
          nextY = y;
        }

        // TODO what if first is white? Can we fix this by taking the next colour?
        Colour_t runColour = s_pieces[XY(x,y)].colour;

        while (1) {
          if ( s_pieces[XY(x,y)].match == kExploded ) break;
          if (runColour == kWhite) runColour = s_pieces[XY(nextX,nextY)].colour;

          else if ( s_pieces[XY(nextX,nextY)].colour == runColour) {} // Progress Same colour
          else if ( s_pieces[XY(nextX,nextY)].colour == kWhite ) {} // Progress Wildcard
          else break;

          ++runSize;
          if (dir == 0) ++nextY;
          else          ++nextX;
          if (nextX == max || nextY == max) break;
        }

        if (runSize > 4) {
          // Find colour
          explode(kColourBoom, 0, 0, runColour);
          matchesFound = true;
        } else if (runSize > 2) {
          APP_LOG(APP_LOG_LEVEL_DEBUG,"MATCH-3");
          if  (dir == 0) for (int P=y; P < nextY; ++P) s_pieces[XY(x,P)].match++; // A piece can be matched up to twice
          else           for (int P=x; P < nextX; ++P) s_pieces[XY(P,y)].match++;
          matchesFound = true;
        }

        //Promote white
        if (runSize == 4) {
          APP_LOG(APP_LOG_LEVEL_DEBUG,"PROMOTE WHITE");
          if  (dir == 0) {
            for (int P=y; P < nextY; ++P) {
              if (isSwitch(x,P) || P == nextY-1) {
                s_pieces[XY(x,P)].promoteFlag = kWhite;
                break;
              }
            }
          } else {
            for (int P=x; P < nextX; ++P) {
              if (isSwitch(P,y) || P == nextX-1) {
                s_pieces[XY(P,y)].promoteFlag = kWhite;
                break;
              }
            }
          }
        }

        if      (dir == 0 && runSize > 2) y += (runSize-1);
        else if (dir == 1 && runSize > 2) x += (runSize-1);
      }
    }
  }


  // Promote black
  for (int x=0; x < BOARD_PIECES_X; ++x) {
    for (int y=0; y < BOARD_PIECES_Y; ++y) {
      if (s_pieces[XY(x,y)].match == kMatchedTwice) {
        s_pieces[XY(x,y)].promoteFlag = kBlack;
        APP_LOG(APP_LOG_LEVEL_DEBUG,"PROMOTE BLACK");
      }
    }
  }

  //remember to unset switches so they don't trigger on iterations of this phase
  s_switch.first = GPoint(-1,-1);
  s_switch.second = GPoint(-1,-1);

  if (matchesFound == true) s_gameState = kFlashRemoved;
  else                      s_gameState = kIdle;
}

void flashRemoved() {
  static int count = 0;
  if (++count > ANIM_FPS/2) {
    count = 0;
    s_gameState = kRemoveAndReplace;
  }
  redraw();
}


void settleBoard() {
  bool settled = true;
  for (int x=0; x < BOARD_PIECES_X; ++x) {
    for (int y=0; y < BOARD_PIECES_Y; ++y) {
      int floor = y * PIECE_PIXELS * SUB_PIXEL;
      if (s_pieces[XY(x,y)].loc.y < floor) {
        s_pieces[XY(x,y)].v += GRAVITY;
        s_pieces[XY(x,y)].loc.y += s_pieces[XY(x,y)].v;
        settled = false;
      } else {
        s_pieces[XY(x,y)].v = 0;
        s_pieces[XY(x,y)].loc.y = floor;
      }
    }
  }

  if (settled) s_gameState = kFindMatches;
  redraw();
}


/**
 * Remove matched pieces, fill in.
 **/
void removeAndReplace() {
  // REMOVE
  for (int x=0; x < BOARD_PIECES_X; ++x) {
    for (int y=BOARD_PIECES_Y-1; y >= 0; --y) {
      if      (s_pieces[XY(x,y)].promoteFlag != kNONE)      s_pieces[XY(x,y)].colour = s_pieces[XY(x,y)].promoteFlag;
      else if (s_pieces[XY(x,y)].match       != kUnmatched) s_pieces[XY(x,y)].colour = kNONE;
      s_pieces[XY(x,y)].promoteFlag = kNONE;
      s_pieces[XY(x,y)].match = kUnmatched;
    }
  }
  // REPLACE
  for (int x=0; x < BOARD_PIECES_X; ++x) {
    int newPieces = 0;
    for (int y = BOARD_PIECES_Y - 1; y >= 0; --y) {
      if (s_pieces[XY(x,y)].colour != kNONE) continue;
      bool pieceMoved = false;
      for (int Y = y-1; Y >= 0; --Y) {
        if (s_pieces[XY(x,Y)].colour == kNONE) continue;
        s_pieces[XY(x,y)] = s_pieces[XY(x,Y)];
        s_pieces[XY(x,Y)].colour = kNONE; // Set to cascade
        pieceMoved = true;
        break;
      }

      if (pieceMoved == false) {
        // Generate a new piece off the top
        s_pieces[XY(x,y)].loc.y =  -(++newPieces * PIECE_SUB_PIXELS);
        s_pieces[XY(x,y)].colour = (rand() % N_PRIMARY_COLOURS) + 1;
      }
    }
  }
  APP_LOG(APP_LOG_LEVEL_DEBUG,"END MOVE");
  s_gameState = kSettleBoard;
}

void gameLoop(void* data) {
  if (++s_frame == ANIM_FPS) s_frame = 0;

  switch (s_gameState) {
    case kIdle: break;
    case kAwaitingDirection: if (s_frame == 0 || s_frame == ANIM_FPS/2) redraw(); break;
    case kCheckMove: checkMove(); break;
    case kNudgeAnimate: nudgeAnimate(); break;
    case kSwapAnimate: swapAnimate(); break;
    case kFindMatches: findMatches(); break;
    case kFlashRemoved: flashRemoved(); break;
    case kRemoveAndReplace: removeAndReplace(); break;
    case kSettleBoard: settleBoard(); break;
    default: break;
  }
  app_timer_register(ANIM_DELAY, gameLoop, NULL);
}


void newGame() {
  // Zero data store
  memset(&s_pieces, 0, BOARD_PIECES_X * BOARD_PIECES_Y * sizeof(Piece_t));
  // Fill with colour
  int offset = BOARD_SIZE_Y * SUB_PIXEL;
  for (int y = BOARD_PIECES_Y-1; y >= 0; --y) {
    for (int x = 0; x < BOARD_PIECES_X; ++x) {
      bool placing = true;
      while (placing) {
        s_pieces[ XY(x,y) ].colour = (rand() % N_PRIMARY_COLOURS) + 1;
        placing = checkLocation(GPoint(x,y)); // if match found then we're still placing as we need to try again
      }
      s_pieces[ XY(x,y) ].loc.x = x * PIECE_SUB_PIXELS; // Set location
      s_pieces[ XY(x,y) ].loc.y = (y * PIECE_SUB_PIXELS) - offset;// - (rand() % PIECE_SUB_PIXELS);
    }
    offset += PIECE_SUB_PIXELS;// + (rand() % PIECE_SUB_PIXELS);
  }
  s_gameState = kSettleBoard;
}

void mainWindowClickHandler(ClickRecognizerRef recognizer, void *context) {
  ButtonId button = click_recognizer_get_button_id(recognizer);

  if (s_gameState == kAwaitingDirection) {

    s_switch.first = s_cursor;
    if        (BUTTON_ID_UP     == button && s_cursor.y != 0) {
      s_switch.second = GPoint(s_cursor.x, s_cursor.y - 1);
    } else if (BUTTON_ID_SELECT == button && s_cursor.x != BOARD_PIECES_X - 1) {
      s_switch.second = GPoint(s_cursor.x + 1, s_cursor.y);
    } else if (BUTTON_ID_DOWN   == button && s_cursor.y != BOARD_PIECES_Y - 1) {
      s_switch.second = GPoint(s_cursor.x, s_cursor.y + 1);
    } else if (BUTTON_ID_BACK   == button && s_cursor.x != 0) {
      s_switch.second = GPoint(s_cursor.x - 1, s_cursor.y);
    } else {
      APP_LOG(APP_LOG_LEVEL_DEBUG,"SWITCH DIRn INVALID");
      s_gameState = kIdle; // Invalid
      redraw();
      return;
    }
    s_gameState = kCheckMove;

  } else {

    if (BUTTON_ID_UP == button) --s_cursor.y;
    else if (BUTTON_ID_SELECT == button) ++s_cursor.x;
    else if (BUTTON_ID_DOWN == button && s_gameState == kIdle) s_gameState = kAwaitingDirection;
    else if (BUTTON_ID_BACK == button) newGame();
    if (s_cursor.x >= BOARD_PIECES_X) s_cursor.x = 0;
    if (s_cursor.y < 0) s_cursor.y = BOARD_PIECES_Y - 1;
    redraw();

  }

}

void mainWindowClickConfigProvider(Window *window) {
  window_single_repeating_click_subscribe(BUTTON_ID_SELECT, 100, mainWindowClickHandler);
  window_single_repeating_click_subscribe(BUTTON_ID_UP, 100, mainWindowClickHandler);
  window_single_click_subscribe(BUTTON_ID_DOWN, mainWindowClickHandler);
  window_single_click_subscribe(BUTTON_ID_BACK, mainWindowClickHandler);
}

static void mainWindowUpdateProc(Layer *this_layer, GContext *ctx) {

}

static void boardUpdateProc(Layer *this_layer, GContext *ctx) {
  graphics_context_set_antialiased(ctx, 1);

  // Fill back
  graphics_context_set_fill_color(ctx, GColorLightGray);
  graphics_fill_rect(ctx, GRect(0, 0, BOARD_SIZE_X, BOARD_SIZE_Y), 0, GCornersAll);

  // Fill highight
  graphics_context_set_fill_color(ctx, GColorDarkGray);
  graphics_fill_rect(ctx, GRect(s_cursor.x * PIECE_PIXELS, 0, PIECE_PIXELS, BOARD_SIZE_Y), 0, GCornersAll);
  graphics_fill_rect(ctx, GRect(0, s_cursor.y * PIECE_PIXELS, BOARD_SIZE_X, PIECE_PIXELS), 0, GCornersAll);

  graphics_context_set_stroke_color(ctx, GColorBlack);
  // Draw frame
  for (int x = 0; x < BOARD_PIECES_X; ++x) {
    if (x%2 == 1 && x != BOARD_PIECES_X - 1) continue;
    graphics_draw_rect(ctx, GRect(x*PIECE_PIXELS, 0, PIECE_PIXELS+1, BOARD_SIZE_Y+1));
  }
  for (int y = 0; y < BOARD_PIECES_Y; ++y) {
    if (y%2 == 1 && y != BOARD_PIECES_Y - 1) continue;
    graphics_draw_rect(ctx, GRect(0, y*PIECE_PIXELS, BOARD_SIZE_X+1, PIECE_PIXELS+1));
  }

  // Frame highlight
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_draw_rect(ctx, GRect(s_cursor.x*PIECE_PIXELS, s_cursor.y*PIECE_PIXELS, PIECE_PIXELS+1, PIECE_PIXELS+1));

  // Fill shapes
  for (int x = 0; x < BOARD_PIECES_X; ++x) {
    for (int y = 0; y < BOARD_PIECES_Y; ++y) {
      graphics_context_set_stroke_color(ctx, GColorBlack);
      int xy = XY(x,y);
      if (s_gameState == kFlashRemoved && s_pieces[xy].match != kUnmatched) {
        GColor highlight;
        if      (s_pieces[xy].match == kMatchedOnce)  highlight = GColorYellow;
        else if (s_pieces[xy].match == kMatchedTwice) highlight = GColorOrange;
        else if (s_pieces[xy].match == kExploded)     highlight = GColorRed;
        graphics_context_set_fill_color(ctx, highlight);
        graphics_fill_rect(ctx, GRect((s_pieces[xy].loc.x/SUB_PIXEL)+1, (s_pieces[xy].loc.y/SUB_PIXEL)+1, PIECE_PIXELS-1, PIECE_PIXELS-1), 0, GCornersAll);
      }
      switch (s_pieces[xy].colour) {
        case kRed: graphics_context_set_fill_color(ctx, GColorRed); break;
        case kYellow: graphics_context_set_fill_color(ctx, GColorYellow); break;
        case kBlue: graphics_context_set_fill_color(ctx, GColorElectricUltramarine); break;
        case kGreen: graphics_context_set_fill_color(ctx, GColorGreen); break;
        case kPurple: graphics_context_set_fill_color(ctx, GColorCeleste); break;
        case kOrange: graphics_context_set_fill_color(ctx, GColorChromeYellow); break;
        case kPink: graphics_context_set_fill_color(ctx, GColorFashionMagenta); break;
        case kWhite: graphics_context_set_fill_color(ctx, GColorWhite); break;
        case kBlack: graphics_context_set_fill_color(ctx, GColorBlack); break;
        default: continue;
      }
      if (s_shapes[ s_pieces[xy].colour ] != NULL) {
        gpath_move_to(s_shapes[ s_pieces[xy].colour ], GPoint(s_pieces[xy].loc.x/SUB_PIXEL, s_pieces[xy].loc.y/SUB_PIXEL));
        gpath_draw_filled(ctx, s_shapes[ s_pieces[xy].colour ]);
        gpath_draw_outline(ctx, s_shapes[ s_pieces[xy].colour ]);
      } else {
        graphics_fill_rect(ctx, GRect((s_pieces[xy].loc.x/SUB_PIXEL)+2, (s_pieces[xy].loc.y/SUB_PIXEL)+2, PIECE_PIXELS-3, PIECE_PIXELS-3), 2, GCornersAll);
      }
    }
  }

  // Move Arrows
  if (s_frame < ANIM_FPS/2 && s_gameState == kAwaitingDirection) {
    graphics_context_set_fill_color(ctx, GColorWhite);
    for (int d = 0; d < N_CARDINAL; ++d) {
      gpath_move_to(s_arrows[d], GPoint(s_cursor.x * PIECE_PIXELS, s_cursor.y * PIECE_PIXELS));
      gpath_draw_filled(ctx, s_arrows[d]);
      gpath_draw_outline(ctx, s_arrows[d]);
    }
  }

  // Cursor
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, s_motionCursor, 5);
  graphics_draw_circle(ctx, s_motionCursor, 5);


  // Redo border
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_draw_rect(ctx, GRect(0, 0, BOARD_SIZE_X+1, BOARD_SIZE_Y+1));

}

static void data_handler(AccelData* data, uint32_t num_samples) {

  // Update
  s_motionCursor.x += data[0].x;
  s_motionCursor.y += data[0].y;

  if (s_motionCursor.x < 0) s_motionCursor.x += BOARD_SIZE_X * SUB_PIXEL;
  else if (s_motionCursor.x >= BOARD_SIZE_X * SUB_PIXEL) s_motionCursor.x -= BOARD_SIZE_X * SUB_PIXEL;

  if (s_motionCursor.y < 0) s_motionCursor.y += BOARD_SIZE_Y * SUB_PIXEL;
  else if (s_motionCursor.y >= BOARD_SIZE_Y * SUB_PIXEL) s_motionCursor.x -= BOARD_SIZE_Y * SUB_PIXEL;

}

void mainWindowLoad(Window* parentWindow) {
  GRect b = layer_get_bounds( window_get_root_layer(parentWindow) );

  s_statusBar = status_bar_layer_create();
  layer_add_child(window_get_root_layer(parentWindow), status_bar_layer_get_layer(s_statusBar));
  status_bar_layer_set_separator_mode(s_statusBar, StatusBarLayerSeparatorModeDotted);

  s_mainWindowLayer = layer_create( GRect(0, STATUS_BAR_LAYER_HEIGHT, b.size.w, b.size.h - STATUS_BAR_LAYER_HEIGHT) );
  layer_add_child(window_get_root_layer(parentWindow), s_mainWindowLayer);
  layer_set_update_proc(s_mainWindowLayer, mainWindowUpdateProc);

  int disp = (b.size.w - BOARD_SIZE_X) / 2;

  s_boardLayer = layer_create( GRect(disp, disp, BOARD_SIZE_X + 1, BOARD_SIZE_Y + 1) );
  layer_add_child(s_mainWindowLayer, s_boardLayer);
  layer_set_update_proc(s_boardLayer, boardUpdateProc);
  layer_set_clips(s_boardLayer, true);

  s_shapes[kRed] = gpath_create(&shapeRed);
  s_shapes[kOrange] = gpath_create(&shapeOrange);
  s_shapes[kGreen] = gpath_create(&shapeGreen);
  s_shapes[kWhite] = gpath_create(&shapeWhite);
  s_shapes[kBlack] = gpath_create(&shapeBlack);
  s_shapes[kPink] = gpath_create(&shapePink);
  s_shapes[kYellow] = gpath_create(&shapeYellow);
  s_shapes[kPurple] = gpath_create(&shapePurple);
  s_shapes[kBlue] = gpath_create(&shapeBlue);

  s_arrows[kN] = gpath_create(&shapeN);
  s_arrows[kE] = gpath_create(&shapeE);
  s_arrows[kS] = gpath_create(&shapeS);
  s_arrows[kW] = gpath_create(&shapeW);

  int num_samples = 1;
  accel_data_service_subscribe(num_samples, data_handler);
  accel_service_set_sampling_rate(ACCEL_SAMPLING_25HZ);

  srand(0);
  newGame();
  gameLoop(NULL);

}


void mainWindowUnload() {
  layer_destroy(s_boardLayer);
  layer_destroy(s_mainWindowLayer);
  status_bar_layer_destroy(s_statusBar);
  s_boardLayer = NULL;
  s_mainWindowLayer = NULL;
  s_statusBar = NULL;
}
