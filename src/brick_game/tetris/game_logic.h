#ifndef _BACKEND_H_
#define _BACKEND_H_
#define FIELD_ROWS 20
#define FIELD_COLS 10
#define MASK_SIZE 4
#define NUM_STATES 6
#define NUM_SIGNALS 10
#define SCORE_FILE_PATH "high_score.dat"
#include "game_interface.h"

typedef void (*action)(void);
typedef enum {
  SIG_START = 0,
  SIG_ROTATE,
  SIG_LEFT,
  SIG_RIGHT,
  SIG_SOFT_DROP,
  SIG_HARD_DROP,
  SIG_PAUSE,
  SIG_QUIT,
  SIG_TICK,
  SIG_NONE
} signals;

typedef enum { P_I = 0, P_O, P_S, P_Z, P_L, P_J, P_T, P_COUNT } TetrominoId;

static const int TETROMINO_MASKS[P_COUNT][4][4] = {
    // I
    {{0, 0, 0, 0}, {0, 0, 0, 0}, {1, 1, 1, 1}, {0, 0, 0, 0}},
    // O
    {{0, 0, 0, 0}, {0, 1, 1, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}},
    // S
    {{0, 0, 0, 0}, {0, 0, 1, 1}, {0, 1, 1, 0}, {0, 0, 0, 0}},
    // Z
    {{0, 0, 0, 0}, {1, 1, 0, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}},
    // L
    {{0, 0, 0, 0}, {0, 0, 1, 0}, {1, 1, 1, 0}, {0, 0, 0, 0}},
    // J
    {{0, 0, 0, 0}, {1, 0, 0, 0}, {1, 1, 1, 0}, {0, 0, 0, 0}},
    // T
    {{0, 0, 0, 0}, {0, 1, 0, 0}, {1, 1, 1, 0}, {0, 0, 0, 0}}};

typedef struct {
  int field[FIELD_ROWS][FIELD_COLS];
  int frame[FIELD_ROWS][FIELD_COLS];  // overlay of active piece
  int* field_rows[FIELD_ROWS];
  int* frame_rows[FIELD_ROWS];

  int next_tetromino_preview[4][4];  // next_tetromino_preview
  int* next_rows[4];

  // тетромино которое падает рн
  TetrominoId cur_tetromino_id;
  TetrominoId next_tetromino_id;  // некст фигурка
  int rotation;                   // 0..3
  int row;                        // верх-лев клетка маски 4на4 на поле
  int col;                        // аналогично

  // meta
  int score;
  int high_score;
  int level;
  int speed;
  int tick;

  int next_gen_counter;  // счетчик фигур цикл
} EngineState;

typedef enum {
  START = 0,
  SPAWN,
  FALLING,
  LOCK,
  GAME_OVER,
  PAUSE
} tetrisState_t;

#endif
