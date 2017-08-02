bin_PROGRAMS =
noinst_LIBRARIES =


AM_CXXFLAGS = -Wall -Werror -pedantic

.PHONY: format
format:
	find -name "*.h" -exec clang-format -style=google -i {} \+
	find -name "*.cpp" -exec clang-format -style=google -i {} \+


noinst_LIBRARIES += libmines.a

libmines_a_SOURCES = \
  mines/compat/make_unique.h \
  mines/game/game.cpp \
  mines/game/game.h \
  mines/game/grid.h \
  mines/solver/nop.cpp \
  mines/solver/nop.h \
  mines/solver/single_cell.cpp \
  mines/solver/single_cell.h \
  mines/solver/solver.h \
  mines/ui/text_ui.cpp \
  mines/ui/text_ui.h


bin_PROGRAMS += mines-tui

mines_tui_SOURCES = \
  mines/mines_tui.cpp

mines_tui_LDADD = libmines.a


bin_PROGRAMS += mines-assisted-tui

mines_assisted_tui_SOURCES = \
  mines/mines_assisted_tui.cpp

mines_assisted_tui_LDADD = libmines.a
