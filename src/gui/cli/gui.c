#include <locale.h>
#include <stdbool.h>

#include "../../brick_game/tetris/game_interface.h"
#include "frontend.h"

typedef enum {
  NOT_VALUABLE_INPUT = 0,
  START_INPUT,
  QUIT_INPUT,
  PAUSE_INPUT,
  RESTART_INPUT,
} InputSignals_t;

static void game_loop(void);

static int parse_input(void) {
  int result = NOT_VALUABLE_INPUT;
  int ch = getch();
  if (ch == ERR) return result;

  if (ch == 'q' || ch == 'Q') {
    userInput(Terminate, false);
    return QUIT_INPUT;
  }

  switch (ch) {
    case 'p':
    case 'P':
      userInput(Pause, false);
      result = PAUSE_INPUT;
      break;
    case 'r':
    case 'R':
      userInput(Start, false);
      result = RESTART_INPUT;
      break;
    case KEY_LEFT:
      userInput(Left, false);
      break;
    case KEY_RIGHT:
      userInput(Right, false);
      break;
    case KEY_DOWN:
      userInput(Down, false);
      break;
    case ' ':
      userInput(Action, false);
      break;
    default:
      break;
  }
  return result;
}

static int main_menu_status(void) {
  int status = NOT_VALUABLE_INPUT;
  bool waiting = true;
  while (waiting) {
    print_menu();
    int ch = getch();
    if (ch == '\n') {
      status = START_INPUT;
      waiting = false;
    } else if (ch == 'q' || ch == 'Q') {
      status = QUIT_INPUT;
      waiting = false;
    }
  }
  return status;
}

static void wait_for_restart_or_exit(void) {
  for (;;) {
    int ch = getch();
    if (ch == 'r' || ch == 'R') {
      userInput(Terminate, false);
      game_loop();
      return;
    }
    if (ch == 'q' || ch == 'Q' || ch == 27) {
      userInput(Terminate, false);
      return;
    }
  }
}

static void game_loop(void) {
  userInput(Start, false);
  for (;;) {
    refresh();
    GameInfo_t info = updateCurrentState();
    print_field(&info);
    if (info.pause == 2) {
      print_game_over_prompt();
      wait_for_restart_or_exit();
      return;
    }
    if (parse_input() == QUIT_INPUT) return;
  }
}

int main(void) {
  WIN_INIT(50);
  init_colors();
  setlocale(LC_ALL, "");

  int menu_status = main_menu_status();
  if (menu_status != QUIT_INPUT) {
    game_loop();
  }
  endwin();

  return 0;
}
