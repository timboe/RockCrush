#include <pebble.h>
#include "mainWindow.h"



static Layer* s_mainWindowLayer = NULL;
static Layer* s_boardLayer = NULL;
static StatusBarLayer* s_statusBar = NULL;

static GPoint s_cursor = {0,0};

#define MS_IN_SEC 1000
#define ANIM_FPS 25
#define ANIM_FRAMES ANIM_FPS*ANIM_DURATION
#define ANIM_DELAY MS_IN_SEC/ANIM_FPS
#define GRAVITY 5
static int s_frame = 0;
static int s_animateFrames = 0;

#define SUB_PIXEL 100

#define BOARD_PIECES_X 9
#define BOARD_PIECES_Y 9
#define PIECE_PIXELS 15
#define BOARD_PIECES BOARD_PIECES_X*PIECE_PIXELS_Y
#define BOARD_SIZE_X BOARD_PIECES_X*PIECE_PIXELS
#define BOARD_SIZE_Y BOARD_PIECES_Y*PIECE_PIXELS

#define N_PRIMARY_COLOURS 7
#define N_COLOURS 9
typedef enum {
  kRed,
  kYellow,
  kBlue,
  kGreen,
  kPurple,
  kOrange,
  kPink,
  kWhite,
  kBlack,
  kNONE
} Colour_t;

typedef enum {
  kIdle,
  kAwaitingDirection,
  kCheckMove,
    kNudgeAnimate,
    kSwapAnimate,
} GameState_t;
GameState_t s_gameState = kIdle;

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

static const GPathInfo shapeGreen = {
  .num_points = 5,
  .points = (GPoint []) {{2, 3}, {5, 13}, {7, 13}, {13, 9}, {11, 2}}
};

static const GPathInfo shapeWhite = {
  .num_points = 8,
  .points = (GPoint []) {{5, 2}, {10, 2}, {13, 5}, {13, 10}, {10, 13}, {5, 13}, {2, 10}, {2, 5}}
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
  int x;
  int y;
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
    if (switchColour == kBlack) return true; // Always valid
    if (a == b && b == switchColour) return true; // Standard
    if (a == b && switchColour == kWhite) return true; // Play wildcolour
    return false;
}

// Need to look two away on either side 0000CCSCC0000. 0=ignore, C=check, S=swap
void checkMove() {

  s_animateFrames = 0;

  for (int p=0; p<2; ++p) {
    // Look in location A with location B's colour and visa versa
    GPoint piece = s_switch.first;
    Colour_t colour = s_pieces[ XYp( s_switch.second ) ].colour;
    if (p == 1) {
      piece = s_switch.second;
      colour = s_pieces[ XYp( s_switch.first ) ].colour;
    }

    // TODO - loop this
    bool isValid = false;

    if (isValid == false && piece.x >= 2) {
      isValid = checkTriplet( s_pieces[XY(piece.x-2, piece.y)].colour, s_pieces[XY(piece.x-1, piece.y)].colour, colour);
    }

    if (isValid == false && piece.x >= 1 && piece.x < BOARD_PIECES_X-2) {
      isValid = checkTriplet( s_pieces[XY(piece.x-1, piece.y)].colour, s_pieces[XY(piece.x+1, piece.y)].colour, colour);
    }

    if (isValid == false && piece.x < BOARD_PIECES_X-3) {
      isValid = checkTriplet( s_pieces[XY(piece.x+2, piece.y)].colour, s_pieces[XY(piece.x+1, piece.y)].colour, colour);
    }

    if (isValid == false && piece.y >= 2) {
      isValid = checkTriplet( s_pieces[XY(piece.x, piece.y-2)].colour, s_pieces[XY(piece.x, piece.y-1)].colour, colour);
    }

    if (isValid == false && piece.y >= 1 && piece.x < BOARD_PIECES_Y-2) {
      isValid = checkTriplet( s_pieces[XY(piece.x, piece.y-1)].colour, s_pieces[XY(piece.x, piece.y+1)].colour, colour);
    }

    if (isValid == false && piece.x < BOARD_PIECES_Y-3) {
      isValid = checkTriplet( s_pieces[XY(piece.x, piece.y+2)].colour, s_pieces[XY(piece.x, piece.y+1)].colour, colour);
    }

    if (isValid) {
      s_gameState = kSwapAnimate;
      return;
    }
  }

  // Not valid
  s_gameState = kNudgeAnimate;
}

