#pragma once
#include <pebble.h>

void splashWindowLoad(Window *window);
void splashWindowUnload();

void splashWindowClickConfigProvider(Window *window);

void startSplashTick();
void stopSplashTick();
