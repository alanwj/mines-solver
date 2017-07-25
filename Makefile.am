SUBDIRS = compat

bin_PROGRAMS = mines-tui
noinst_LIBRARIES = libmines.a libtui.a libmanual_player.a

AM_CXXFLAGS = -Wall -Werror -pedantic

mines_tui_SOURCES = \
  mines_tui.cpp

mines_tui_LDADD = libmines.a libmanual_player.a libtui.a compat/libcompat.a

libmines_a_SOURCES = \
  mines.cpp \
  mines.h

libmanual_player_a_SOURCES = \
  manual_player.cpp \
  manual_player.h

libtui_a_SOURCES = \
  tui.cpp \
  tui.h

.PHONY: format
format:
	find -name "*.h" -exec clang-format -style=google -i {} \+
	find -name "*.cpp" -exec clang-format -style=google -i {} \+
