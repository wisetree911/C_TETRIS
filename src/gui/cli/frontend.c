#include "frontend.h"

#define TETROMINO_COUNT 7
#define CUSTOM_COLOR_BASE 16

enum {
  PAIR_TETROMINO_BASE = 1,
  PAIR_BORDER = PAIR_TETROMINO_BASE + TETROMINO_COUNT,
  PAIR_FIELD_BG,
  PAIR_SIDEBAR_BG,
  PAIR_SIDEBAR_HEADER,
  PAIR_SIDEBAR_PREVIEW
};

typedef struct {
  short border;
  short field_bg;
  short sidebar_bg;
  short sidebar_header;
  short sidebar_preview;
  short tetromino[TETROMINO_COUNT];
} ColorTheme;

typedef struct {
  short r;
  short g;
  short b;
  short fallback;
} PastelSpec;

char field[WIDTH_IN_CHARS][HIGHT_IN_CHARS];

static ColorTheme theme = {
    .border = PAIR_BORDER,
    .field_bg = PAIR_FIELD_BG,
    .sidebar_bg = PAIR_SIDEBAR_BG,
    .sidebar_header = PAIR_SIDEBAR_HEADER,
    .sidebar_preview = PAIR_SIDEBAR_PREVIEW,
    .tetromino = {PAIR_TETROMINO_BASE + 0, PAIR_TETROMINO_BASE + 1,
                  PAIR_TETROMINO_BASE + 2, PAIR_TETROMINO_BASE + 3,
                  PAIR_TETROMINO_BASE + 4, PAIR_TETROMINO_BASE + 5,
                  PAIR_TETROMINO_BASE + 6}};

static bool colors_enabled = false;

static void configure_theme(void);
static short setup_pastel_color(short slot_index, const PastelSpec* spec);
static void paint_cell_block(int top, int left, short pair, char fallback_char);
static void draw_playfield(const GameInfo_t* info);
static void draw_playfield_frame(void);
static void draw_sidebar(const GameInfo_t* info);
static void draw_sidebar_background(void);
static void draw_preview(const GameInfo_t* info);
static void draw_sidebar_text(const GameInfo_t* info);
static void draw_sidebar_text_line(int row_y, const char* text, short pair);
static void draw_sidebar_frame(void);
static short tetromino_pair_from_cell(int cell);
static int field_cell_value(const GameInfo_t* info, int row, int col);
static int preview_cell_value(const GameInfo_t* info, int row, int col);

void init_colors(void) {
  colors_enabled = false;
  if (!has_colors()) return;
  if (start_color() == ERR) return;
#ifdef NCURSES_VERSION
  use_default_colors();
#endif
  configure_theme();
  colors_enabled = true;
}

static void configure_theme(void) {
  static const PastelSpec tetromino_specs[TETROMINO_COUNT] = {
      {700, 900, 1000, COLOR_CYAN},    /* I */
      {1000, 950, 700, COLOR_YELLOW},  /* O */
      {700, 950, 750, COLOR_GREEN},    /* S */
      {1000, 700, 700, COLOR_RED},     /* Z */
      {1000, 800, 600, COLOR_MAGENTA}, /* L */
      {750, 800, 1000, COLOR_BLUE},    /* J */
      {900, 750, 1000, COLOR_MAGENTA}  /* T */
  };
  static const PastelSpec field_spec = {1000, 1000, 1000, COLOR_WHITE};

  short color_slot = CUSTOM_COLOR_BASE;
  for (int i = 0; i < TETROMINO_COUNT; ++i) {
    short bg = setup_pastel_color(color_slot++, &tetromino_specs[i]);
    init_pair(theme.tetromino[i], COLOR_BLACK, bg);
  }

  short field_bg = setup_pastel_color(color_slot++, &field_spec);
  init_pair(theme.field_bg, COLOR_BLACK, field_bg);

  init_pair(theme.border, COLOR_BLACK, COLOR_BLACK);
  init_pair(theme.sidebar_bg, COLOR_BLACK, COLOR_WHITE);
  init_pair(theme.sidebar_header, COLOR_BLACK, COLOR_WHITE);
  init_pair(theme.sidebar_preview, COLOR_BLACK, COLOR_WHITE);
}

static short setup_pastel_color(short slot_index, const PastelSpec* spec) {
  if (can_change_color() && COLORS > slot_index) {
    init_color(slot_index, spec->r, spec->g, spec->b);
    return slot_index;
  }
  return spec->fallback;
}

