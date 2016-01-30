#include "mainWindow.h"
#include "globals.h"
#include "persistence.h"
#include "main.h"

//

static GRect s_wave[3];
static int s_waveV = 0;
static GRect s_liquid;
static int s_colourBackground;
static int s_colourFourground;
static int s_nWaves = 0;
static int s_liquidEnd = 0;
static int s_windowSizeY = 0;

static int s_frame = 0;

static int s_tiltMode = 0;

static bool s_hintOn = false;
static bool s_hintStatus = false;

static AppTimer* s_hintTimer = NULL;
static AppTimer* s_gameLoopTime = NULL;

#ifdef DEBUG_MODE
static AppTimer* s_FPSTimer = NULL;
static int s_FPS = 0;
static int s_lastSecondFPS = 0;
#endif

static Layer* s_mainBackgroundLayer = NULL;
static Layer* s_mainForegroundLayer = NULL;
static Layer* s_boardLayer = NULL;
static StatusBarLayer* s_statusBar = NULL;

static GPoint s_cursor = {0,0};
static GPoint s_motionCursor = {400,400};
static GPoint s_availableMove = {-1,-1};

static bool s_flashArrows = false;

//static GPoint s_bombLocation[N_COLOURS];
static int s_spentLifeAnimRadius = 0;
static const Colour_t s_extraLifeBoomColour[3] = {kRed, kYellow, kGreen};

static bool s_gameOverMessage = false;

static Score_t s_score;
static Piece_t s_pieces[BOARD_PIECES];
static Switch_t s_switch;
static int s_currentRun;

static GameState_t s_gameState = kIdle; // Game FSM
static ScoreState_t s_scoreState = kWait;



Score_t* getScore() { return &s_score; };
Piece_t* getPiece() { return &s_pieces[0]; }
int XY(int x, int y) { return x + (BOARD_PIECES_X * y); }
int XYp(GPoint p) { return XY(p.x,p.y); }

static void redraw() {
  layer_mark_dirty(s_mainBackgroundLayer);
};

// One point per square in a match 3
#define SCORE_3 3
// Two points per square in a match 4
#define SCORE_4 6
//Note this is bonus on top of two x SCORE_3's
#define SCORE_T 6
// Two points per square clearing a row/col
#define SCORE_ROW BOARD_PIECES_X
#define SCORE_COLUMN BOARD_PIECES_Y
// Two points _per exploded piece_
#define SCORE_BOOM 5
void score(MatchType_t type, int n) {
  int b4 = s_score.pointBuffer;
  int M = (s_score.level + 1) / 2; // Bonuses are worth more in later levels
  switch (type) {
    case kRow: s_score.pointBuffer += SCORE_COLUMN * M; break;
    case kColumn: s_score.pointBuffer += SCORE_ROW * M; break;
    case kCross: s_score.pointBuffer += (SCORE_ROW+SCORE_COLUMN) * M; break;
    case kBOOM: case kColourBoom: case kMiniBoom: s_score.pointBuffer += SCORE_BOOM * n * M; break;
    case kMatch3: s_score.pointBuffer += SCORE_3 + M; break; // Note - plus sign - nerfed
    case kMatch4: s_score.pointBuffer += SCORE_4 + M; break; // Note - plus sign - nerfed
    case kMatchT: s_score.pointBuffer += SCORE_T * M; break;
  }
  //APP_LOG(APP_LOG_LEVEL_INFO, "points enum %i scored %i", type, s_score.pointBuffer - b4);
}