void nudgeAnimate() {
  static int v = 0;
  static int mode = 0;

  if (mode == 0 && v >= 50) mode = 1;
  else if (mode == 1 && v <= -50) mode = 2;
  else if (mode == 2 && v >= 0) mode = 3;

  if (mode == 0 || mode == 2) v += GRAVITY*2;
  else if (mode == 1) v -= GRAVITY*2;

  if (s_switch.first.x > s_switch.second.x){
    s_pieces[XYp(s_switch.first)].x  -= v;
    s_pieces[XYp(s_switch.second)].x += v;
  } else if (s_switch.second.x > s_switch.first.x) {
    s_pieces[XYp(s_switch.first)].x  += v;
    s_pieces[XYp(s_switch.second)].x -= v;
  } else if (s_switch.first.y > s_switch.second.y){
    s_pieces[XYp(s_switch.first)].y  -= v;
    s_pieces[XYp(s_switch.second)].y += v;
  } else{
    s_pieces[XYp(s_switch.first)].y  += v;
    s_pieces[XYp(s_switch.second)].y -= v;
  }

  if (mode == 3) {
    mode = 0;
    v = 0;
    s_gameState = kIdle; // TODO keep going!
  }

  redraw();
}

void swapAnimate() {
  s_gameState = kIdle; // TODO me
}

void gameLoop(void* data) {
  if (++s_frame == ANIM_FPS) s_frame = 0;

  switch (s_gameState) {
    case kIdle: break;
    case kAwaitingDirection: if (s_frame == 0 || s_frame == ANIM_FPS/2) redraw(); break;
    case kCheckMove: checkMove(); break;
    case kNudgeAnimate: nudgeAnimate(); break;
    case kSwapAnimate: swapAnimate(); break;
    default: break;
  }
  app_timer_register(ANIM_DELAY, gameLoop, NULL);
}

