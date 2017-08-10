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
  mines/solver/local.cpp \
  mines/solver/local.h \
  mines/solver/nop.cpp \
  mines/solver/nop.h \
  mines/solver/solver.h \
  mines/ui/text_ui.cpp \
  mines/ui/text_ui.h


noinst_LIBRARIES += libgtk_ui.a

libgtk_ui_a_SOURCES = \
  mines/ui/gtk_ui.cpp \
  mines/ui/gtk_ui.h

libgtk_ui_a_CPPFLAGS = @GTKMM_CFLAGS@


bin_PROGRAMS += mines-gui

mines_gui_SOURCES = \
  mines/mines_gui.cpp

mines_gui_CPPFLAGS = @GTKMM_CFLAGS@
mines_gui_LDADD = \
  libmines.a \
  libgtk_ui.a \
  @GTKMM_LIBS@


bin_PROGRAMS += mines-assisted-gui

mines_assisted_gui_SOURCES = \
  mines/mines_assisted_gui.cpp

mines_assisted_gui_CPPFLAGS = @GTKMM_CFLAGS@
mines_assisted_gui_LDADD = \
  libmines.a \
  libgtk_ui.a \
  @GTKMM_LIBS@


bin_PROGRAMS += mines-tui

mines_tui_SOURCES = \
  mines/mines_tui.cpp

mines_tui_LDADD = libmines.a


bin_PROGRAMS += mines-assisted-tui

mines_assisted_tui_SOURCES = \
  mines/mines_assisted_tui.cpp

mines_assisted_tui_LDADD = libmines.a
