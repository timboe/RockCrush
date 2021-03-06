#include <splash.h>
#include <main.h>
#include <globals.h>
#include <persistence.h>
#include <mainWindow.h>


static Layer* s_mainLayer;
static Layer* s_scrollLayer;
static StatusBarLayer* s_statusBar = NULL;

#define TEXT_BUFFER_SIZE 32
#define BORDER 5
#define ROUND_REDUCTION 20

static int s_frame = 0;

//static int s_tickerTape = 0;

#define MENU_ITEMS 5
static int s_textOff = 0;
char* s_textMain[MENU_ITEMS];
char* s_textSub[MENU_ITEMS];
static int s_selectedMenuItem;
static int s_minOption = 0;

static AppTimer* s_tickTimer;

static bool s_arrowBlink;

static void redraw() {
  layer_mark_dirty(s_mainLayer);
};

static void splashUpdateProc(Layer* thisLayer, GContext *ctx) {

  GRect b = layer_get_bounds(thisLayer);
  #ifdef PBL_ROUND
  // Modify for round screen
  b.size.w -= ROUND_REDUCTION * 2;
  b.origin.x += ROUND_REDUCTION;
  b.size.h -= (ROUND_REDUCTION * 3) / 4;
  #endif

  GRect titleR = GRect(b.origin.x + 5, 5, b.size.w - 10, PBL_IF_ROUND_ELSE((9*b.size.h)/20, (4*b.size.h)/10) );
  // Fill back
  graphics_context_set_fill_color(ctx, GColorLightGray);
  graphics_fill_rect(ctx, titleR, 10, GCornersAll);
  // Fill screws
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_fill_color(ctx, COLOR_FALLBACK(GColorDarkGray, GColorLightGray));
  GPoint screw_one = GPoint(titleR.origin.x + 1*titleR.size.w/6 - 2, titleR.origin.y + 2*titleR.size.h/3);
  GPoint screw_two = GPoint(titleR.origin.x + 5*titleR.size.w/6 - 2, titleR.origin.y + 1*titleR.size.h/3);
  graphics_fill_circle(ctx, screw_one, 4);
  graphics_fill_circle(ctx, screw_two, 4);
  graphics_draw_circle(ctx, screw_one, 4);
  graphics_draw_circle(ctx, screw_two, 4);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_line(ctx, GPoint(screw_one.x-2, screw_one.y-2), GPoint(screw_one.x+2, screw_one.y+2));
  graphics_draw_line(ctx, GPoint(screw_two.x+3, screw_two.y-1), GPoint(screw_two.x-3, screw_two.y+1));
  graphics_context_set_stroke_width(ctx, 3);
  GRect middle_wall = GRect(titleR.origin.x + 1, titleR.origin.y + 0, titleR.size.w - 2, titleR.size.h - 0);
  GRect inner_wall  = GRect(titleR.origin.x + 2, titleR.origin.y + 2, titleR.size.w - 4, titleR.size.h - 4);
  graphics_context_set_stroke_color(ctx, COLOR_FALLBACK(GColorDarkGray, GColorLightGray));
  graphics_draw_round_rect(ctx, middle_wall, 10);
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_round_rect(ctx, inner_wall, 10);

  // Fill the text

  GRect rectA = GRect(titleR.origin.x + 8,
    titleR.origin.y,
    titleR.size.w-20,
    20);

  GRect rectB = GRect(titleR.origin.x + 6,
    titleR.origin.y + 23,
    titleR.size.w - 20,
    20);

  static const char txtA[] = "ROCK";
  static const char txtB[] = "Crusher";

  bool bw = PBL_IF_BW_ELSE(true,false);
  draw3DText(ctx, rectA, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK), txtA, 2, GTextAlignmentLeft, bw, GColorBlack, GColorWhite);
  draw3DText(ctx, rectB, fonts_get_system_font(FONT_KEY_GOTHIC_28), txtB, 2, GTextAlignmentRight, bw, GColorBlack, GColorWhite);

  GRect optionsR = GRect(b.origin.x + 5, b.size.h - (9*b.size.h/20) - 5, b.size.w - 10, 4*b.size.h/10);
  graphics_context_set_fill_color(ctx, COLOR_FALLBACK(GColorLightGray, GColorWhite));
  graphics_fill_rect(ctx, optionsR, 10, GCornersAll);
  graphics_context_set_stroke_color(ctx, GColorBlack);
  // Fill Frame
  graphics_context_set_stroke_width(ctx, 3);
  middle_wall = GRect(optionsR.origin.x + 1, optionsR.origin.y + 0, optionsR.size.w - 2, optionsR.size.h - 0);
  inner_wall  = GRect(optionsR.origin.x + 2, optionsR.origin.y + 2, optionsR.size.w - 4, optionsR.size.h - 4);
  graphics_context_set_stroke_color(ctx, COLOR_FALLBACK(GColorDarkGray, GColorLightGray));
  graphics_draw_round_rect(ctx, middle_wall, 10);
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_round_rect(ctx, inner_wall, 10);

  //Draw arrows
  if (s_arrowBlink) {
    graphics_context_set_fill_color(ctx, COLOR_FALLBACK(GColorWhite, GColorLightGray));
    if (s_selectedMenuItem > s_minOption) {
      gpath_move_to(getArrow(kN), GPoint(b.origin.x + 30, b.size.h - (9*b.size.h/20) - 2) );
      gpath_draw_filled(ctx, getArrow(kN));
      gpath_draw_outline(ctx, getArrow(kN));
      gpath_move_to(getArrow(kN), GPoint(b.size.w + b.origin.x - 45, b.size.h - (9*b.size.h/20) - 2) );
      gpath_draw_filled(ctx, getArrow(kN));
      gpath_draw_outline(ctx, getArrow(kN));
    }
    if (s_selectedMenuItem < MENU_ITEMS-1) {
      gpath_move_to(getArrow(kS), GPoint(b.origin.x + 30, b.size.h - (b.size.h/20) - PBL_IF_ROUND_ELSE(24,25) ));
      gpath_draw_filled(ctx, getArrow(kS));
      gpath_draw_outline(ctx, getArrow(kS));
      gpath_move_to(getArrow(kS), GPoint(b.size.w + b.origin.x - 45, b.size.h - (b.size.h/20) - PBL_IF_ROUND_ELSE(24,25) ));
      gpath_draw_filled(ctx, getArrow(kS));
      gpath_draw_outline(ctx, getArrow(kS));
    }
  }

}