// Do these three colours form a valid run?
bool checkTriplet(Colour_t a, Colour_t b, Colour_t switchColour) {
  if (a == kNONE || b == kNONE || switchColour == kNONE) return false; // All must be coloured
  if (switchColour == kBlack) { /*APP_LOG(APP_LOG_LEVEL_DEBUG, "Y-black");*/ return true; } // A switch with black is always valid

  int nWhite = 0;
  if (a == kWhite) ++nWhite;
  if (b == kWhite) ++nWhite;
  if (switchColour == kWhite) ++nWhite;

  if (nWhite > 1) {
    return true; //must be WcW for WWc or cWW hence must be true, in fact will auto-fire so this statement should never come into effect
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


// See if the given location has a match, try all the posible ways of getting 3 rocks in a row which includ this one
bool checkLocation(GPoint location) {
  for (int check=0; check < 6; ++check) {
    int xA = 0, xB = 0, yA = 0, yB = 0;
    if        (check == 0 && location.x >= 2) { // X -ve
      xA = -2; xB = -1;
    } else if (check == 1 && location.x >= 1 && location.x <= BOARD_PIECES_X-2) { // X mid
      xA = -1; xB = 1;
    } else if (check == 2 && location.x <= BOARD_PIECES_X-3) { // X +ve
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

    bool isValid = checkTriplet(
      s_pieces[XY(location.x+xA, location.y+yA)].colour,
      s_pieces[XY(location.x+xB, location.y+yB)].colour,
      s_pieces[XYp(location)].colour);
    if (isValid) return true;
  }
  return false;
}

// Function to see if a proposed move results in matches
// Need to look two away on either side 0000CCSCC0000. 0=ignore, C=check, S=swap in both V and H
bool checkMove() {

  // we switch the colours temporarily
  switchColours(s_switch.first, s_switch.second);
  bool isValid = false;

  for (int p=0; p<2; ++p) {
    // Look in location A with location B's colour and visa versa
    if (p == 0) isValid = checkLocation(s_switch.first);
    if (p == 1) isValid = checkLocation(s_switch.second);
    if (isValid) break;
  }

  // switch back! we will animate the actual pieces into their new positions
  switchColours(s_switch.first, s_switch.second);

  // This routine also finds the computer a valid next move
  if (s_gameState == kFindNextMove) return isValid;

  // If it was user initiated we take action with the FSM
  if (isValid == true) {
    s_currentRun = 0;
    s_gameState = kSwapAnimate;
    s_hintOn = false;
    if (s_hintTimer) app_timer_cancel(s_hintTimer);
    s_hintTimer = NULL;
  } else {
    s_gameState = kNudgeAnimate;
  }
  return false; // This return to the FSM says no need to re-draw
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

bool nudgeAnimate() {
  static int v = 0, mode = 0;
  static GPoint firstStart, secondStart;

  if (mode == 0 && v == 0) {
    firstStart = s_pieces[ XYp(s_switch.first) ].loc;
    secondStart = s_pieces[ XYp(s_switch.second) ].loc;
  }

  if (mode == 0 && v >= NUDGE_MAX_SPEED) mode = 1;
  else if (mode == 1 && v <= -NUDGE_MAX_SPEED) mode = 2;
  else if (mode == 2 && v >= 0) mode = 3;

  if (mode == 0 || mode == 2) v += GRAVITY;
  else if (mode == 1) v -= GRAVITY;

  updateSwitchPiecePhysics(v);

  if (mode == 3) {
    mode = 0;
    v = 0;
    s_pieces[ XYp(s_switch.first)  ].loc = firstStart; // Snap back to grid
    s_pieces[ XYp(s_switch.second) ].loc = secondStart;
    s_gameState = kIdle;
  }

  return true; // Redraw
}

bool swapAnimate() {
  static int v = 0, mode = 0, travel = 0;
  static GPoint firstStart, secondStart;

  if (mode == 0) {
    firstStart = s_pieces[ XYp(s_switch.first) ].loc;
    secondStart = s_pieces[ XYp(s_switch.second) ].loc;
    mode = 1;
    travel = 0;
    v = SWAP_STARTING_VELOCITY; // TWEAK starting velocity
  } else if (mode == 1) {
    v += GRAVITY*2;
    travel += v;
    updateSwitchPiecePhysics(v);
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
      s_pieces[ XYp(s_switch.first) ].loc = secondStart; // Make switch sub-pixel perfect
      s_pieces[ XYp(s_switch.second) ].loc = firstStart;
      switchPieces(); // Do a full copy
    }
  }

  return true; // Redraw
}

// Return true if the coordinates point to a switched piece
bool isSwitch(int x, int y) {
  if (s_switch.first.x  == -1) return false;
  if (s_switch.first.x  == x && s_switch.first.y == y) return true;
  if (s_switch.second.x == x && s_switch.second.y == y) return true;
  return false;
}

// Explosion always takes preference at the moment. May rebalance?
void explode(MatchType_t dir, int x, int y, Colour_t c) {
  int n = 0; // number of exploded
  if (dir == kRow) {
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "EXPLODE ROW %i", y);
    for (int X = 0; X < BOARD_PIECES_X; ++X) s_pieces[XY(X,y)].exploded = true;
  } else if (dir == kColumn){
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "EXPLODE COLUMN %i", x);
    for (int Y = 0; Y < BOARD_PIECES_Y; ++Y) s_pieces[XY(x,Y)].exploded = true;
  } else if (dir == kCross){
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "EXPLODE CROSS %i %i", x, y);
    for (int Y = 0; Y < BOARD_PIECES_Y; ++Y) s_pieces[XY(x,Y)].exploded = true;
    for (int X = 0; X < BOARD_PIECES_X; ++X) s_pieces[XY(X,y)].exploded = true;
  } else if (dir == kBOOM) {
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "EXPLODE BIG %i %i", x, y);
    for (int X = x-2; X <= x+2; ++X) {
      for (int Y = y-2; Y <= y+2; ++Y) {
        if (X < 0 || X >= BOARD_PIECES_X) continue;
        if (Y < 0 || Y >= BOARD_PIECES_Y) continue;
        if (X == x-2 && Y == y-2) continue;
        if (X == x+2 && Y == y+2) continue;
        if (X == x-2 && Y == y+2) continue;
        if (X == x+2 && Y == y-2) continue;
        s_pieces[XY(X,Y)].exploded = true;
        ++n;
      }
    }
  } else if (dir == kColourBoom) {
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "EXPLODE COLOUR %i", c);
    for (int X = 0; X < BOARD_PIECES_X; ++X) {
      for (int Y = 0; Y < BOARD_PIECES_Y; ++Y) {
        if (s_pieces[XY(X,Y)].colour == c) {
          s_pieces[XY(X,Y)].exploded = true;
          ++n;
        }
      }
    }
  } else if (dir == kMiniBoom) {
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "EXPLODE SMALL %i %i col:%i", x, y, c);
    for (int X = x-2; X <= x+2; ++X) {
      for (int Y = y-2; Y <= y+2; ++Y) {
        if (X < 0 || X >= BOARD_PIECES_X) continue;
        if (Y < 0 || Y >= BOARD_PIECES_Y) continue;
        if (s_pieces[XY(X,Y)].colour == c) {
          s_pieces[XY(X,Y)].exploded = true;
          ++n;
        }
      }
    }
  }
  score(dir, n);
}

/**
 * Iterate the game board and markup for found combinations
 **/
