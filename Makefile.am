SUBDIRS = compat

bin_PROGRAMS = mines-tui mines-assisted-tui
noinst_LIBRARIES = libmines.a

AM_CXXFLAGS = -Wall -Werror -pedantic

COMMON_LDADD = compat/libcompat.a

mines_tui_SOURCES = \
  mines_tui.cpp

mines_tui_LDADD = \
  $(COMMON_LDADD) \
  libmines.a

mines_assisted_tui_SOURCES = \
  mines_assisted_tui.cpp

mines_assisted_tui_LDADD = \
  $(COMMON_LDADD) \
  libmines.a

libmines_a_SOURCES = \
  grid.h \
  mines.cpp \
  mines.h \
  nop_solver.cpp \
  nop_solver.h \
  single_cell_solver.cpp \
  single_cell_solver.h \
  solver.h \
  text_ui.cpp \
  text_ui.h

.PHONY: format
format:
	find -name "*.h" -exec clang-format -style=google -i {} \+
	find -name "*.cpp" -exec clang-format -style=google -i {} \+