void mainWindowClickHandler(ClickRecognizerRef recognizer, void *context) {
  ButtonId button = click_recognizer_get_button_id(recognizer);

  if (s_gameState == kIdle) {

    if (BUTTON_ID_UP == button) --s_cursor.y;
    else if (BUTTON_ID_SELECT == button) ++s_cursor.x;
    else if (BUTTON_ID_DOWN == button) s_gameState = kAwaitingDirection;
    // else back, show menu
    if (s_cursor.x >= BOARD_PIECES_X) s_cursor.x = 0;
    if (s_cursor.y < 0) s_cursor.y = BOARD_PIECES_Y - 1;
    redraw();

  } else if (s_gameState == kAwaitingDirection) {

    s_switch.first = s_cursor;
    if (BUTTON_ID_UP == button && s_cursor.y != 0) {
      s_switch.second = GPoint(s_cursor.x, s_cursor.y - 1);
    } else if (BUTTON_ID_SELECT == button && s_cursor.x != BOARD_PIECES_X - 2) {
      s_switch.second = GPoint(s_cursor.x + 1, s_cursor.y);
    } else if (BUTTON_ID_DOWN == button && s_cursor.y != BOARD_PIECES_Y - 2) {
      s_switch.second = GPoint(s_cursor.x, s_cursor.y + 1);
    } else if (BUTTON_ID_BACK == button && s_cursor.x != 0) {
      s_switch.second = GPoint(s_cursor.x - 1, s_cursor.y);
    } else {
      s_gameState = kIdle; // Invalid
      redraw();
      return;
    }
    s_gameState = kCheckMove;

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
  graphics_context_set_antialiased(ctx, 0);

  // Fill back
  graphics_context_set_fill_color(ctx, GColorDarkGray);
  graphics_fill_rect(ctx, GRect(0, 0, BOARD_SIZE_X, BOARD_SIZE_Y), 0, GCornersAll);

  // Fill highight
  graphics_context_set_fill_color(ctx, GColorLightGray);
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
  graphics_draw_rect(ctx, GRect(s_cursor.x * PIECE_PIXELS, 0, PIECE_PIXELS+1, BOARD_SIZE_Y+1));
  graphics_draw_rect(ctx, GRect(0, s_cursor.y * PIECE_PIXELS, BOARD_SIZE_X+1, PIECE_PIXELS+1));

  // Redo border
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_draw_rect(ctx, GRect(0, 0, BOARD_SIZE_X+1, BOARD_SIZE_Y+1));


  for (int x = 0; x < BOARD_PIECES_X; ++x) {
    for (int y = 0; y < BOARD_PIECES_Y; ++y) {
      graphics_context_set_stroke_color(ctx, GColorBlack);
      int xy = XY(x,y);
      switch (s_pieces[xy].colour) {
        case kRed: graphics_context_set_fill_color(ctx, GColorRed); break;
        case kYellow: graphics_context_set_fill_color(ctx, GColorYellow); break;
        case kBlue: graphics_context_set_fill_color(ctx, GColorBlue); break;
        case kGreen: graphics_context_set_fill_color(ctx, GColorGreen); break;
        case kPurple: graphics_context_set_fill_color(ctx, GColorPurple); break;
        case kOrange: graphics_context_set_fill_color(ctx, GColorOrange); break;
        case kPink: graphics_context_set_fill_color(ctx, GColorShockingPink); break;
        case kWhite: graphics_context_set_fill_color(ctx, GColorWhite); break;
        case kBlack: graphics_context_set_fill_color(ctx, GColorBlack); break;
        default: break;
      }
      if (s_shapes[ s_pieces[xy].colour ] != NULL) {
        gpath_move_to(s_shapes[ s_pieces[xy].colour ], GPoint(s_pieces[xy].x/SUB_PIXEL, s_pieces[xy].y/SUB_PIXEL));
        gpath_draw_filled(ctx, s_shapes[ s_pieces[xy].colour ]);
        gpath_draw_outline(ctx, s_shapes[ s_pieces[xy].colour ]);
      } else {
        graphics_fill_rect(ctx, GRect((s_pieces[xy].x/SUB_PIXEL)+2, (s_pieces[xy].y/SUB_PIXEL)+2, PIECE_PIXELS-3, PIECE_PIXELS-3), 2, GCornersAll);
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

  }
  //
  //




  //
  // // TODO charge status
  // // BATTERY + BT
  // if (clockUpgrade >= TECH_BATTERY) {
  //   graphics_context_set_stroke_color(ctx, GColorWhite);
  //   graphics_context_set_fill_color(ctx, getLiquidTimeHighlightColour());
  //   graphics_draw_rect(ctx, GRect(119,9,18,7));
  //   graphics_draw_rect(ctx, GRect(137,11,2,3));
  //   graphics_fill_rect(ctx, GRect(121,11,s_battery.charge_percent/7,3), 0, GCornersAll); // 100%=14 pixels
  //   if (s_bluetoothStatus) {
  //     graphics_context_set_compositing_mode(ctx, GCompOpSet);
  //     graphics_context_set_fill_color(ctx, getLiquidTimeColour());
  //     graphics_fill_rect(ctx, GRect(8, 6, 5, 12), 0, GCornersAll);
  //     drawBitmap(ctx, getBluetoothImage(), GRect(7, 5, 7, 14));
  //   }
  // }
  //
  // // DATE
  // if (clockUpgrade >= TECH_MONTH) {
  //   GRect dateRect = GRect(b.origin.x, b.origin.y, b.size.w, 30);
  //   draw3DText(ctx, dateRect, getClockSmallFont(), s_dateBuffer, 1, false, GColorBlack, GColorBlack);
  // }
  //
  // int makeRoomForTheDateOffset = 0;
  // if (clockUpgrade == 0) makeRoomForTheDateOffset = -10;
  // GRect timeRect = GRect(b.origin.x, b.origin.y + CLOCK_OFFSET + makeRoomForTheDateOffset, b.size.w, b.size.h - CLOCK_OFFSET - makeRoomForTheDateOffset);
  //
  // draw3DText(ctx, timeRect, getClockFont(), s_timeBuffer, s_clockPixelOffset, false, GColorBlack, GColorBlack);
  //
  // if (s_clockTickCount == 0) return; // No animation in progress
  //
  // graphics_context_set_stroke_color(ctx, GColorBlack);
  // for (unsigned i = 0; i < N_SPOOGELET; ++i) {
  //   const GPoint p = GPoint(s_spoogelet[i].x / SUB_PIXEL, s_spoogelet[i].y / SUB_PIXEL);
  //   if ((p.x - s_attractor.x) < 8 && (p.x - s_attractor.x) > -8 && (p.y - s_attractor.y) < 8 && (p.y - s_attractor.y) > -8) continue; // Too close
  //   const uint8_t r = 1+rand()%3;
  //   if (i%2==0) graphics_context_set_fill_color(ctx, getSpoogicalColourA());
  //   else        graphics_context_set_fill_color(ctx, getSpoogicalColourB());
  //   graphics_fill_circle(ctx, p, r);
  //   graphics_draw_circle(ctx, p, r);
  // }

}

void newGame() {
  for (int x = 0; x < BOARD_PIECES_X; ++x) {
    for (int y = 0; y < BOARD_PIECES_Y; ++y) {
      s_pieces[ XY(x,y) ].colour = rand() % N_COLOURS;
      s_pieces[ XY(x,y) ].x = x * PIECE_PIXELS * SUB_PIXEL;
      s_pieces[ XY(x,y) ].y = y * PIECE_PIXELS * SUB_PIXEL;
    }
  }
}

void mainWindowLoad(Window* parentWindow) {
  GRect b = layer_get_bounds( window_get_root_layer(parentWindow) );

  s_statusBar = status_bar_layer_create();
  layer_add_child(window_get_root_layer(parentWindow), status_bar_layer_get_layer(s_statusBar));
  status_bar_layer_set_separator_mode(s_statusBar, StatusBarLayerSeparatorModeDotted);

  s_mainWindowLayer = layer_create( GRect(0, STATUS_BAR_LAYER_HEIGHT, b.size.w, b.size.h - STATUS_BAR_LAYER_HEIGHT) );
  layer_add_child(window_get_root_layer(parentWindow), s_mainWindowLayer);
  layer_set_update_proc(s_mainWindowLayer, mainWindowUpdateProc);

  int hDisp = (b.size.w - BOARD_SIZE_X) / 2;
  int vDisp = (b.size.h - BOARD_SIZE_Y - STATUS_BAR_LAYER_HEIGHT) / 2;
  s_boardLayer = layer_create( GRect(hDisp, vDisp, BOARD_SIZE_X + 1, BOARD_SIZE_Y + 1) );
  layer_add_child(s_mainWindowLayer, s_boardLayer);
  layer_set_update_proc(s_boardLayer, boardUpdateProc);
  layer_set_clips(s_boardLayer, true);

  s_shapes[kRed] = gpath_create(&shapeRed);
  s_shapes[kGreen] = gpath_create(&shapeGreen);
  s_shapes[kWhite] = gpath_create(&shapeWhite);

  s_arrows[kN] = gpath_create(&shapeN);
  s_arrows[kE] = gpath_create(&shapeE);
  s_arrows[kS] = gpath_create(&shapeS);
  s_arrows[kW] = gpath_create(&shapeW);


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
