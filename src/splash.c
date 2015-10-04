#include <splash.h>

static Layer* s_mainLayer;
static StatusBarLayer* s_statusBar = NULL;

#define TEXT_COLOUR_L GColorFolly
#define TEXT_COLOUR_U GColorOrange
#define TEXT_COLOUR_R GColorMelon
#define TEXT_COLOUR_D GColorDarkCandyAppleRed
#define TEXT_COLOUR_C GColorRed

void draw3DText(GContext *ctx, GRect loc, GFont f, const char* buffer, uint8_t offset, GTextAlignment al, bool BWMode, GColor BWFg, GColor BWBg) {

  if (BWMode) graphics_context_set_text_color(ctx, BWBg);

  // corners
  if (!BWMode) graphics_context_set_text_color(ctx, TEXT_COLOUR_L);
  loc.origin.x -= offset; // CL
  loc.origin.y += offset; // UL
  graphics_draw_text(ctx, buffer, f, loc, GTextOverflowModeWordWrap, al, NULL);

  if (!BWMode) graphics_context_set_text_color(ctx, TEXT_COLOUR_C);
  loc.origin.x += offset; // CU
  graphics_draw_text(ctx, buffer, f, loc, GTextOverflowModeWordWrap, al, NULL);
  loc.origin.x += offset; // RU
  if (!BWMode) graphics_draw_text(ctx, buffer, f, loc, GTextOverflowModeWordWrap, al, NULL);


  if (!BWMode) graphics_context_set_text_color(ctx, TEXT_COLOUR_R);
  loc.origin.y -= offset; // CR
  graphics_draw_text(ctx, buffer, f, loc, GTextOverflowModeWordWrap, al, NULL);
  loc.origin.y -= offset; // DR
  if (!BWMode) graphics_draw_text(ctx, buffer, f, loc, GTextOverflowModeWordWrap, al, NULL);


  if (!BWMode) graphics_context_set_text_color(ctx, TEXT_COLOUR_D);
  loc.origin.x -= offset; // DC
  graphics_draw_text(ctx, buffer, f, loc, GTextOverflowModeWordWrap, al, NULL);
  loc.origin.x -= offset; // DR
  if (!BWMode) graphics_draw_text(ctx, buffer, f, loc, GTextOverflowModeWordWrap, al, NULL);

  if (!BWMode) graphics_context_set_text_color(ctx, TEXT_COLOUR_L);
  loc.origin.y += offset; // CR
  graphics_draw_text(ctx, buffer, f, loc, GTextOverflowModeWordWrap, al, NULL);

  // main
  if (BWMode) graphics_context_set_text_color(ctx, BWFg);
  else graphics_context_set_text_color(ctx, TEXT_COLOUR_C);
  loc.origin.x += offset; // O
  graphics_draw_text(ctx, buffer, f, loc, GTextOverflowModeWordWrap, al, NULL);
}


static void splashUpdateProc(Layer* thisLayer, GContext *ctx) {

  GRect b = layer_get_bounds(thisLayer);
  GRect titleR = GRect(5, 5, b.size.w - 10, 3 * b.size.h / 10);

  // Fill back
  graphics_context_set_fill_color(ctx, GColorLightGray);
  graphics_fill_rect(ctx, titleR, 10, GCornersAll);

  // Fill screws
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_fill_color(ctx, GColorDarkGray);
  GPoint screw_one = GPoint(titleR.origin.x + 1*titleR.size.w/6 - 2, titleR.origin.y + titleR.size.h/3);
  GPoint screw_two = GPoint(titleR.origin.x + 5*titleR.size.w/6 - 2, titleR.origin.y + titleR.size.h/3);
  graphics_fill_circle(ctx, screw_one, 4);
  graphics_fill_circle(ctx, screw_two, 4);
  graphics_draw_circle(ctx, screw_one, 4);
  graphics_draw_circle(ctx, screw_two, 4);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_line(ctx, GPoint(screw_one.x-2, screw_one.y-2), GPoint(screw_one.x+2, screw_one.y+2));
  graphics_draw_line(ctx, GPoint(screw_two.x+3, screw_two.y-1), GPoint(screw_two.x-3, screw_two.y+1));
  graphics_context_set_stroke_width(ctx, 1);

  // Fill borders
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, 5);
  graphics_draw_rect(ctx, GRect(titleR.origin.x, titleR.origin.y, titleR.size.w,  titleR.size.h));
  // Fill Frame
  graphics_context_set_stroke_width(ctx, 3);
  GRect middle_wall = GRect(titleR.origin.x + 1, titleR.origin.y + 0, titleR.size.w - 2, titleR.size.h - 0);
  GRect inner_wall  = GRect(titleR.origin.x + 2, titleR.origin.y + 2, titleR.size.w - 4, titleR.size.h - 4);
  graphics_context_set_stroke_color(ctx, GColorDarkGray);
  graphics_draw_round_rect(ctx, middle_wall, 10);
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_round_rect(ctx, inner_wall, 10);

  // Fill the text

  GRect oreR = GRect(titleR.origin.x + 7,
    titleR.origin.y - 2,
    titleR.size.w-20,
    20);

  GRect swapperR = GRect(titleR.origin.x + 10,
    titleR.origin.y + 15,
    titleR.size.w - 20,
    20);

    static const char txtOre[] = "O r e";
    static const char txtSwapper[] = "Swapper";

  draw3DText(ctx, oreR,     fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK), txtOre, 2, GTextAlignmentLeft, false, GColorBlack, GColorBlack);
  draw3DText(ctx, swapperR, fonts_get_system_font(FONT_KEY_GOTHIC_28), txtSwapper, 2, GTextAlignmentRight, false, GColorBlack, GColorBlack);

}

void splashWindowLoad(Window* parentWindow) {
  GRect b = layer_get_bounds( window_get_root_layer(parentWindow) );

  s_statusBar = status_bar_layer_create();
  layer_add_child(window_get_root_layer(parentWindow), status_bar_layer_get_layer(s_statusBar));
  status_bar_layer_set_separator_mode(s_statusBar, StatusBarLayerSeparatorModeDotted);

  s_mainLayer = layer_create( GRect(0, STATUS_BAR_LAYER_HEIGHT, b.size.w, b.size.h - STATUS_BAR_LAYER_HEIGHT) );
  layer_add_child(window_get_root_layer(parentWindow), s_mainLayer);
  layer_set_update_proc(s_mainLayer, splashUpdateProc);


}

void splashWindowUnload() {
  layer_destroy(s_mainLayer);
  status_bar_layer_destroy(s_statusBar);
  s_mainLayer = NULL;

}
