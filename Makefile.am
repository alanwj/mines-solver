SUBDIRS = compat

bin_PROGRAMS = mines-tui mines-assisted-tui
noinst_LIBRARIES = libmines.a libtui.a libmanual_player.a libassisted_player.a

AM_CXXFLAGS = -Wall -Werror -pedantic

COMMON_LDADD = compat/libcompat.a

mines_tui_SOURCES = \
  mines_tui.cpp

mines_tui_LDADD = \
  $(COMMON_LDADD) \
  libmines.a \
  libmanual_player.a \
  libtui.a

mines_assisted_tui_SOURCES = \
  mines_assisted_tui.cpp

mines_assisted_tui_LDADD = \
  $(COMMON_LDADD) \
  libmines.a \
  libassisted_player.a \
  libtui.a

libmines_a_SOURCES = \
  mines.cpp \
  mines.h

libmanual_player_a_SOURCES = \
  manual_player.cpp \
  manual_player.h

libassisted_player_a_SOURCES = \
  assisted_player.cpp \
  assisted_player.h

libtui_a_SOURCES = \
  tui.cpp \
  tui.h

.PHONY: format
format:
	find -name "*.h" -exec clang-format -style=google -i {} \+
	find -name "*.cpp" -exec clang-format -style=google -i {} \+
