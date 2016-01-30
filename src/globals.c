#include "globals.h"

static GPath* s_shapes[N_COLOURS] = {NULL};
static GPath* s_arrows[N_CARDINAL] = {NULL};

GPath* getShape(int n) { return s_shapes[n]; }
GPath* getArrow(int n) { return s_arrows[n]; }

const GPathInfo shapeRed = {
  .num_points = 7,
  .points = (GPoint []) {{2, 12}, {13, 13}, {12, 3}, {9, 7}, {7, 5}, {5, 9}, {2, 2}}
};

const GPathInfo shapePink = {
  .num_points = 10,
  .points = (GPoint []) {{4, 13}, {13, 13}, {13, 8}, {13, 6}, {9, 2}, {6, 3}, {8, 6}, {7, 8}, {4, 5}, {2, 7}}
};

const GPathInfo shapeOrange = {
  .num_points = 8,
  .points = (GPoint []) {{2, 8}, {6, 6}, {8, 2}, {10, 6}, {13, 8}, {10, 10}, {8, 12}, {6, 10}}
};

const GPathInfo shapeGreen = {
  .num_points = 5,
  .points = (GPoint []) {{2, 3}, {5, 13}, {7, 13}, {13, 9}, {11, 2}}
};

const GPathInfo shapeYellow = {
  .num_points = 4,
  .points = (GPoint []) {{2, 10}, {11, 2}, {13, 6}, {4, 13}}
};

const GPathInfo shapeBlue = {
  .num_points = 5,
  .points = (GPoint []) {{2, 2}, {7, 5}, {13, 3}, {9, 13}, {2, 11}}
};

const GPathInfo shapePurple = {
  .num_points = 3,
  .points = (GPoint []) {{3, 2}, {13, 8}, {3, 13}}
};

const GPathInfo shapeBrown = {
  .num_points = 4,
  .points = (GPoint []) {{2, 13}, {7, 1}, {13, 13}, {8, 10}}
};

const GPathInfo shapeWhite = {
  .num_points = 8,
  .points = (GPoint []) {{5, 2}, {10, 2}, {13, 5}, {13, 10}, {10, 13}, {5, 13}, {2, 10}, {2, 5}}
};

const GPathInfo shapeN = {
  .num_points = 4,
  .points = (GPoint []) {{0, -1}, {15, -1}, {9, -8}, {6, -8}}
};

const GPathInfo shapeE = {
  .num_points = 4,
  .points = (GPoint []) {{16, 0}, {16, 15}, {23, 9}, {23, 6}}
};

const GPathInfo shapeS = {
  .num_points = 4,
  .points = (GPoint []) {{0, 16}, {15, 16}, {9, 23}, {6, 23}}
};

const GPathInfo shapeW = {
  .num_points = 4,
  .points = (GPoint []) {{-1, 0}, {-1, 15}, {-8, 9}, {-8, 6}}
};

GColor COLOURS[N_COLOURS];

void initGlobals() {
  s_shapes[kRed] = gpath_create(&shapeRed);
  s_shapes[kOrange] = gpath_create(&shapeOrange);
  s_shapes[kGreen] = gpath_create(&shapeGreen);
  s_shapes[kWhite] = gpath_create(&shapeWhite);
  s_shapes[kBlack] = gpath_create(&shapeWhite);
  s_shapes[kPink] = gpath_create(&shapePink);
  s_shapes[kYellow] = gpath_create(&shapeYellow);
  s_shapes[kPurple] = gpath_create(&shapePurple);
  s_shapes[kBlue] = gpath_create(&shapeBlue);
  s_shapes[kBrown] = gpath_create(&shapeBrown);

  COLOURS[kRed] = COLOR_FALLBACK(GColorRed, GColorBlack);
  COLOURS[kOrange] = COLOR_FALLBACK(GColorOrange, GColorLightGray);
  COLOURS[kGreen] = COLOR_FALLBACK(GColorGreen, GColorWhite);
  COLOURS[kWhite] = COLOR_FALLBACK(GColorWhite, GColorWhite);
  COLOURS[kBlack] = COLOR_FALLBACK(GColorBlack, GColorBlack);
  COLOURS[kPink] = COLOR_FALLBACK(GColorMagenta, GColorBlack);
  COLOURS[kYellow] = COLOR_FALLBACK(GColorYellow, GColorWhite);
  COLOURS[kPurple] = COLOR_FALLBACK(GColorCyan, GColorWhite);
  COLOURS[kBlue] = COLOR_FALLBACK(GColorElectricUltramarine, GColorLightGray);
  COLOURS[kBrown] = COLOR_FALLBACK(GColorImperialPurple, GColorBlack);

  s_arrows[kN] = gpath_create(&shapeN);
  s_arrows[kE] = gpath_create(&shapeE);
  s_arrows[kS] = gpath_create(&shapeS);
  s_arrows[kW] = gpath_create(&shapeW);
}

void destroyGlobals() {
  for (int i=0; i < N_COLOURS; ++i) {
    gpath_destroy(s_shapes[i]);
    if (i >= N_CARDINAL) continue;
    gpath_destroy(s_arrows[i]);
  }
}

void draw3DText(GContext *ctx, GRect loc, GFont f, const char* buffer, uint8_t offset, GTextAlignment al, bool BWMode, GColor BWFg, GColor BWBg) {

  if (BWMode) graphics_context_set_text_color(ctx, BWBg);

  // corners
  if (!BWMode) graphics_context_set_text_color(ctx, TEXT_COLOUR_L);
  loc.origin.x -= offset; // CL
  loc.origin.y += offset; // UL
  if (!BWMode) graphics_draw_text(ctx, buffer, f, loc, GTextOverflowModeWordWrap, al, NULL);

  if (!BWMode) graphics_context_set_text_color(ctx, TEXT_COLOUR_U);
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