static void paint_cell_block(int top, int left, short pair,
                             char fallback_char) {
  chtype glyph = fallback_char;
  if (colors_enabled && pair > 0) glyph = ' ' | COLOR_PAIR(pair);
  for (int dy = 0; dy < ONE_PIXEL_HEIGHT; ++dy) {
    for (int dx = 0; dx < ONE_PIXEL_WIDTH; ++dx) {
      mvaddch(top + dy, left + dx, glyph);
    }
  }
}

static void draw_playfield(const GameInfo_t* info) {
  for (int r = 0; r < HIGHT_IN_PIXELS; ++r) {
    for (int c = 0; c < WIDTH_IN_PIXELS; ++c) {
      int cell = field_cell_value(info, r, c);
      short pair = 0;
      char fallback = ' ';
      if (cell > 0) {
        pair = colors_enabled ? tetromino_pair_from_cell(cell) : 0;
        fallback = colors_enabled ? ' ' : DEFAULT_CHAR;
      } else {
        pair = colors_enabled ? theme.field_bg : 0;
        fallback = ' ';
      }
      paint_cell_block(r * ONE_PIXEL_HEIGHT, c * ONE_PIXEL_WIDTH, pair,
                       fallback);
    }
  }
}

static void draw_playfield_frame(void) {
  chtype border_ch = colors_enabled ? ' ' | COLOR_PAIR(theme.border) : '$';
  for (int x = 0; x < WIDTH_IN_CHARS + 1; ++x) {
    mvaddch(0, x, border_ch);
    mvaddch(HIGHT_IN_CHARS, x, border_ch);
  }
  for (int y = 0; y <= HIGHT_IN_CHARS; ++y) {
    mvaddch(y, 0, border_ch);
    mvaddch(y, WIDTH_IN_CHARS, border_ch);
  }
}

static void draw_sidebar(const GameInfo_t* info) {
  draw_sidebar_background();
  draw_preview(info);
  draw_sidebar_text(info);
  draw_sidebar_frame();
}

static void draw_sidebar_background(void) {
  for (int r = SIDEBAR_TOP_PIX; r < SIDEBAR_HEIGHT_IN_PIX; ++r) {
    for (int c = SIDEBAR_LEFT_PIX; c < SIDEBAR_LEFT_PIX + SIDEBAR_WIDTH_IN_PIX;
         ++c) {
      short pair = colors_enabled ? theme.sidebar_bg : 0;
      paint_cell_block(r * ONE_PIXEL_HEIGHT, c * ONE_PIXEL_WIDTH, pair,
                       SIDEBAR_DEFAULT_CHAR);
    }
  }
}

static void draw_preview(const GameInfo_t* info) {
  for (int r = 0; r < 4; ++r) {
    for (int c = 0; c < 4; ++c) {
      int cell = preview_cell_value(info, r, c);
      short pair = 0;
      char fallback = SIDEBAR_DEFAULT_CHAR;
      if (cell > 0) {
        pair = colors_enabled ? tetromino_pair_from_cell(cell) : 0;
        fallback = colors_enabled ? ' ' : DEFAULT_CHAR;
      } else if (colors_enabled) {
        pair = theme.sidebar_preview;
      }
      int top = (PREVIEW_PIX_TOP + r) * ONE_PIXEL_HEIGHT;
      int left = (PREVIEW_PIX_LEFT + c) * ONE_PIXEL_WIDTH;
      paint_cell_block(top, left, pair, fallback);
    }
  }
}