bool findMatches() {
  // One look for explosions

  bool matchesFound = false;

  // Double or singe black? Must be a switch
  if (s_switch.first.x != -1) {
    if (s_pieces[XYp(s_switch.first)].colour == kBlack && s_pieces[XYp(s_switch.second)].colour == kBlack) { // double black
      explode(kBOOM, s_switch.second.x, s_switch.second.y, 0);
      matchesFound = true;
    } else if (s_pieces[XYp(s_switch.first)].colour == kWhite && s_pieces[XYp(s_switch.second)].colour == kBlack) { //White black
      explode(kCross, s_switch.second.x, s_switch.second.y, 0);
      matchesFound = true;
    } else if (s_pieces[XYp(s_switch.first)].colour == kBlack && s_pieces[XYp(s_switch.second)].colour == kWhite) { //Black white
      explode(kCross, s_switch.second.x, s_switch.second.y, 0);
      matchesFound = true;
    } else if (s_pieces[XYp(s_switch.first)].colour == kBlack) { // Single black A
      explode(kMiniBoom, s_switch.second.x, s_switch.second.y, s_pieces[XYp(s_switch.second)].colour);
      s_pieces[XYp(s_switch.first)].exploded = true;
      matchesFound = true;
    } else if (s_pieces[XYp(s_switch.second)].colour == kBlack) { // Single black B
      explode(kMiniBoom, s_switch.first.x, s_switch.first.y, s_pieces[XYp(s_switch.first)].colour);
      s_pieces[XYp(s_switch.second)].exploded = true;
      matchesFound = true;
    }
  }

  // Markup the board
  for (int dir = 0; dir < 2; ++dir) { // x or y

    int xStop = 0, yStop = 0, max = 0;
    if (dir == 0) { // go along x, look in y
      yStop = 2;
      max = BOARD_PIECES_Y;
    } else { // go along y, look in x
      xStop = 2;
      max = BOARD_PIECES_X;
    }

    for (int x=0; x < BOARD_PIECES_X - xStop; ++x) {
      for (int y=0; y < BOARD_PIECES_Y - yStop; ++y) {

        // Find a run
        int runSize = 1, nextX, nextY;
        if (dir == 0) {
          nextX = x;
          nextY = y+1;
        } else {
          nextX = x+1;
          nextY = y;
        }

        Colour_t runColour = s_pieces[XY(x,y)].colour;
        while (1) {
          if (runColour == kWhite) runColour = s_pieces[XY(nextX,nextY)].colour; // If first colour(s) were white - need to update to know colour of run

          if      ( s_pieces[XY(nextX,nextY)].colour == runColour) {} // Progress - Same colour
          else if ( s_pieces[XY(nextX,nextY)].colour == kWhite ) {} // Progress - Wildcard, white always matches
          else break; // Square is different colour - stop here

          ++runSize; // Square accepted
          if (dir == 0) {
            if (++nextY == max) break; // This is where the asymmetric grid bug was hiding before, only compare one
          } else {
            if (++nextX == max) break;
          }
        }

        if (runSize > 4) {
          explode(kColourBoom, 0, 0, runColour); // do colour BOOM
          matchesFound = true;
        } else if (runSize > 2) {
          if (runColour == kBlack) { // Special - three blacks? Very rare, but deserves a big explosion! Like an extended black-black match
            //APP_LOG(APP_LOG_LEVEL_DEBUG,"MATCH-3-or-4-BLACK!!!!");
            if  (dir == 0) for (int P=y; P < nextY; ++P) explode(kBOOM, x, P, 0);
            else           for (int P=x; P < nextX; ++P) explode(kBOOM, P, y, 0);
            matchesFound = true;
          } else {
            //APP_LOG(APP_LOG_LEVEL_DEBUG,"MATCH-3-or-4");
            runSize == 3 ? score(kMatch3, 0) : score(kMatch4, 0);
            if  (dir == 0) for (int P=y; P < nextY; ++P) s_pieces[XY(x,P)].match++; // A piece can be matched up to twice
            else           for (int P=x; P < nextX; ++P) s_pieces[XY(P,y)].match++;
            matchesFound = true;
          }
        }

        //Promote white
        if (runSize == 4) { // We promote the switched piece if user instagated, or the final piece if part of a cascade
          //APP_LOG(APP_LOG_LEVEL_DEBUG,"PROMOTE WHITE");
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

        // Skip over the cells we've just dealed with (remember we're at the end of the loop so going to get a +1 anyway)
        if      (dir == 0 && runSize > 2) y += (runSize-1);
        else if (dir == 1 && runSize > 2) x += (runSize-1);
      }
    }
  }

  // Promote black
  for (int x=0; x < BOARD_PIECES_X; ++x) {
    for (int y=0; y < BOARD_PIECES_Y; ++y) {
      if (s_pieces[XY(x,y)].match == kMatchedTwice) {
        score(kMatchT, 0); // This is a bonus on top of the 2x match-3 / match-4s
        s_pieces[XY(x,y)].promoteFlag = kBlack; // This can in theory overwrite a promote white command
        //APP_LOG(APP_LOG_LEVEL_DEBUG,"PROMOTE BLACK");
      }
    }
  }

  //remember to unset switches so they don't trigger on iterations of this phase
  s_switch.first = GPoint(-1,-1);
  s_switch.second = GPoint(-1,-1);

  if (matchesFound == true) { // Remove and repeat, continue to cascade until no runs remain
    s_gameState = kFlashRemoved;
    if (s_currentRun < ANIM_FPS/3) ++s_currentRun; // This makes things faster if more matches are found
  } else { // No more runs
     s_gameState = kFindNextMove; // Let the comp try and find a valid move, if any
   }

  return false; // don't redraw, nothing graphical
}

void enableHint(void* data) {
  s_hintOn = true;
  redraw();
  s_hintTimer = NULL;
}

// Find a next legal move
bool findNextMove() {
  bool foundMove = false;
  for (int x=0; x < BOARD_PIECES_X; ++x) {
    for (int y=0; y < BOARD_PIECES_Y; ++y) {
      if (y < BOARD_PIECES_Y - 1) {
        s_switch.first = GPoint(x,y);
        s_switch.second = GPoint(x,y+1);
        foundMove = checkMove();
        if (foundMove == true) break;
      }
      if (x < BOARD_PIECES_X - 1) {
        s_switch.first = GPoint(x,y);
        s_switch.second = GPoint(x+1,y);
        foundMove = checkMove();
        if (foundMove == true) break;
      }
    }
    if (foundMove == true) break;
  }
  if (foundMove == true) { // There is a legal move somewhere on the board, we'll take note of it in case the user is dumb and can't find it on their own
    s_availableMove = s_switch.first;
    s_gameState = kIdle;
    #ifdef AUTO_MODE
    s_gameState = kCheckMove; // This ONE line lets the pebble play automatically, love that
    #endif
    s_hintTimer = app_timer_register(HINT_TIMER, enableHint, NULL); // Pops up a hint in the future
  } else { // Oh dear, there are no legal swaps left on the board. Hope the user has some lives left!
    s_availableMove = GPoint(-1,-1);
    s_gameState = kCheckLives;
  }
  return false; // don't redraw
}

bool flashRemoved() {
  static int count = 0; // The currentRun variable reduces the flash period each match in a cascade until there is no delay
  if (++count > (ANIM_FPS/3 - s_currentRun)) { // The increase in speed is a nice effect
    count = 0;
    s_gameState = kRemoveAndReplace;
  }
  if (count == 1) return true; // Only need to redraw on the first frame to get the red backgrounds rendered
  return false;
}

bool settleBoard() {
  bool settled = true;
  for (int x=0; x < BOARD_PIECES_X; ++x) {
    for (int y=0; y < BOARD_PIECES_Y; ++y) {
      int floor = y * PIECE_PIXELS * SUB_PIXEL;
      if (s_pieces[XY(x,y)].loc.y < floor) { // Still needs to fall
        if (s_pieces[XY(x,y)].v == 0) s_pieces[XY(x,y)].v = PIECE_PIXELS * s_currentRun; // Keep the momentum going with a increasing initial speed
        s_pieces[XY(x,y)].v += GRAVITY;
        s_pieces[XY(x,y)].loc.y += s_pieces[XY(x,y)].v;
        settled = false;
      } else if (s_pieces[XY(x,y)].v > 0) { // Has fallen too far
        s_pieces[XY(x,y)].v = 0;
        s_pieces[XY(x,y)].loc.y = floor;
      } else { // Is in place. As we're searching top-down, everything below must also be in place too.
        break; // Efficiency - break the inner loop early
      }
    }
  }
  if (settled == true) s_gameState = kFindMatches;
  return true; // redraw, we're moving things
}

/**
 * Remove matched pieces, fill in.
 **/
bool removeAndReplace() {
  // REMOVE
  for (int x=0; x < BOARD_PIECES_X; ++x) {
    for (int y=BOARD_PIECES_Y-1; y >= 0; --y) {
      if (s_pieces[XY(x,y)].promoteFlag != kNONE) {
        s_pieces[XY(x,y)].colour = s_pieces[XY(x,y)].promoteFlag;
      } else if (s_pieces[XY(x,y)].match != kUnmatched || s_pieces[XY(x,y)].exploded == true) {
        s_pieces[XY(x,y)].colour = kNONE; // I'm effectivly dead now
      }
      s_pieces[XY(x,y)].promoteFlag = kNONE;
      s_pieces[XY(x,y)].match = kUnmatched;
      s_pieces[XY(x,y)].exploded = false;
    }
  }
  // REPLACE
  for (int x=0; x < BOARD_PIECES_X; ++x) {
    int newPieces = 0; // Per column
    for (int y = BOARD_PIECES_Y - 1; y >= 0; --y) { // Go bottom up, easier to do cascade
      if (s_pieces[XY(x,y)].colour != kNONE) continue;
      bool pieceMoved = false;
      for (int Y = y-1; Y >= 0; --Y) { // Find next rock above me
        if (s_pieces[XY(x,Y)].colour == kNONE) continue;
        s_pieces[XY(x,y)] = s_pieces[XY(x,Y)]; // Move me down
        s_pieces[XY(x,Y)].colour = kNONE; // Set to cascade up
        pieceMoved = true;
        break;
      }

      if (pieceMoved == false) {
        // Generate a new piece off the top
        s_pieces[XY(x,y)].loc.y =  -(++newPieces * PIECE_SUB_PIXELS);
        s_pieces[XY(x,y)].colour = (rand() % s_score.nColoursActive) + 1; // Keep fair mix of all colours in play
      }
    }
  }
  //APP_LOG(APP_LOG_LEVEL_DEBUG,"END MOVE");
  s_gameState = kSettleBoard;
  return false; // don't redraw, settle board does all the animation
}

bool awaitingDirection() {
  if (s_frame == 0) {
    s_flashArrows = true;
    return true; // Flashing cursor only
  } else if (s_frame == ANIM_FPS/2) {
    s_flashArrows = false;
    return true; // Flashing cursor only
  }
  return false;
}

bool checkScoreBuffer() {
  if (s_score.pointBuffer == 0) return false;

  // Do we have a little or a lot of points to award?
  if (s_score.pointBuffer >= s_score.pointsToNextLevel/11) s_nWaves = 3; // > ~9%
  else if (s_score.pointBuffer >= s_score.pointsToNextLevel/17) s_nWaves = 2; // > ~6%
  else if (s_score.pointBuffer >= s_score.pointsToNextLevel/33) s_nWaves = 1; // > ~3 %
  else return false; // Not enough

  s_wave[0].origin.y = (s_windowSizeY) * SUB_PIXEL; //size 10
  s_wave[1].origin.y = (s_windowSizeY + 20) * SUB_PIXEL; // gap 10, size 15
  s_wave[2].origin.y = (s_windowSizeY + 45) * SUB_PIXEL; // gap 10, size 20
  s_waveV = 0;

  s_score.points += s_score.pointBuffer;
  s_score.pointBuffer = 0;
  if (s_score.points > s_score.pointsToNextLevel) s_score.points = s_score.pointsToNextLevel; // Cap here

  // Where should the liquid now end?
  int fraction = 100 - ((s_score.points * SUB_PIXEL) / s_score.pointsToNextLevel); // 0 - 100
  s_liquidEnd = (fraction * s_windowSizeY);// [don't divide through, keep in subpixel]  / SUB_PIXEL;

  s_scoreState = kApplyPoints;
  return false;
}

bool applyPoints() {
  s_waveV += GRAVITY;
  for (int i = 0; i < 3; ++i) s_wave[i].origin.y -= s_waveV;

  // Can we move the background liquid? Is it masked by a rising wave?
  if (s_liquid.origin.y > s_liquidEnd && s_wave[0].origin.y + (5*SUB_PIXEL) < s_liquidEnd) s_liquid.origin.y -= s_waveV;

  // All waves have reached the top - next phase
  if (s_wave[2].origin.y + (s_wave[2].size.h * SUB_PIXEL) < 0) s_scoreState = kCheckNewLevel;
  return true;
}

void updateLevelColour() {
  s_colourFourground = s_score.level % N_LEVEL_COLOURS;
  s_colourBackground = (s_score.level - 1) % N_LEVEL_COLOURS;
  //APP_LOG(APP_LOG_LEVEL_INFO, "LEVEL %i: (nCol %i) FG and BG colours : %i %i", s_score.level, N_LEVEL_COLOURS, s_colourFourground, s_colourBackground);
}

bool checkNewLevel() {
  if (s_score.points >= s_score.pointsToNextLevel) {
    s_score.points -= s_score.pointsToNextLevel;
    s_score.pointsToNextLevel = (23 * s_score.pointsToNextLevel) / 20; //TODO balance this 23/20 = +15%
    switch (++s_score.level) { // At certain levels we make things even harded by adding more colours into the mix
        case 3: s_score.nColoursActive = 6; break;
        case 6: s_score.nColoursActive = 7; break;
        case 12: s_score.nColoursActive = 8; break;
        default: break;
    }
    updateLevelColour();
    s_liquidEnd = s_windowSizeY * SUB_PIXEL; // Fourground becomes background
    s_liquid.origin.y = s_windowSizeY * SUB_PIXEL;
  }
  s_nWaves = 0;
  s_scoreState = kWait;
  //APP_LOG(APP_LOG_LEVEL_INFO, "Wave animation done");
  return true;
}

bool checkLives() {
  static int v = 0;
  if (s_score.lives > 0) {
    v += GRAVITY/2;
    s_spentLifeAnimRadius += v;
    if (s_spentLifeAnimRadius > 200 * SUB_PIXEL) {
      s_spentLifeAnimRadius = 0;
      explode(kColourBoom, 0, 0, s_extraLifeBoomColour[ --s_score.lives ] );
      s_gameState = kFlashRemoved;
      s_currentRun = 0;
      v = 0;
    }
  } else {
    s_gameState = kGameOver;
  }
  return true;
}

bool gameOver() {
  if (s_frame == 0) {
    s_gameOverMessage = !s_gameOverMessage; // Blink message
    return true;
  }
  return false;
}

void gameLoop(void* data) {
  if (++s_frame == ANIM_FPS) s_frame = 0; // Note we may not draw every frame
  #ifdef DEBUG_MODE
  ++s_FPS;
  #endif

  bool requestRedraw = false;

  switch (s_gameState) {
    case kIdle: break;
    case kAwaitingDirection: requestRedraw = awaitingDirection(); break;
    case kCheckMove: requestRedraw = checkMove(); break;
    case kNudgeAnimate: requestRedraw = nudgeAnimate(); break;
    case kSwapAnimate: requestRedraw = swapAnimate(); break;
    case kFindMatches: requestRedraw = findMatches(); break;
    case kFlashRemoved: requestRedraw = flashRemoved(); break;
    case kRemoveAndReplace: requestRedraw = removeAndReplace(); break;
    case kSettleBoard: requestRedraw = settleBoard(); break;
    case kFindNextMove: requestRedraw = findNextMove(); break;
    case kCheckLives: requestRedraw = checkLives(); break;
    case kGameOver: requestRedraw = gameOver(); break;
    default: break;
  }

  switch (s_scoreState) {
    case kWait: requestRedraw |= checkScoreBuffer(); break;
    case kApplyPoints: requestRedraw |= applyPoints(); break;
    case kCheckNewLevel: requestRedraw |= checkNewLevel(); break;
    default: break;
  }

  // only if taking acceleromiter data do we ALWAY redraw
  if (s_tiltMode > 0 || requestRedraw == true) {
    redraw();
  }

  s_gameLoopTime = app_timer_register(ANIM_DELAY, gameLoop, NULL);
}

#ifdef DEBUG_MODE
void FPSTimer(void* data) {
  s_lastSecondFPS = s_FPS;
  s_FPS = 0;
  s_FPSTimer = app_timer_register(1000, FPSTimer, NULL);
}
#endif

/**  Called when a direction key is pressed when in SelectDirection mode
 *   OR whenever a new cell is entered by the accelerometer
 **/
void checkSwitch(int x, int y) {
  if (s_gameState != kAwaitingDirection) return;

  // Check out of bounds (for button instagated)
  if (y < 0 || y >= BOARD_PIECES_Y || x < 0 || x >= BOARD_PIECES_X) {
    redraw(); //  why?
    return;
  }

  s_switch.second.x = x;
  s_switch.second.y = y;

  s_gameState = kCheckMove;
}

static void dataHandler(AccelData* data, uint32_t num_samples) {
  // Update MotionCursor (the small white dot)
  s_motionCursor.x += data[0].x / s_tiltMode; // 0=off, 1=high, 2=low
  s_motionCursor.y -= data[0].y / s_tiltMode;

  if (s_motionCursor.x < 0) s_motionCursor.x += BOARD_SIZE_X * SUB_PIXEL;
  else if (s_motionCursor.x > BOARD_SIZE_X * SUB_PIXEL) s_motionCursor.x -= BOARD_SIZE_X * SUB_PIXEL;

  if (s_motionCursor.y < 0) s_motionCursor.y += BOARD_SIZE_Y * SUB_PIXEL;
  else if (s_motionCursor.y > BOARD_SIZE_Y * SUB_PIXEL) s_motionCursor.y -= BOARD_SIZE_Y * SUB_PIXEL;

  GPoint before = s_cursor;

  // Translate MotionCursor location to a cell
  s_cursor.x = s_motionCursor.x / (PIECE_SUB_PIXELS); //Note: bracket due to macro
  s_cursor.y = s_motionCursor.y / (PIECE_SUB_PIXELS);

  if (before.x != s_cursor.x || before.y != s_cursor.y) {
    checkSwitch(s_cursor.x, s_cursor.y); // Check the square i just moved into
  }

}

void checkTiltMode() {
  s_tiltMode = getTiltStatus();
  if (s_tiltMode > 0) {
    accel_data_service_subscribe(1, dataHandler);
    accel_service_set_sampling_rate(ACCEL_SAMPLING_25HZ);
  } else{
    accel_data_service_unsubscribe();
  }
}

void newGame(bool doLoadGame) {
  // Zero data store
  memset(&s_pieces, 0, BOARD_PIECES_X * BOARD_PIECES_Y * sizeof(Piece_t));
  memset(&s_score, 0, sizeof(Score_t));
  if (doLoadGame == true) {
    loadGame();
  } else { // new game
    // Init score
    s_score.level = 1;
    s_score.lives = 3;
    s_score.pointsToNextLevel = 200;
    s_score.nColoursActive = 5;
    #ifdef DEBUG_MODE
    s_score.nColoursActive = 3; // Allows to test long runs and interaction of special pieces with ease
    #endif
  }
  updateLevelColour();

  int offset = BOARD_SIZE_Y * SUB_PIXEL;
  for (int y = BOARD_PIECES_Y-1; y >= 0; --y) {
    for (int x = 0; x < BOARD_PIECES_X; ++x) {
      if (doLoadGame == false) {
        bool placing = true;
        while (placing) {
          s_pieces[ XY(x,y) ].colour = (rand() % s_score.nColoursActive) + 1;
          placing = checkLocation(GPoint(x,y)); // if match found then we're still placing as we need to try again
        }
      }
      s_pieces[ XY(x,y) ].loc.x = x * PIECE_SUB_PIXELS; // Set location
      s_pieces[ XY(x,y) ].loc.y = (y * PIECE_SUB_PIXELS) - offset; // Lets pieces fall in
    }
    offset += PIECE_SUB_PIXELS;
  }

  checkTiltMode();
  s_hintStatus = getHintStatus();  // Only want to read this once from store
  s_scoreState = kWait;
  s_gameState = kSettleBoard;
  s_gameOverMessage = false;
}

void mainWindowClickHandler(ClickRecognizerRef recognizer, void *context) {
  ButtonId button = click_recognizer_get_button_id(recognizer);

  if (s_gameState == kAwaitingDirection) {

    // Check the direction I just pressed
    if      (BUTTON_ID_UP     == button) checkSwitch(s_cursor.x    , s_cursor.y - 1);
    else if (BUTTON_ID_SELECT == button) checkSwitch(s_cursor.x + 1, s_cursor.y    );
    else if (BUTTON_ID_DOWN   == button) checkSwitch(s_cursor.x    , s_cursor.y + 1);
    else if (BUTTON_ID_BACK   == button) checkSwitch(s_cursor.x - 1, s_cursor.y    );

  } else {

    if (s_tiltMode == 0) {
      if        (BUTTON_ID_DOWN == button) {
        if (++s_cursor.y >= BOARD_PIECES_Y) s_cursor.y = 0;
      } else if (BUTTON_ID_SELECT == button) {
        if (++s_cursor.x >= BOARD_PIECES_X) s_cursor.x = 0;
      }
    } else {
      if      (BUTTON_ID_DOWN == button)   s_motionCursor.y += PIECE_SUB_PIXELS; // Manipulation of white dot cursor
      else if (BUTTON_ID_SELECT == button) s_motionCursor.x += PIECE_SUB_PIXELS;
    }

    if (BUTTON_ID_UP == button && s_gameState == kIdle) {
      s_gameState = kAwaitingDirection;
      s_frame = ANIM_FPS-1;
      s_switch.first = s_cursor;
    } else if (BUTTON_ID_BACK == button) {
      pushSplashWindow();
      return;
    }

    redraw();
  }
}

void mainWindowClickConfigProvider(Window *window) {
  window_single_repeating_click_subscribe(BUTTON_ID_SELECT, 100, mainWindowClickHandler);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 100, mainWindowClickHandler);
  window_single_click_subscribe(BUTTON_ID_UP, mainWindowClickHandler);
  window_single_click_subscribe(BUTTON_ID_BACK, mainWindowClickHandler);
}

