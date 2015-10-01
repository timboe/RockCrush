#include <pebble.h>
#include "mainWindow.h"

static Layer* s_mainWindowLayer = NULL;
static StatusBarLayer* s_statusBar = NULL;

static GPoint s_cursor = {0,0};

#define START_X 5
#define START_Y 5

#define BOARD_PIECES 9
#define PIECE_PIXELS 15
#define PIECE_HALF (PIECE_PIXELS/2)
#define BOARD_SIZE BOARD_PIECES*PIECE_PIXELS

#define N_PRIMARY_COLOURS 7;
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

typedef struct {
  Colour_t colour;
  int x;
  int y;
} Piece_t;

Piece_t s_pieces[BOARD_PIECES*BOARD_PIECES];

int XY(int x, int y) { return x + (BOARD_PIECES * y); }

void redraw() {
  layer_mark_dirty(s_mainWindowLayer);
};

void mainWindowClickHandler(ClickRecognizerRef recognizer, void *context) {
  ButtonId button = click_recognizer_get_button_id(recognizer);

  if (BUTTON_ID_UP == button) {
    APP_LOG(APP_LOG_LEVEL_DEBUG,"Up %i", s_cursor.y);
    --s_cursor.y;
  } else if (BUTTON_ID_SELECT == button) {
    ++s_cursor.x;
  } else if (BUTTON_ID_DOWN == button) {
    ++s_cursor.y;
  } else { // Back
    --s_cursor.x;
  }

  if (s_cursor.x >= BOARD_PIECES) s_cursor.x = 0;
  else if (s_cursor.x < 0) s_cursor.x = BOARD_PIECES - 1;

  if (s_cursor.y >= BOARD_PIECES) s_cursor.y = 0;
  else if (s_cursor.y < 0) s_cursor.y = BOARD_PIECES - 1;

  redraw();
}

void mainWindowClickConfigProvider(Window *window) {
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 100, mainWindowClickHandler);
  window_single_repeating_click_subscribe(BUTTON_ID_SELECT, 100, mainWindowClickHandler);
  window_single_repeating_click_subscribe(BUTTON_ID_UP, 100, mainWindowClickHandler);
  window_single_click_subscribe(BUTTON_ID_BACK, mainWindowClickHandler);
}

static void mainWindowUpdateProc(Layer *this_layer, GContext *ctx) {
  GRect b = layer_get_bounds(this_layer);
  GPoint origin = GPoint( (b.size.w - BOARD_SIZE) / 2, (b.size.h - BOARD_SIZE) / 2);
  // const int clockUpgrade = getUserSetting(SETTING_TECH);

  graphics_context_set_stroke_color(ctx, GColorBlue);
  graphics_draw_rect(ctx, GRect(origin.x - 1, origin.y - 1, BOARD_SIZE + 2, BOARD_SIZE + 2));

  graphics_context_set_stroke_color(ctx, GColorYellow);
  graphics_draw_line(ctx, GPoint(origin.x, origin.y + s_cursor.y * PIECE_PIXELS),
                          GPoint(origin.x + BOARD_SIZE, origin.y + s_cursor.y * PIECE_PIXELS));
  graphics_draw_line(ctx, GPoint(origin.x, origin.y + (s_cursor.y + 1) * PIECE_PIXELS),
                          GPoint(origin.x + BOARD_SIZE, origin.y + (s_cursor.y + 1) * PIECE_PIXELS));

  graphics_draw_line(ctx, GPoint(origin.x + s_cursor.x * PIECE_PIXELS, origin.y),
                          GPoint(origin.x + s_cursor.x * PIECE_PIXELS, origin.y + BOARD_SIZE));
  graphics_draw_line(ctx, GPoint(origin.x + (s_cursor.x + 1) * PIECE_PIXELS, origin.y),
                          GPoint(origin.x + (s_cursor.x + 1) * PIECE_PIXELS, origin.y + BOARD_SIZE));


  for (int x = 0; x < BOARD_PIECES; ++x) {
    for (int y = 0; y < BOARD_PIECES; ++y) {
      graphics_context_set_stroke_color(ctx, GColorDarkGray);
      int xy = XY(x,y);
      switch (s_pieces[ xy ].colour) {
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
      graphics_fill_circle(ctx,
        GPoint(
          origin.x + PIECE_HALF + 1 + (s_pieces[xy].x * PIECE_PIXELS),
          origin.y + PIECE_HALF + 1 + (s_pieces[xy].y * PIECE_PIXELS)
        ),
        PIECE_HALF - 1
      );
    }
  }



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
  for (int x = 0; x < BOARD_PIECES; ++x) {
    for (int y = 0; y < BOARD_PIECES; ++y) {
      s_pieces[ XY(x,y) ].colour = rand() % N_PRIMARY_COLOURS;
      s_pieces[ XY(x,y) ].x = x;
      s_pieces[ XY(x,y) ].y = y;
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

  newGame();

}


void mainWindowUnload() {
  layer_destroy(s_mainWindowLayer);
  status_bar_layer_destroy(s_statusBar);
  s_mainWindowLayer = NULL;
  s_statusBar = NULL;
}