static void draw_sidebar_text(const GameInfo_t* info) {
  int score = info ? info->score : 0;
  int high = info ? info->high_score : 0;
  int level = info ? info->level : 0;
  int speed = info ? info->speed : 0;
  int pause_state = info ? info->pause : 0;

  const char* pause_text = "OFF";
  if (pause_state == 1)
    pause_text = "ON";
  else if (pause_state == 2)
    pause_text = "OVER";

  char buffer[64];
  draw_sidebar_text_line(1, "NEXT", theme.sidebar_header);
  draw_sidebar_text_line(2, "", theme.sidebar_bg);

  draw_sidebar_text_line(12, "STATS", theme.sidebar_header);
  snprintf(buffer, sizeof(buffer), "SCORE: %d", score);
  draw_sidebar_text_line(14, buffer, theme.sidebar_bg);
  snprintf(buffer, sizeof(buffer), "HIGH : %d", high);
  draw_sidebar_text_line(15, buffer, theme.sidebar_bg);
  snprintf(buffer, sizeof(buffer), "LEVEL: %d", level);
  draw_sidebar_text_line(17, buffer, theme.sidebar_bg);
  snprintf(buffer, sizeof(buffer), "SPEED: %d", speed);
  draw_sidebar_text_line(18, buffer, theme.sidebar_bg);
  snprintf(buffer, sizeof(buffer), "PAUSE: %s", pause_text);
  draw_sidebar_text_line(20, buffer, theme.sidebar_bg);

  draw_sidebar_text_line(24, pause_state == 2 ? "GAME OVER" : "",
                         theme.sidebar_header);

  draw_sidebar_text_line(26, "CONTROLS", theme.sidebar_header);
  draw_sidebar_text_line(28, "SPACE ROTATE", theme.sidebar_bg);
  draw_sidebar_text_line(29, "DOWN  DROP", theme.sidebar_bg);
  draw_sidebar_text_line(30, "P PAUSE / R RESET", theme.sidebar_bg);
  draw_sidebar_text_line(31, "Q QUIT", theme.sidebar_bg);
}

static void draw_sidebar_text_line(int row_y, const char* text, short pair) {
  int text_left = SIDEBAR_LEFT_PIX * ONE_PIXEL_WIDTH + 1;
  int text_width = SIDEBAR_WIDTH_IN_PIX * ONE_PIXEL_WIDTH - 2;
  chtype fill = ' ';
  if (colors_enabled && pair > 0) fill |= COLOR_PAIR(pair);
  mvhline(row_y, text_left, fill, text_width);
  if (text) {
    if (colors_enabled && pair > 0) {
      attron(COLOR_PAIR(pair));
      mvaddnstr(row_y, text_left, text, text_width);
      attroff(COLOR_PAIR(pair));
    } else {
      mvaddnstr(row_y, text_left, text, text_width);
    }
  }
}

static void draw_sidebar_frame(void) {
  chtype border_ch = colors_enabled ? ' ' | COLOR_PAIR(theme.border) : '$';
  int sidebar_left = SIDEBAR_LEFT_PIX * ONE_PIXEL_WIDTH - 1;
  int sidebar_right = SIDEBAR_LEFT_PIX * ONE_PIXEL_WIDTH +
                      SIDEBAR_WIDTH_IN_PIX * ONE_PIXEL_WIDTH;
  for (int x = WIDTH_IN_CHARS; x <= sidebar_right; ++x) {
    mvaddch(0, x, border_ch);
    mvaddch(HIGHT_IN_CHARS, x, border_ch);
  }
  for (int y = 0; y <= HIGHT_IN_CHARS; ++y) {
    mvaddch(y, sidebar_left, border_ch);
    mvaddch(y, sidebar_right, border_ch);
  }
}

static short tetromino_pair_from_cell(int cell) {
  int index = cell - 1;
  if (index < 0 || index >= TETROMINO_COUNT) return theme.field_bg;
  return theme.tetromino[index];
}

static int field_cell_value(const GameInfo_t* info, int row, int col) {
  int value = 0;
  if (info && info->field) value = info->field[row][col];
  return value;
}

static int preview_cell_value(const GameInfo_t* info, int row, int col) {
  int value = 0;
  if (info && info->next) value = info->next[row][col];
  return value;
}

void print_field(GameInfo_t* info) {
  draw_playfield(info);
  draw_playfield_frame();
  draw_sidebar(info);
  refresh();
}

void print_menu() {
  GameInfo_t empty = {0};
  draw_playfield(&empty);
  draw_playfield_frame();
  draw_sidebar(&empty);
  const char* text = "Press Enter to start.";
  int y = HIGHT_IN_CHARS / 2;
  int x = (WIDTH_IN_CHARS - (int)strlen(text)) / 2;
  if (x < 1) x = 1;
  mvaddnstr(y, x, text, (int)strlen(text));
  refresh();
}

void print_game_over_prompt(void) {
  const char* msg = "Game Over! Press R to restart or Q to quit.";
  int y = HIGHT_IN_CHARS / 2;
  int x = (WIDTH_IN_CHARS - (int)strlen(msg)) / 2;
  if (x < 1) x = 1;
  mvaddnstr(y, x, msg, (int)strlen(msg));
  refresh();
}
