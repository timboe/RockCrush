#include <pebble.h>
#include "main.h"
#include "mainWindow.h"
#include "splash.h"
#include "globals.h"
#include "persistence.h"

static Window* s_mainWindow;
static Window* s_splashWindow;

static void init(void) {
  s_mainWindow = window_create();
  window_set_click_config_provider(s_mainWindow, (ClickConfigProvider) mainWindowClickConfigProvider);
  window_set_background_color(s_mainWindow, GColorWhite);
  window_set_window_handlers(s_mainWindow, (WindowHandlers) {
    .load = mainWindowLoad,
    .unload = mainWindowUnload,
  });

  s_splashWindow = window_create();
  window_set_click_config_provider(s_splashWindow, (ClickConfigProvider) splashWindowClickConfigProvider);
  window_set_background_color(s_splashWindow, GColorBlack);
  window_set_window_handlers(s_splashWindow, (WindowHandlers) {
    .load = splashWindowLoad,
    .unload = splashWindowUnload,
  });

  initGlobals();
  initPersistence();
}

static void deinit(void) {
  window_destroy(s_mainWindow);
  window_destroy(s_splashWindow);
  destroyGlobals();
}

void pushMainWindow() {
  APP_LOG(APP_LOG_LEVEL_WARNING,"push main");
  window_stack_remove(s_splashWindow, true);
  window_stack_push(s_mainWindow, true);
}

void pushSplashWindow() {
  APP_LOG(APP_LOG_LEVEL_WARNING,"Push splash");
  window_stack_remove(s_mainWindow, true);
  window_stack_push(s_splashWindow, true);
}

int main(void) {
  //reset(); // testing!
  init();
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window:: %p", s_splashWindow);
  window_stack_push(s_splashWindow, true);
  app_event_loop();
  deinit();
}
