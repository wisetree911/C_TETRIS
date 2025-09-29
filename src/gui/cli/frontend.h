#include <ncurses.h>

#include "../../brick_game/tetris/game_interface.h"
#define ONE_PIXEL_WIDTH 4
#define ONE_PIXEL_HEIGHT 2
#define HIGHT_IN_PIXELS 20
#define WIDTH_IN_PIXELS 10
#define DEFAULT_CHAR '.'
#define SIDEBAR_DEFAULT_CHAR ' '

#define SIDEBAR_TOP_PIX 0
#define SIDEBAR_LEFT_PIX 11
#define SIDEBAR_HEIGHT_IN_PIX 20
#define SIDEBAR_WIDTH_IN_PIX 6
#define PREVIEW_PIX_TOP 1
#define PREVIEW_PIX_LEFT WIDTH_IN_PIXELS + 2

#define HIGHT_IN_CHARS ONE_PIXEL_HEIGHT* HIGHT_IN_PIXELS
#define WIDTH_IN_CHARS ONE_PIXEL_WIDTH* WIDTH_IN_PIXELS
#define WIN_INIT(time)    \
  {                       \
    initscr();            \
    noecho();             \
    curs_set(0);          \
    keypad(stdscr, TRUE); \
    timeout(time);        \
  }
extern char field[WIDTH_IN_CHARS][HIGHT_IN_CHARS];

void print_field(GameInfo_t* info);
void print_menu();
void print_game_over_prompt(void);
void init_colors(void);
