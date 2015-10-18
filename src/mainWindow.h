#pragma once
#include <pebble.h>
#include "globals.h"

void mainWindowLoad(Window *window);
void mainWindowUnload();

void mainWindowClickConfigProvider(Window *window);

Score_t* getScore();
Piece_t* getPiece();

void newGame(bool doLoadGame);

void startGameTick();
void stopGameTick();
