#include "persistence.h"
#include "globals.h"
#include "mainWindow.h"

#define KEY_HINT 0
#define KEY_BACKLIGHT 1
#define KEY_TILT 2
#define KEY_BOARD 3
#define KEY_SCORE 4
#define KEY_BEST_LEVEL 5
#define KEY_CURRENT_LEVEL 6

static Colour_t s_colourArray[BOARD_PIECES_X * BOARD_PIECES_Y];

void initPersistence() {

  if (persist_exists(KEY_HINT) == 0
    || persist_exists(KEY_BACKLIGHT) == 0
    || persist_exists(KEY_TILT) == 0
    || persist_exists(KEY_BEST_LEVEL) == 0
    || persist_exists(KEY_CURRENT_LEVEL) == 0) {
    setHintStatus(1);
    setBacklightStatus(1);
    setTiltStatus(1);
    setBestLevel(1);
    setCurrentLevel(1);
  }

  // Load backlight stats
  light_enable((bool)getBacklightStatus());

}

int getHintStatus() { return persist_read_int(KEY_HINT); }
int getBacklightStatus() { return persist_read_int(KEY_BACKLIGHT); }
int getTiltStatus() { return persist_read_int(KEY_TILT); }
bool getGameInProgress() { return (persist_exists(KEY_BOARD) && persist_exists(KEY_SCORE)); }
int getCurrentLevel() { return persist_read_int(KEY_CURRENT_LEVEL); }
int getBestLevel() { return persist_read_int(KEY_BEST_LEVEL); }

void setHintStatus(int i) {
  if (i < 0) i = 0;
  if (i >= 2) i = 0;
  persist_write_int(KEY_HINT, i);
}

void setBacklightStatus(int i) {
  if (i < 0) i = 0;
  if (i >= 2) i = 0;
  light_enable((bool)i);
  persist_write_int(KEY_BACKLIGHT, i);
}

void setTiltStatus(int i) {
  if (i < 0) i = 0;
  if (i >= 3) i = 0;
  persist_write_int(KEY_TILT, i);
}

void setCurrentLevel(int i) {
  persist_write_int(KEY_CURRENT_LEVEL, i);
}

void setBestLevel(int i) {
  persist_write_int(KEY_BEST_LEVEL, i);
}

void loadGame() {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD GAME");
  int score = persist_read_data(KEY_SCORE, getScore(), sizeof(Score_t) );
  int board = persist_read_data(KEY_BOARD, s_colourArray, BOARD_PIECES*sizeof(Colour_t));
  for (int i = 0; i < BOARD_PIECES; ++i) { getPiece()[i].colour = s_colourArray[i]; /*APP_LOG(APP_LOG_LEVEL_DEBUG, "set piece %i to %i", i, getPiece()[i].colour);*/ }
  APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD: board result:%i score result:%i", board, score);
}

void saveGame() {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "SAVE GAME");
  for (int i = 0; i < BOARD_PIECES; ++i) { s_colourArray[i] = getPiece()[i].colour; /*APP_LOG(APP_LOG_LEVEL_DEBUG, "saved piece %i to %i", i, s_colourArray[i] );*/ }
  int board = persist_write_data(KEY_BOARD, s_colourArray, BOARD_PIECES*sizeof(Colour_t));
  int score = persist_write_data(KEY_SCORE, getScore(), sizeof(Score_t));
  APP_LOG(APP_LOG_LEVEL_DEBUG, "SAVE: Wrote board:%i bytes and score:%i bytes", board, score);
  setCurrentLevel( getScore()->level );
}

void endGame() {
  if ( getScore()->level > getBestLevel() ) {
    setBestLevel( getScore()->level );
  }
  if (persist_exists(KEY_BOARD)) persist_delete(KEY_BOARD);
  if (persist_exists(KEY_SCORE)) persist_delete(KEY_SCORE);
}

void reset() {
  if (persist_exists(KEY_HINT)) persist_delete(KEY_HINT);
  if (persist_exists(KEY_BACKLIGHT)) persist_delete(KEY_BACKLIGHT);
  if (persist_exists(KEY_TILT)) persist_delete(KEY_TILT);
  if (persist_exists(KEY_BOARD)) persist_delete(KEY_BOARD);
  if (persist_exists(KEY_SCORE)) persist_delete(KEY_SCORE);
  if (persist_exists(KEY_BEST_LEVEL)) persist_delete(KEY_BEST_LEVEL);
  if (persist_exists(KEY_CURRENT_LEVEL)) persist_delete(KEY_CURRENT_LEVEL);
}
