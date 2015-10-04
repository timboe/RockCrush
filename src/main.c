#include <pebble.h>
#include <mainWindow.h>

static Window* s_mainWindow;

static void init(void) {
  s_mainWindow = window_create();
  window_set_click_config_provider(s_mainWindow, (ClickConfigProvider) mainWindowClickConfigProvider);
  window_set_background_color(s_mainWindow, GColorWhite);
  window_set_window_handlers(s_mainWindow, (WindowHandlers) {
    .load = mainWindowLoad,
    .unload = mainWindowUnload,
  });
  window_stack_push(s_mainWindow, true);
  //light_enable(1);
}

static void deinit(void) {
  window_destroy(s_mainWindow);
}

int main(void) {
  init();
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", s_mainWindow);
  app_event_loop();
  deinit();
}
