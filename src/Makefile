SRC_DIR      = .
BRICK_GAME_DIR = $(SRC_DIR)/brick_game
TETRIS_DIR   = $(BRICK_GAME_DIR)/tetris
GUI_DIR      = $(SRC_DIR)/gui/cli
TEST_DIR     = $(SRC_DIR)/tests
BUILD_DIR    = $(SRC_DIR)/../build
LIB_DIR      = $(BUILD_DIR)/lib
OBJ_DIR      = $(BUILD_DIR)/obj
DIST_DIR     = dist
INSTALL_DIR  = $(HOME)/TetrisGame
DIST_NAME    = tetris_project

TETRIS_SRC   = $(TETRIS_DIR)/game_logic.c
GUI_SRC      = $(GUI_DIR)/gui.c $(TETRIS_DIR)/frontend.c
TEST_SRC     = $(TEST_DIR)/test.c

TETRIS_OBJ   = $(OBJ_DIR)/brick_game/tetris/game_logic.o
GUI_OBJ      = $(OBJ_DIR)/gui/cli/gui.o $(OBJ_DIR)/gui/cli/frontend.o
TEST_OBJ     = $(TEST_DIR)/test.o

LIB_NAME     = libbrick_game_tetris.a
LIB_TARGET   = $(LIB_DIR)/$(LIB_NAME)
EXEC         = tetris
DOC          = README.md
FSM_DOT      = docs/fsm.dot
FSM_PNG      = docs/fsm.png
HIGH_SCORE   = high_score.dat
DIST_FILES   = brick_game gui tests Makefile $(DOC) docs high_score.dat

OS_NAME := $(shell uname -s)

PKG_CHECK_CFLAGS := $(shell pkg-config --cflags check 2>/dev/null)
PKG_CHECK_LIBS   := $(shell pkg-config --libs check 2>/dev/null)

CHECK_CFLAGS := $(PKG_CHECK_CFLAGS)
CHECK_LIBS   := $(PKG_CHECK_LIBS)
CURSES_LIB   = -lncurses

ifeq ($(OS_NAME),Linux)
  OPEN        = xdg-open
  ifeq ($(strip $(CHECK_LIBS)),)
    CHECK_LIBS = -lcheck -lsubunit -lrt -lpthread -lm
  endif
endif

ifeq ($(OS_NAME),Darwin)
  OPEN        = open
  CHECK_CFLAGS += -I/opt/homebrew/opt/check/include
  ifeq ($(strip $(CHECK_LIBS)),)
    CHECK_LIBS = -lcheck -lpthread -lm
  endif
  LD_EXTRA    = -L/opt/homebrew/opt/check/lib
endif

ifeq ($(strip $(CHECK_LIBS)),)
  CHECK_LIBS = -lcheck -lpthread -lm
endif

CC           = gcc
BASE_CFLAGS  = -Wall -Wextra -Werror -std=c11
INCLUDE_DIRS = -I$(SRC_DIR)
CFLAGS       = $(BASE_CFLAGS) $(INCLUDE_DIRS)
APP_LIBS     = -L$(LIB_DIR) -lbrick_game_tetris $(CURSES_LIB) $(LD_EXTRA)
TEST_LIBS    = -L$(LIB_DIR) -lbrick_game_tetris $(CHECK_LIBS) $(LD_EXTRA)
GCOV_FLAGS   = -fprofile-arcs -ftest-coverage

DIRS := $(OBJ_DIR)/brick_game/tetris $(OBJ_DIR)/gui/cli $(OBJ_DIR)/tests $(LIB_DIR) $(DIST_DIR)
$(shell mkdir -p $(DIRS))

.PHONY: all clean install uninstall dvi dist test gcov_report rebuild

all: $(EXEC)

$(EXEC): $(LIB_TARGET) $(GUI_OBJ)
	$(CC) $(CFLAGS) $(GUI_OBJ) $(APP_LIBS) -o $@

$(LIB_TARGET): $(TETRIS_OBJ)
	@mkdir -p $(LIB_DIR)
	ar rcs $@ $^

$(OBJ_DIR)/brick_game/tetris/%.o: $(TETRIS_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/gui/cli/%.o: $(GUI_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(TEST_DIR)/%.o: $(TEST_DIR)/%.c
	$(CC) $(CFLAGS) $(CHECK_CFLAGS) -c $< -o $@

$(FSM_PNG): $(FSM_DOT)
	@mkdir -p $(dir $@)
	dot -Tpng $(FSM_DOT) -o $@

install: $(EXEC) $(FSM_PNG)
	mkdir -p $(INSTALL_DIR)
	cp $(EXEC) $(INSTALL_DIR)/
	cp high_score.dat $(INSTALL_DIR)/
	cp $(FSM_PNG) $(INSTALL_DIR)/ || true
	@echo "Installation finished. Run $(INSTALL_DIR)/$(EXEC) to play."

uninstall:
	rm -rf $(INSTALL_DIR)
	@echo "Done."

dvi: $(DOC)
	@if [ -n "$(OPEN)" ] && command -v $(OPEN) >/dev/null 2>&1; then \
		$(OPEN) $(DOC) || cat $(DOC); \
	else \
		cat $(DOC); \
	fi

dist: clean $(FSM_PNG)
	mkdir -p $(DIST_DIR)/$(DIST_NAME)
	cp -r $(DIST_FILES) $(DIST_DIR)/$(DIST_NAME)/
	tar -czf $(DIST_DIR)/$(DIST_NAME).tar.gz -C $(DIST_DIR) $(DIST_NAME)
	rm -rf $(DIST_DIR)/$(DIST_NAME)
	@echo "Archive created at $(DIST_DIR)/$(DIST_NAME).tar.gz"

test: $(LIB_TARGET) $(TEST_OBJ)
	$(CC) $(CFLAGS) $(CHECK_CFLAGS) $(TEST_OBJ) $(TEST_LIBS) -o $(TEST_DIR)/tests_run
	CK_FORK=no $(TEST_DIR)/tests_run


GCOV_INFO       = gcov_report/coverage.info
GCOV_INFO_FLTR  = gcov_report/coverage_filtered.info

gcov_report: clean
	$(MAKE) CFLAGS="$(CFLAGS) $(GCOV_FLAGS)" test
	@mkdir -p gcov_report
	@lcov --capture --directory ../build --base-directory . --output-file $(GCOV_INFO) --rc branch_coverage=0
	@lcov --remove $(GCOV_INFO) '*tests/test.c' --output-file $(GCOV_INFO_FLTR) --rc branch_coverage=0 --ignore-errors unused
	@genhtml $(GCOV_INFO_FLTR) --output-directory gcov_report >/dev/null
	@echo "Coverage report generated: gcov_report/index.html"


clean:
	rm -rf $(BUILD_DIR) $(EXEC) $(DIST_DIR)
	rm -f $(TEST_DIR)/test.o $(TEST_DIR)/tests_run
	@find . -name '*.gcda' -delete 2>/dev/null || true
	@find . -name '*.gcno' -delete 2>/dev/null || true
	rm -f *.info
	rm -rf gcov_report

rebuild: clean all