void setFillColour(GContext *ctx, int i) {
  #ifdef PBL_BW
    if (i == 0) graphics_context_set_fill_color(ctx, GColorWhite);
    else graphics_context_set_fill_color(ctx, GColorLightGray);
  #else
    switch (i) {
      case 0: graphics_context_set_fill_color(ctx, GColorTiffanyBlue); break;
      case 1: graphics_context_set_fill_color(ctx, GColorMediumAquamarine); break;
      case 2: graphics_context_set_fill_color(ctx, GColorVividCerulean); break;
      case 3: graphics_context_set_fill_color(ctx, GColorSpringBud); break;
      case 4: graphics_context_set_fill_color(ctx, GColorBlueMoon); break;
      case 5: graphics_context_set_fill_color(ctx, GColorYellow); break;
      case 6: graphics_context_set_fill_color(ctx, GColorChromeYellow); break;
      case 7: graphics_context_set_fill_color(ctx, GColorSunsetOrange); break;
      case 8: graphics_context_set_fill_color(ctx, GColorMelon); break;
      case 9: graphics_context_set_fill_color(ctx, GColorPurple); break;
      case 10: graphics_context_set_fill_color(ctx, GColorRichBrilliantLavender); break;
      case 11: graphics_context_set_fill_color(ctx, GColorLiberty); break;
      case 12: graphics_context_set_fill_color(ctx, GColorWhite); break;
      default: graphics_context_set_fill_color(ctx, GColorRed); break;
    }
  #endif
}

