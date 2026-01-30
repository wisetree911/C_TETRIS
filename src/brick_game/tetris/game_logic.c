#include "game_logic.h"

static EngineState engine;
static tetrisState_t state = START;
static bool high_score_loaded = false;

static void load_high_score(void);
static void store_high_score(void);
static action fsm_table[NUM_STATES][NUM_SIGNALS];
static void dispatch(signals sig) {
  action a = fsm_table[state][sig];
  if (a) a();
}

static void spawn_next_tetromino(void);
static void move_left(void);
static void move_right(void);
static void move_down(void);
static void rotate(void);
static void exit_game(void);
static void fall(void);
static void start_game(void);
static void toggle_pause(void);
static void drop_figure(void);
static void clear_full_rows_and_count_score(void);

static action fsm_table[NUM_STATES][NUM_SIGNALS] = {
    {start_game, NULL, NULL, NULL, NULL, NULL, toggle_pause, exit_game, NULL,
     NULL},  // START
    {start_game, NULL, NULL, NULL, NULL, NULL, toggle_pause, exit_game, NULL,
     NULL},  // SPAWN
    {start_game, rotate, move_left, move_right, move_down, drop_figure,
     toggle_pause, exit_game, fall, NULL},  // FALLING
    {start_game, NULL, NULL, NULL, NULL, NULL, toggle_pause, exit_game, NULL,
     NULL},  // LOCK
    {start_game, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
     NULL},  // GAME_OVER
    {start_game, NULL, NULL, NULL, NULL, NULL, toggle_pause, NULL, NULL,
     NULL}  // PAUSE
};

static void load_high_score(void) {
  FILE* file = fopen(SCORE_FILE_PATH, "r");
  int stored = 0;
  if (!file) {
    engine.high_score = 0;
    store_high_score();
  } else {
    if (fscanf(file, "%d", &stored) != 1 || stored < 0) stored = 0;
    fclose(file);
    engine.high_score = stored;
  }
}

static void store_high_score(void) {
  FILE* file = fopen(SCORE_FILE_PATH, "w");
  if (file) {
    fprintf(file, "%d\n", engine.high_score);
    fclose(file);
  }
}

static void init_rows() {
  for (int r = 0; r < FIELD_ROWS; ++r) {
    engine.field_rows[r] = engine.field[r];
    engine.frame_rows[r] = engine.frame[r];
  }
  for (int r = 0; r < 4; ++r)
    engine.next_rows[r] = engine.next_tetromino_preview[r];
}
static void clear_field(int a[FIELD_ROWS][FIELD_COLS]) {
  for (int r = 0; r < FIELD_ROWS; ++r) memset(a[r], 0, sizeof(a[r]));
}
static void clear_next() {
  for (int r = 0; r < 4; ++r)
    memset(engine.next_tetromino_preview[r], 0,
           sizeof(engine.next_tetromino_preview[r]));
}

static void reset_state() {
  int saved_high = engine.high_score;
  memset(&engine, 0, sizeof(engine));
  init_rows();
  clear_field(engine.field);
  clear_field(engine.frame);
  clear_next();
  engine.level = 1;
  engine.speed = 12;
  engine.tick = 0;
  engine.next_gen_counter = 0;
  engine.next_tetromino_id = (TetrominoId)0;
  engine.high_score = saved_high;
}
static int is_cell_filled_in_rotated_mask(
    TetrominoId id, int rotation, int row,
    int col) {  // функция проверяет будет ли занята клетка [r, c] в массиве
                // 4на4 при повороте на rot
  // разворот на 90 (направо) по чс
  int src_row = row, src_col = col;
  if (rotation == 1) {
    src_row = 3 - col;
    src_col = row;
  } else if (rotation == 2) {
    src_row = 3 - row;
    src_col = 3 - col;
  } else if (rotation == 3) {
    src_row = col;
    src_col = 3 - row;
  }
  return TETROMINO_MASKS[id][src_row]
                        [src_col];  // вернет 1 если клетка занята, 0 если нет
}
static int can_place_tetromino_in_field(
    TetrominoId id, int rot, int row,
    int col) {  // проверяет можно ли поставить фигуру на главном поле, row, col
                // - позиция на поле, координаты верхнего левого угла 4на4 маски
                // фигуры
  int can_place = 1;
  for (int r = 0; r < 4 && can_place; ++r) {
    for (int c = 0; c < 4 && can_place; ++c) {
      if (!is_cell_filled_in_rotated_mask(id, rot, r, c))
        continue;  // скип если фигура в точку не попадает
      int field_row =
          row + r;  // если фигура в точку попадает то считает ее коорд на поле
      int field_col = col + c;
      if (field_row < 0 || field_row >= FIELD_ROWS || field_col < 0 ||
          field_col >= FIELD_COLS) {
        can_place =
            0;  // если фигура выходит за границы то сразу 0 - нельзя поставить
      } else if (engine.field[field_row][field_col]) {
        can_place = 0;  // если в этой точке на поле что-то есть то тоже сразу 0
                        // - нельзя поставить
      }
    }
  }
  return can_place;
}

