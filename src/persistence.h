#pragma once
#include <pebble.h>

void initPersistence();

int getHintStatus();
int getBacklightStatus();
int getTiltStatus();
bool getGameInProgress();
int getCurrentLevel();
int getBestLevel();

void setHintStatus(int i);
void setBacklightStatus(int i);
void setTiltStatus(int i);
void setCurrentLevel();
void setBestLevel();

void loadGame();
void saveGame();
void endGame();
void reset();