void tick(void* data) {
  s_tickTimer = app_timer_register(ANIM_DELAY, tick, NULL);
  static int v = 0;

  if (++s_frame == ANIM_FPS) s_frame = 0;

  if (s_frame == 0 || s_frame % (ANIM_FPS/2) == 0) {
    s_arrowBlink = !s_arrowBlink;
    redraw();
  }

  int desiredOffset = s_selectedMenuItem * 50 * SUB_PIXEL;

  if (desiredOffset < s_textOff) {
    v += GRAVITY * 10;
    s_textOff -= v;
    if (desiredOffset >= s_textOff) { v=0; s_textOff = desiredOffset;}
    redraw();
  } else if (desiredOffset > s_textOff) {
    v += GRAVITY * 10;
    s_textOff += v;
    if (desiredOffset <= s_textOff) { v=0; s_textOff = desiredOffset; }
    redraw();
  }

  if (s_frame == 0) APP_LOG(APP_LOG_LEVEL_DEBUG, "ticking still");
}

void scrollUpdateProc(Layer* thisLayer, GContext *ctx) {
  graphics_context_set_text_color(ctx, TEXT_COLOUR_C);

  for (int i=0 ; i < MENU_ITEMS; ++i) {
    GRect b = layer_get_bounds(thisLayer);
    b.origin.y = 50*i - (s_textOff / SUB_PIXEL) - 3;
    b.size.h = 50;

    draw3DText(ctx, b, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD), s_textMain[i], 1, GTextAlignmentLeft, true, GColorWhite, GColorBlack);

    b.origin.y += 25;
    graphics_context_set_text_color(ctx, GColorBlack);
    graphics_draw_text(ctx, s_textSub[i], fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), b, GTextOverflowModeWordWrap, GTextAlignmentRight, NULL);

  }
}

void setText() {
  strcpy(s_textMain[0], "CONTINUE");
  strcpy(s_textMain[1], "NEW GAME");
  strcpy(s_textMain[2], "TILT INPUT");
  strcpy(s_textMain[3], "BACKLIGHT");
  strcpy(s_textMain[4], "HINTS");

  char buffer[TEXT_BUFFER_SIZE];
  snprintf(buffer, TEXT_BUFFER_SIZE, "Level %i", getCurrentLevel());
  strcpy(s_textSub[0], buffer);
  snprintf(buffer, TEXT_BUFFER_SIZE, "Best: Level %i", getBestLevel());
  strcpy(s_textSub[1], buffer);
  switch (getTiltStatus()) {
    case 0: strcpy(s_textSub[2], "Off"); break;
    case 1: strcpy(s_textSub[2], "High"); break;
    case 2: strcpy(s_textSub[2], "Low"); break;
  }
  switch (getBacklightStatus()) {
    case 0: strcpy(s_textSub[3], "Auto"); break;
    case 1: strcpy(s_textSub[3], "On"); break;
  }
  switch (getHintStatus()) {
    case 0: strcpy(s_textSub[4], "Off"); break;
    case 1: strcpy(s_textSub[4], "On"); break;
  }
  redraw();
}