static void update_frame_overlay() {
  for (int r = 0; r < FIELD_ROWS; ++r)
    memcpy(engine.frame[r], engine.field[r],
           sizeof(engine.frame[r]));  // копирует поле в фрейм
  for (int r = 0; r < 4; ++r) {
    for (int c = 0; c < 4; ++c) {
      if (!is_cell_filled_in_rotated_mask(engine.cur_tetromino_id,
                                          engine.rotation, r, c))
        continue;  // скип если фигура в точку не попадает
      int field_row = engine.row + r;
      int field_col = engine.col + c;
      if (field_row >= 0 && field_row < FIELD_ROWS && field_col >= 0 &&
          field_col < FIELD_COLS)
        engine.frame[field_row][field_col] =
            (int)engine.cur_tetromino_id +
            1;  // нет проверки на коллизии тк вызывается только при условии
                // can_place_tetromино_in_field()
    }
  }
}
// LOCK
static void
lock_active_tetromino_into_field() {  // выполняется после неудачной попытки
                                      // опустить фигуру вниз, останавливает
                                      // фигуру и вписывает ее в engine.field
  for (int r = 0; r < 4; ++r) {
    for (int c = 0; c < 4; ++c) {
      if (!is_cell_filled_in_rotated_mask(engine.cur_tetromino_id,
                                          engine.rotation, r, c))
        continue;
      int fr = engine.row + r;
      int fc = engine.col + c;
      if (fr >= 0 && fr < FIELD_ROWS && fc >= 0 && fc < FIELD_COLS)
        engine.field[fr][fc] =
            (int)engine.cur_tetromino_id + 1;  // по сути излишне но пох
    }
  }
  state = SPAWN;
  clear_full_rows_and_count_score();
  spawn_next_tetromino();
}
static void clear_full_rows_and_count_score() {
  int cleared = 0;  // сколько строк заполнены
  for (int r = 0; r < FIELD_ROWS; ++r) {
    int full = 1;
    for (int c = 0; c < FIELD_COLS; ++c)
      if (engine.field[r][c] == 0) {
        full = 0;
        break;
      }
    if (full) {  // если да то свдигает все вышестоящие строки на 1 вниз
      cleared++;
      for (int rr = r; rr > 0; --rr)
        memcpy(engine.field[rr], engine.field[rr - 1],
               sizeof(engine.field[rr]));
      // memset(engine.field[0], 0, sizeof(engine.field[0])); // самую верхнюю
      // строку зануляет
      for (int c = 0; c < FIELD_COLS; ++c) engine.field[0][c] = 0;
    }
  }
  if (cleared == 1)
    engine.score += 100;
  else if (cleared == 2)
    engine.score += 300;
  else if (cleared == 3)
    engine.score += 700;
  else if (cleared >= 4)
    engine.score += 1500;

  int new_level = engine.score / 600 + 1;
  if (new_level > 10) new_level = 10;
  if (new_level != engine.level) {
    engine.level = new_level;
    int new_speed = 12 - (engine.level - 1);
    if (new_speed < 2) new_speed = 2;
    engine.speed = new_speed;
  }
  if (engine.score > engine.high_score) {
    engine.high_score = engine.score;
    store_high_score();
  }
}
static void generate_next_preview(TetrominoId pid) {
  clear_next();  // очистка старого превью
  for (int r = 0; r < 4; ++r)
    for (int c = 0; c < 4; ++c)
      engine.next_tetromino_preview[r][c] =
          TETROMINO_MASKS[pid][r][c] ? (int)pid + 1 : 0;  // заполнение нового
}
static TetrominoId next_tetromino_id() {  // смотрит текущую фигуру и возвращает
                                          // айди следущей (цикл)
  TetrominoId id = (TetrominoId)(engine.next_gen_counter % P_COUNT);
  engine.next_gen_counter++;
  return id;
}
static void place_current_tetromino(
    TetrominoId pid) {  // выставляет текущую фигуру id в стартовой позиции
  engine.cur_tetromino_id = pid;
  engine.rotation = 0;
  engine.row = 0;
  engine.col = (FIELD_COLS - MASK_SIZE) / 2;
}
static int can_fall() {
  return can_place_tetromino_in_field(engine.cur_tetromino_id, engine.rotation,
                                      engine.row + 1, engine.col);
}
// SPAWN
static void
spawn_next_tetromino() {  // если нужно — бутстрапит превью; делает текущей
                          // фигуру из превью; генерирует новую “следующую” и
                          // перерисовывает превью; проверяет can_place — при
                          // неудаче ставит game_over.
  if (engine.next_gen_counter == 0 &&
      engine.next_tetromino_id == 0) {  // бутстрапим если нихера нет
    engine.next_tetromino_id = next_tetromino_id();
    generate_next_preview(engine.next_tetromino_id);
  }
  place_current_tetromino(engine.next_tetromino_id);
  state = FALLING;
  // заготовка под некст
  engine.next_tetromino_id = next_tetromino_id();
  generate_next_preview(engine.next_tetromino_id);
  if (!can_place_tetromino_in_field(
          engine.cur_tetromino_id, engine.rotation, engine.row,
          engine.col)) {  // если фиугра не влезла при спавне значит геймовер
    state = GAME_OVER;    // лучше чекать в начале
  }
}
// MOVE
static void move_left() {
  if (can_place_tetromino_in_field(engine.cur_tetromino_id, engine.rotation,
                                   engine.row, engine.col - 1))
    engine.col--;
}
static void move_right() {
  if (can_place_tetromino_in_field(engine.cur_tetromino_id, engine.rotation,
                                   engine.row, engine.col + 1))
    engine.col++;
}
static void move_down() {
  if (can_place_tetromino_in_field(engine.cur_tetromino_id, engine.rotation,
                                   engine.row + 1, engine.col)) {
    engine.row++;
  } else {
    state = LOCK;
    lock_active_tetromino_into_field();
  }
}
static void drop_figure() {
  while (can_place_tetromino_in_field(engine.cur_tetromino_id, engine.rotation,
                                      engine.row + 1, engine.col)) {
    engine.row++;
  }
  state = LOCK;
  lock_active_tetromino_into_field();
}
static void rotate() {
  int new_rot = (engine.rotation + 1) & 3;  // mod 4
  if (can_place_tetromino_in_field(engine.cur_tetromino_id, new_rot, engine.row,
                                   engine.col)) {
    engine.rotation = new_rot;
  }
}
// PAUSE
static void toggle_pause() {
  if (state == PAUSE)
    state = FALLING;
  else
    state = PAUSE;
}
// GAME_OVER
static void exit_game() {
  store_high_score();
  state = GAME_OVER;
}
// FALL
static void fall() {
  state = FALLING;
  if (can_fall())
    engine.row++;
  else {
    state = LOCK;
    lock_active_tetromino_into_field();
  }
}