static void mainBackgroundUpdateProc(Layer* this_layer, GContext *ctx) {

  // Colour background
  setFillColour(ctx, s_colourBackground);
  graphics_fill_rect(ctx, layer_get_bounds(this_layer), 0, GCornersAll);
  // Colour fourground
  setFillColour(ctx, s_colourFourground);
  GRect L = s_liquid;
  L.origin.y /= SUB_PIXEL;
  graphics_fill_rect(ctx, L, 0, GCornersAll);
  // Colour waves
  if (s_nWaves) {
    setFillColour(ctx, s_colourBackground);
    for (int i = 0; i < s_nWaves; ++i) {
      GRect W = s_wave[i];
      W.origin.y /= SUB_PIXEL;
      graphics_fill_rect(ctx, W, 0, GCornersAll);
    }
  }

  // Draw current level indicator
  GRect b = layer_get_bounds(this_layer);
  GRect levelRect = GRect( ((b.size.w - BOARD_SIZE_X)/2), b.size.h - 15, PIECE_PIXELS, 15);
  #ifdef PBL_ROUND
  levelRect.origin.x = 0; // Round screens - level indicator is centered
  levelRect.size.w = b.size.w;
  levelRect.origin.y -= 4;
  #endif
  static char levelBuffer[5];
  snprintf(levelBuffer, 5, "%i", s_score.level);
  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_draw_text(ctx, levelBuffer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), levelRect, GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);

  // Draw FPS indicator (dbg only)
  #ifdef DEBUG_MODE
  static char FPSBuffer[5];
  snprintf(FPSBuffer, 5, "%i", s_lastSecondFPS);
  GRect fpsRect = GRect( 100, b.size.h - 15, PIECE_PIXELS, 15);
  graphics_draw_text(ctx, FPSBuffer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), fpsRect, GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  #endif

}