void splashWindowClickHandler(ClickRecognizerRef recognizer, void *context) {
  ButtonId button = click_recognizer_get_button_id(recognizer);

  if      (BUTTON_ID_UP     == button && s_selectedMenuItem > s_minOption) --s_selectedMenuItem;
  else if (BUTTON_ID_DOWN   == button && s_selectedMenuItem < MENU_ITEMS-1) ++s_selectedMenuItem;
  else if (BUTTON_ID_SELECT == button) {
    switch (s_selectedMenuItem) {
      case 0: newGame(true); pushMainWindow(); return;
      case 1: APP_LOG(APP_LOG_LEVEL_WARNING,"LOAD"); newGame(false); pushMainWindow(); return;
      case 2: setTiltStatus( getTiltStatus() + 1 ); break;
      case 3: setBacklightStatus( getBacklightStatus() + 1 ); break;
      case 4: setHintStatus( getHintStatus() + 1 ); break;
    }
    setText();
  }

}

void splashWindowClickConfigProvider(Window *window) {
  window_single_repeating_click_subscribe(BUTTON_ID_UP  , 100, splashWindowClickHandler);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 100, splashWindowClickHandler);
  window_single_click_subscribe(BUTTON_ID_SELECT, splashWindowClickHandler);
}

void splashWindowLoad(Window* parentWindow) {
  GRect b = layer_get_bounds( window_get_root_layer(parentWindow) );
  int h = b.size.h - STATUS_BAR_LAYER_HEIGHT;

  s_statusBar = status_bar_layer_create();
  layer_add_child(window_get_root_layer(parentWindow), status_bar_layer_get_layer(s_statusBar));
  status_bar_layer_set_separator_mode(s_statusBar, StatusBarLayerSeparatorModeDotted);

  s_mainLayer = layer_create( GRect(0, STATUS_BAR_LAYER_HEIGHT, b.size.w, h) );
  layer_add_child(window_get_root_layer(parentWindow), s_mainLayer);
  layer_set_update_proc(s_mainLayer, splashUpdateProc);

  #ifdef PBL_ROUND
  // Modify for round screen
  b.size.w -= ROUND_REDUCTION * 2;
  b.origin.x += ROUND_REDUCTION;
  b.size.h -= (ROUND_REDUCTION * 3) / 4;
  h -= (ROUND_REDUCTION * 3) / 4;
  #endif

  s_scrollLayer = layer_create( GRect(b.origin.x + 3*BORDER, h - (9*h/20) - 2, b.size.w - 6*BORDER, (4*h/10) - BORDER - 1) );
  layer_add_child(s_mainLayer, s_scrollLayer);
  layer_set_clips(s_scrollLayer, true);
  layer_set_update_proc(s_scrollLayer, scrollUpdateProc);

  for (int i = 0; i < MENU_ITEMS; ++i) {
    s_textMain[i] = malloc(TEXT_BUFFER_SIZE*sizeof(char));
    s_textSub[i] = malloc(TEXT_BUFFER_SIZE*sizeof(char));
  }
  setText();

  if (getGameInProgress() == false) {
    s_selectedMenuItem = 1;
    s_minOption = 1;
  } else {
    s_selectedMenuItem = 0;
    s_minOption = 0;
  }
  s_textOff =  s_selectedMenuItem * 50 * SUB_PIXEL;

  startSplashTick();
}

void startSplashTick() {
  tick(NULL);
}

void stopSplashTick() {
  if (s_tickTimer) app_timer_cancel(s_tickTimer);
  s_tickTimer = NULL;
}

void splashWindowUnload() {
  APP_LOG(APP_LOG_LEVEL_WARNING,"SW unload start");
  layer_destroy(s_scrollLayer);
  layer_destroy(s_mainLayer);
  status_bar_layer_destroy(s_statusBar);
  s_mainLayer = NULL;
  s_scrollLayer = NULL;
  s_statusBar = NULL;
  for (uint8_t i = 0; i < MENU_ITEMS; ++i) {
    free(s_textMain[i]);
    free(s_textSub[i]);
  }
  stopSplashTick();
  APP_LOG(APP_LOG_LEVEL_WARNING,"SW unload done");
}
