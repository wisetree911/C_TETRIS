#include <check.h>
#include <stdlib.h>

#include "brick_game/tetris/game_logic.h"

static GameInfo_t fresh_state(void) {
  userInput(Start, false);
  return updateCurrentState();
}

static int count_active_cells(const GameInfo_t* info) {
  int count = 0;
  for (int r = 0; r < FIELD_ROWS; ++r) {
    for (int c = 0; c < FIELD_COLS; ++c) {
      if (info->field && info->field[r][c] > 0) count++;
    }
  }
  return count;
}

static int min_active_col(const GameInfo_t* info) {
  int min_col = FIELD_COLS;
  for (int r = 0; r < FIELD_ROWS; ++r) {
    for (int c = 0; c < FIELD_COLS; ++c) {
      if (info->field && info->field[r][c] > 0 && c < min_col) {
        min_col = c;
      }
    }
  }
  return min_col;
}

static int max_active_col(const GameInfo_t* info) {
  int max_col = -1;
  for (int r = 0; r < FIELD_ROWS; ++r) {
    for (int c = 0; c < FIELD_COLS; ++c) {
      if (info->field && info->field[r][c] > 0 && c > max_col) {
        max_col = c;
      }
    }
  }
  return max_col;
}

static int count_row_active(const GameInfo_t* info, int row) {
  int count = 0;
  if (info->field) {
    for (int c = 0; c < FIELD_COLS; ++c) {
      if (info->field[row][c] > 0) count++;
    }
  }
  return count;
}

static int min_active_row(const GameInfo_t* info) {
  int min_row = FIELD_ROWS;
  if (info->field) {
    for (int r = 0; r < FIELD_ROWS; ++r) {
      for (int c = 0; c < FIELD_COLS; ++c) {
        if (r < min_row && info->field[r][c] > 0) {
          min_row = r;
        }
      }
    }
  }
  return min_row;
}

START_TEST(test_start_initial_state) {
  GameInfo_t info = fresh_state();
  ck_assert_int_eq(info.level, 1);
  ck_assert_int_eq(info.speed, 12);
  ck_assert_int_eq(info.score, 0);
  ck_assert_int_ge(info.high_score, info.score);
  ck_assert_int_eq(info.pause, 0);
  ck_assert_ptr_nonnull(info.field);
  ck_assert_int_eq(count_active_cells(&info), 4);
}
END_TEST

START_TEST(test_move_left_stops_at_border) {
  fresh_state();
  for (int i = 0; i < FIELD_COLS; ++i) {
    userInput(Left, false);
  }
  GameInfo_t info = updateCurrentState();
  ck_assert_int_ge(min_active_col(&info), 0);
}
END_TEST

START_TEST(test_move_right_stops_at_border) {
  fresh_state();
  for (int i = 0; i < FIELD_COLS; ++i) {
    userInput(Right, false);
  }
  GameInfo_t info = updateCurrentState();
  ck_assert_int_lt(max_active_col(&info), FIELD_COLS);
}
END_TEST

START_TEST(test_hard_drop_places_piece_on_floor) {
  fresh_state();
  userInput(Down, false);
  GameInfo_t info = updateCurrentState();
  ck_assert_int_eq(count_row_active(&info, FIELD_ROWS - 1), 4);
}
END_TEST

START_TEST(test_tick_advances_piece) {
  GameInfo_t info = fresh_state();
  int start_row = min_active_row(&info);
  int ticks = info.speed;
  for (int i = 0; i <= ticks; ++i) {
    info = updateCurrentState();
  }
  int after_row = min_active_row(&info);
  ck_assert_int_gt(after_row, start_row);
}
END_TEST

START_TEST(test_rotate_action_changes_width) {
  GameInfo_t info = fresh_state();
  int width_before = max_active_col(&info) - min_active_col(&info);
  userInput(Action, false);
  info = updateCurrentState();
  int width_after = max_active_col(&info) - min_active_col(&info);
  ck_assert_int_lt(width_after, width_before);
}
END_TEST

START_TEST(test_up_action_has_no_effect) {
  GameInfo_t info = fresh_state();
  int start_row = min_active_row(&info);
  int start_col = min_active_col(&info);
  userInput(Up, false);
  info = updateCurrentState();
  ck_assert_int_eq(min_active_row(&info), start_row);
  ck_assert_int_eq(min_active_col(&info), start_col);
}
END_TEST

START_TEST(test_pause_toggle) {
  fresh_state();
  userInput(Pause, false);
  GameInfo_t info = updateCurrentState();
  ck_assert_int_eq(info.pause, 1);
  userInput(Pause, false);
  info = updateCurrentState();
  ck_assert_int_eq(info.pause, 0);
}
END_TEST

START_TEST(test_terminate_sets_game_over) {
  fresh_state();
  userInput(Terminate, false);
  GameInfo_t info = updateCurrentState();
  ck_assert_int_eq(info.pause, 2);
}
END_TEST

static Suite* create_tetris_suite(void) {
  Suite* s = suite_create("brick_game_tetris");
  TCase* tc_core = tcase_create("core");

  tcase_add_test(tc_core, test_start_initial_state);
  tcase_add_test(tc_core, test_move_left_stops_at_border);
  tcase_add_test(tc_core, test_move_right_stops_at_border);
  tcase_add_test(tc_core, test_hard_drop_places_piece_on_floor);
  tcase_add_test(tc_core, test_tick_advances_piece);
  tcase_add_test(tc_core, test_rotate_action_changes_width);
  tcase_add_test(tc_core, test_up_action_has_no_effect);
  tcase_add_test(tc_core, test_pause_toggle);
  tcase_add_test(tc_core, test_terminate_sets_game_over);

  suite_add_tcase(s, tc_core);
  return s;
}

int main(void) {
  Suite* suite = create_tetris_suite();
  SRunner* runner = srunner_create(suite);
  srunner_run_all(runner, CK_ENV);
  int failed = srunner_ntests_failed(runner);
  srunner_free(runner);
  return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