static void mainForegroundUpdateProc(Layer* this_layer, GContext *ctx) {
  GRect b = layer_get_bounds(this_layer);

  #ifdef PBL_ROUND
  b.size.h -= 16; // Because of the time at the top, default position is too low on a round screen
  #endif

  // Draw the three lives
  GPoint lives = GPoint( b.size.w/2 - PIECE_PIXELS, b.size.h - 6);
  for (int L=0; L<3; ++L) {
    GColor in;
    GColor out;
    if (s_score.lives >= (3 - L)) {
      if (L == 0) { in = COLOR_FALLBACK(GColorMintGreen, GColorWhite); out = COLOR_FALLBACK(GColorDarkGreen, GColorBlack); }
      else if (L == 1) { in = COLOR_FALLBACK(GColorRajah, GColorWhite); out = COLOR_FALLBACK(GColorWindsorTan, GColorBlack); }
      else { in = COLOR_FALLBACK(GColorMelon, GColorWhite); out = COLOR_FALLBACK(GColorDarkCandyAppleRed, GColorBlack); }
    } else {
      in = COLOR_FALLBACK(GColorLightGray, GColorBlack);
      out = COLOR_FALLBACK(GColorDarkGray, GColorBlack);
    }
    GPoint p = lives;
    p.x += PIECE_PIXELS * L;
    graphics_context_set_stroke_color(ctx, out);
    graphics_context_set_fill_color(ctx, in);
    graphics_context_set_stroke_width(ctx, 3);
    graphics_fill_circle(ctx, p, 3);
    graphics_draw_circle(ctx, p, 3);
    // Animation?
    if (s_spentLifeAnimRadius > 0 && s_score.lives == (3 - L)) {
      graphics_context_set_stroke_width(ctx, 5);
      graphics_context_set_stroke_color(ctx, COLOR_FALLBACK(in,out));
      graphics_draw_circle(ctx, p, s_spentLifeAnimRadius / SUB_PIXEL );
    }
  }

  // Draw the game over message
  if (s_gameOverMessage == true) {
    static const char txtGame[] = "GAME";
    static const char txtOver[] = "OVER";
    b.origin.y += 30;
    draw3DText(ctx, b, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD), txtGame, 1, GTextAlignmentCenter, true, GColorWhite, GColorBlack);
    b.origin.y += 40;
    draw3DText(ctx, b, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD), txtOver, 1, GTextAlignmentCenter, true, GColorWhite, GColorBlack);
  }
}