//  START
static void start_game() {
  if (!high_score_loaded) {
    load_high_score();
    high_score_loaded = true;
  }
  state = START;
  reset_state();
  state = SPAWN;  // SPAWN
  spawn_next_tetromino();
}

void userInput(UserAction_t action, bool hold) {
  (void)hold;
  signals sig = SIG_NONE;
  switch (action) {
    case Start:
      sig = SIG_START;
      break;
    case Pause:
      sig = SIG_PAUSE;
      break;
    case Terminate:
      sig = SIG_QUIT;
      break;
    case Left:
      sig = SIG_LEFT;
      break;
    case Right:
      sig = SIG_RIGHT;
      break;
    case Down:
      sig = SIG_HARD_DROP;
      break;
    case Action:
      sig = SIG_ROTATE;
      break;
    case Up:
      sig = SIG_NONE;
      break;
    default:
      break;
  }
  if (sig != SIG_NONE) dispatch(sig);
}

GameInfo_t updateCurrentState() {
  if (state == FALLING) {
    engine.tick++;
    if (engine.tick >= engine.speed) {
      engine.tick = 0;
      dispatch(SIG_TICK);
    }
  }
  update_frame_overlay();
  GameInfo_t info;
  info.field = engine.frame_rows;
  info.next = engine.next_rows;
  info.score = engine.score;
  info.high_score = engine.high_score;
  info.level = engine.level;
  info.speed = engine.speed;
  int ui_state = 0;
  if (state == PAUSE) {
    ui_state = 1;
  } else if (state == GAME_OVER) {
    ui_state = 2;
  }
  info.pause = ui_state;
  return info;
}
