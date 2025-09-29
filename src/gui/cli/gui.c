#include <locale.h>
#include <string.h>

#include "../../brick_game/tetris/game_interface.h"
#include "frontend.h"

typedef enum {
  NOT_VALUABLE_INPUT = 0,
  START_INPUT,
  QUIT_INPUT,
  PAUSE_INPUT,
  RESTART_INPUT,
} InputSignals_t;

int parse_input() {
  int result = NOT_VALUABLE_INPUT;
  int ch = getch();
  if (ch != ERR) {
    if (ch == 'q' || ch == 'Q') {
      userInput(Terminate, false);
      result = QUIT_INPUT;
    } else {
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
        case KEY_UP:
          break;
        case ' ':
          userInput(Action, false);
          break;
        default:
          break;
      }
    }
  }
  return result;
}

int main_menu_status() {
  int status = NOT_VALUABLE_INPUT;
  int waiting = 1;
  while (waiting) {
    print_menu();
    int ch = getch();
    if (ch == '\n') {
      status = START_INPUT;
      waiting = 0;
    } else if (ch == 'q' || ch == 'Q') {
      status = QUIT_INPUT;
      waiting = 0;
    }
  }
  return status;
}

void game_loop() {
  userInput(Start, false);
  int running = 1;
  int exit_requested = 0;
  while (running) {
    refresh();
    GameInfo_t info = updateCurrentState();
    print_field(&info);
    if (info.pause == 2) {
      print_game_over_prompt();
      running = 0;
    } else {
      if (parse_input() == QUIT_INPUT) {
        exit_requested = 1;
        running = 0;
      }
    }
  }
  if (!exit_requested) {
    int waiting = 1;
    while (waiting) {
      int ch = getch();
      if (ch == 'r' || ch == 'R') {
        userInput(Terminate, false);
        waiting = 0;
        game_loop();
      } else if (ch == 'q' || ch == 'Q' || ch == 27) {
        userInput(Terminate, false);
        waiting = 0;
        exit_requested = 1;
      }
    }
  }
}

int main() {
  int exit_code = 0;

  WIN_INIT(50);
  init_colors();
  setlocale(LC_ALL, "");
  int menu_status = main_menu_status();
  if (menu_status != QUIT_INPUT) {
    game_loop();
  }
  endwin();

  return exit_code;
}