static void boardUpdateProc(Layer* this_layer, GContext *ctx) {
  GRect b = layer_get_bounds(this_layer);
  graphics_context_set_antialiased(ctx, 0);

  // Fill grey back (white on BW)
  graphics_context_set_fill_color(ctx, COLOR_FALLBACK(GColorLightGray, GColorWhite));
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, b, 0, GCornersAll);

  // Frame highlight around currently selected square colours is gray w white highlight, BW is wite w black highight
  graphics_context_set_fill_color(ctx, COLOR_FALLBACK(GColorDarkGray, GColorWhite));
  graphics_context_set_stroke_color(ctx, COLOR_FALLBACK(GColorWhite, GColorBlack));
  GRect highlightR = GRect(s_cursor.x*PIECE_PIXELS, s_cursor.y*PIECE_PIXELS, PIECE_PIXELS+1, PIECE_PIXELS+1);
  graphics_fill_rect(ctx, highlightR, 0, GCornersAll);
  graphics_draw_rect(ctx, highlightR);

  // Draw all board shapes
  for (int x = 0; x < BOARD_PIECES_X; ++x) {
    for (int y = 0; y < BOARD_PIECES_Y; ++y) {
      int xy = XY(x,y);
      if (s_gameState == kFlashRemoved && (s_pieces[xy].match != kUnmatched || s_pieces[XY(x,y)].exploded == true)) {
        // Highlight squares which have just been matched
        GColor highlight = COLOR_FALLBACK(GColorRed, GColorBlack);
        if (s_pieces[xy].match == kMatchedTwice) highlight = COLOR_FALLBACK(GColorDarkCandyAppleRed, GColorBlack);
        graphics_context_set_fill_color(ctx, highlight);
        graphics_context_set_stroke_color(ctx, COLOR_FALLBACK(GColorBlack, GColorLightGray));
        highlightR = GRect((s_pieces[xy].loc.x/SUB_PIXEL), (s_pieces[xy].loc.y/SUB_PIXEL), PIECE_PIXELS+1, PIECE_PIXELS+1);
        graphics_fill_rect(ctx, highlightR, 0, GCornersAll);
        graphics_draw_rect(ctx, highlightR);
      }
      // Draw the shape itself
      graphics_context_set_fill_color(ctx, COLOURS[ s_pieces[xy].colour ]);
      if (getShape( s_pieces[xy].colour ) != NULL) {
        static const bool bw = PBL_IF_BW_ELSE(true,false);
        if (bw == false && s_pieces[xy].colour == kBlack ) {
          graphics_context_set_stroke_color(ctx, GColorWhite);
        } else if (bw == true
          && (s_pieces[xy].match != kUnmatched || s_pieces[XY(x,y)].exploded == true)
          && COLOURS[ s_pieces[xy].colour ].argb != GColorWhite.argb ) {
          graphics_context_set_stroke_color(ctx, GColorWhite);
        } else {
          graphics_context_set_stroke_color(ctx, GColorBlack);
        }
        gpath_move_to(getShape( s_pieces[xy].colour ), GPoint(s_pieces[xy].loc.x/SUB_PIXEL, s_pieces[xy].loc.y/SUB_PIXEL));
        gpath_draw_filled(ctx, getShape( s_pieces[xy].colour ));
        gpath_draw_outline(ctx, getShape( s_pieces[xy].colour ));
      } else {
        // White rectangle of debug - we should not have the case where we render a NULL square
        graphics_fill_rect(ctx, GRect((s_pieces[xy].loc.x/SUB_PIXEL)+2, (s_pieces[xy].loc.y/SUB_PIXEL)+2, PIECE_PIXELS-3, PIECE_PIXELS-3), 2, GCornersAll);
      }
    }
  }

  // Move Arrows
  if (s_flashArrows == true && s_gameState == kAwaitingDirection) {
    graphics_context_set_fill_color(ctx, GColorWhite);
    for (int d = 0; d < N_CARDINAL; ++d) {
      gpath_move_to(getArrow(d), GPoint(s_cursor.x * PIECE_PIXELS, s_cursor.y * PIECE_PIXELS));
      gpath_draw_filled(ctx, getArrow(d));
      gpath_draw_outline(ctx, getArrow(d));
    }
  }

  // Cursor
  if (s_tiltMode > 0) {
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_context_set_stroke_color(ctx, GColorBlack);
    graphics_fill_circle(ctx, GPoint(s_motionCursor.x/SUB_PIXEL,s_motionCursor.y/SUB_PIXEL), 3);
    graphics_draw_circle(ctx, GPoint(s_motionCursor.x/SUB_PIXEL,s_motionCursor.y/SUB_PIXEL), 3);
  }

  // Next move
  if (s_hintOn && s_availableMove.x != -1 && s_gameState == kIdle && s_hintStatus == true) {
    graphics_context_set_stroke_color(ctx, COLOR_FALLBACK(GColorDarkCandyAppleRed,GColorBlack) );
    graphics_context_set_stroke_width(ctx, 3);
    graphics_draw_circle(ctx, GPoint(s_availableMove.x*PIECE_PIXELS + PIECE_PIXELS/2, s_availableMove.y*PIECE_PIXELS + PIECE_PIXELS/2), PIECE_PIXELS);
    graphics_context_set_stroke_width(ctx, 1);
  }

  // Redo border
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_draw_rect(ctx, b);

}

