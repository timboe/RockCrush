#include "persistence.h"
#include "globals.h"
#include "mainWindow.h"

#define KEY_HINT 0
#define KEY_BACKLIGHT 1
#define KEY_TILT 2
#define KEY_BOARD 3
#define KEY_SCORE 4

static int hint, backlight, tilt, currentLevel, bestLevel;
bool gameSaved;

static Colour_t s_colourArray[BOARD_PIECES_X * BOARD_PIECES_Y];

void initPersistence() {
  if (persist_exists(KEY_HINT) == 0 || persist_exists(KEY_BACKLIGHT) == 0 || persist_exists(KEY_TILT) == 0) {
    persist_write_int(KEY_HINT, 1);
    persist_write_int(KEY_BACKLIGHT, 1);
    persist_write_int(KEY_TILT, 1);
  }
  setHintStatus(persist_read_int(KEY_HINT));
  setBacklightStatus(persist_read_int(KEY_BACKLIGHT));
  setTiltStatus(persist_read_int(KEY_TILT));

  gameSaved = false;
  if (persist_exists(KEY_BOARD) && persist_exists(KEY_SCORE)) gameSaved = true;
  currentLevel = 0;
  bestLevel = 0;
  if (persist_exists(KEY_SCORE)) {
    Score_t temp;
    persist_read_data(KEY_SCORE, &temp, sizeof(Score_t) );
    currentLevel = temp.level;
    bestLevel = temp.bestLevel;
  }

}

int getHintStatus() { return hint; }
int getBacklightStatus() { return backlight; }
int getTiltStatus() { return tilt; }
bool getGameInProgress() { return gameSaved; }
int getCurrentLevel() { return currentLevel; }
int getBestLevel() { return bestLevel; }

void setHintStatus(int i) {
  if (i >= 2) i = 0;
  hint = i;
  persist_write_int(KEY_HINT, hint);
}

void setBacklightStatus(int i) {
  if (i >= 2) i = 0;
  backlight = i;
  light_enable(backlight);
  persist_write_int(KEY_BACKLIGHT, backlight);
}

void setTiltStatus(int i) {
  if (i >= 3) i = 0;
  tilt = i;
  persist_write_int(KEY_TILT, tilt);
}

void loadGame() {
  int score = persist_read_data(KEY_SCORE, getScore(), sizeof(Score_t) );
  int board = persist_read_data(KEY_BOARD, s_colourArray, BOARD_PIECES*sizeof(Colour_t));
  for (int i = 0; i < BOARD_PIECES; ++i) { getPiece()[i].colour = s_colourArray[i]; APP_LOG(APP_LOG_LEVEL_DEBUG, "set piece %i to %i", i, getPiece()[i].colour); }
  currentLevel = getScore()->level;
  bestLevel = getScore()->bestLevel;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD: board result:%i score result:%i", board, score);
}

void saveGame() {
  for (int i = 0; i < BOARD_PIECES; ++i) { s_colourArray[i] = getPiece()[i].colour; APP_LOG(APP_LOG_LEVEL_DEBUG, "saved piece %i to %i", i, s_colourArray[i] ); }
  int board = persist_write_data(KEY_BOARD, s_colourArray, BOARD_PIECES*sizeof(Colour_t));
  int score = persist_write_data(KEY_SCORE, getScore(), sizeof(Score_t));
  APP_LOG(APP_LOG_LEVEL_DEBUG, "SAVE: Wrote board:%i bytes and score:%i bytes", board, score);
  currentLevel = getScore()->level;
  bestLevel = getScore()->bestLevel;
  gameSaved = true;
}

void endGame() {
  if (persist_exists(KEY_BOARD)) persist_delete(KEY_BOARD);
  int score = persist_write_data(KEY_SCORE, getScore(), sizeof(Score_t));
  currentLevel = getScore()->level;
  bestLevel = getScore()->bestLevel;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "END: wrote score:%i bytes", score);
  gameSaved = false;
}

void reset() {
  if (persist_exists(KEY_HINT)) persist_delete(KEY_HINT);
  if (persist_exists(KEY_BACKLIGHT)) persist_delete(KEY_BACKLIGHT);
  if (persist_exists(KEY_TILT)) persist_delete(KEY_TILT);
  if (persist_exists(KEY_BOARD)) persist_delete(KEY_BOARD);
  if (persist_exists(KEY_SCORE)) persist_delete(KEY_SCORE);
  initPersistence();
}