void mainWindowLoad(Window* parentWindow) {
  GRect b = layer_get_bounds( window_get_root_layer(parentWindow) );
  s_windowSizeY = b.size.h - STATUS_BAR_LAYER_HEIGHT;

  s_statusBar = status_bar_layer_create();
  layer_add_child(window_get_root_layer(parentWindow), status_bar_layer_get_layer(s_statusBar));
  status_bar_layer_set_separator_mode(s_statusBar, StatusBarLayerSeparatorModeDotted);

  s_mainBackgroundLayer = layer_create( GRect(0, STATUS_BAR_LAYER_HEIGHT, b.size.w, b.size.h - STATUS_BAR_LAYER_HEIGHT) );
  layer_add_child(window_get_root_layer(parentWindow), s_mainBackgroundLayer);
  layer_set_update_proc(s_mainBackgroundLayer, mainBackgroundUpdateProc);

  int dispX = (b.size.w - BOARD_SIZE_X) / 2;
  int dispY = dispX;
  #ifdef PBL_ROUND
  dispY = 5;
  #endif

  s_boardLayer = layer_create( GRect(dispX, dispY, BOARD_SIZE_X + 1, BOARD_SIZE_Y + 1) );
  layer_add_child(s_mainBackgroundLayer, s_boardLayer);
  layer_set_update_proc(s_boardLayer, boardUpdateProc);
  layer_set_clips(s_boardLayer, true);

  s_mainForegroundLayer = layer_create( GRect(0, STATUS_BAR_LAYER_HEIGHT, b.size.w, b.size.h - STATUS_BAR_LAYER_HEIGHT) );
  layer_add_child(window_get_root_layer(parentWindow), s_mainForegroundLayer);
  layer_set_update_proc(s_mainForegroundLayer, mainForegroundUpdateProc);

  s_wave[0] = GRect(0, b.size.h * SUB_PIXEL, b.size.w, 10);
  s_wave[1] = GRect(0, b.size.h * SUB_PIXEL, b.size.w, 15);
  s_wave[2] = GRect(0, b.size.h * SUB_PIXEL, b.size.w, 20);
  s_liquid  = GRect(0, b.size.h * SUB_PIXEL, b.size.w, b.size.h); // TODO x2 for safety
  s_nWaves = 0;
  s_liquidEnd = s_windowSizeY * SUB_PIXEL;

  s_currentRun = 0;
  s_switch.first = GPoint(-1,-1);
  s_switch.second = GPoint(-1,-1);
  s_cursor = GPoint(0,0);
  s_motionCursor = GPoint(PIECE_SUB_PIXELS/2,PIECE_SUB_PIXELS/2);
  s_availableMove = GPoint(-1,-1);
  srand(time(NULL));
  startGameTick();
}

void mainWindowUnload() {
  if (s_gameState == kGameOver) endGame();
  else saveGame();
  layer_destroy(s_boardLayer);
  layer_destroy(s_mainBackgroundLayer);
  layer_destroy(s_mainForegroundLayer);
  status_bar_layer_destroy(s_statusBar);
  s_boardLayer = NULL;
  s_mainBackgroundLayer = NULL;
  s_mainForegroundLayer = NULL;
  s_statusBar = NULL;
  stopGameTick();
  accel_data_service_unsubscribe();
}

void startGameTick() {
  gameLoop(NULL);
  #ifdef DEBUG_MODE
  FPSTimer(NULL);
  #endif
}

void stopGameTick() {
  if (s_gameLoopTime) app_timer_cancel(s_gameLoopTime);
  s_gameLoopTime = NULL;
  if (s_hintTimer) app_timer_cancel(s_hintTimer);
  s_hintTimer = NULL;
  #ifdef DEBUG_MODE
  if (s_FPSTimer) app_timer_cancel(s_FPSTimer);
  s_FPSTimer = 0;
  #endif
}
