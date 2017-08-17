bin_PROGRAMS = mines-solver

AM_CXXFLAGS = -std=c++11 -Wall -Werror -pedantic
CLEANFILES = $(BUILT_SOURCES)

.PHONY: format
format:
	find -name "*.h" -exec clang-format -style=google -i {} \+
	find -name "*.cpp" -exec clang-format -style=google -i {} \+


mines_solver_SOURCES = \
  mines/compat/gdk_pixbuf.cpp \
  mines/compat/gdk_pixbuf.h \
  mines/compat/make_unique.h \
  mines/game/game.cpp \
  mines/game/game.h \
  mines/game/grid.h \
  mines/mines_solver_main.cpp \
  mines/solver/local.cpp \
  mines/solver/local.h \
  mines/solver/nop.cpp \
  mines/solver/nop.h \
  mines/solver/solver.cpp \
  mines/solver/solver.h \
  mines/ui/builder_widget.h \
  mines/ui/counter.cpp \
  mines/ui/counter.h \
  mines/ui/elapsed_time_counter.cpp \
  mines/ui/elapsed_time_counter.h \
  mines/ui/game_window.cpp \
  mines/ui/game_window.h \
  mines/ui/mine_field.cpp \
  mines/ui/mine_field.h \
  mines/ui/remaining_mines_counter.cpp \
  mines/ui/remaining_mines_counter.h \
  mines/ui/reset_button.cpp \
  mines/ui/reset_button.h \
  mines/ui/resources.cpp \
  mines/ui/resources.h \
  mines/ui/ui.cpp \
  mines/ui/ui.h

mines_solver_CPPFLAGS = @GTKMM_CFLAGS@
mines_solver_LDADD = @GTKMM_LIBS@


RESOURCE_FILES = \
  mines/ui/resources/mines-solver.ui \
  mines/ui/resources/flag.svg \
  mines/ui/resources/mine.svg \
  mines/ui/resources/smiley-cool.svg \
  mines/ui/resources/smiley-cry.svg \
  mines/ui/resources/smiley-happy.svg \
  mines/ui/resources/smiley-scared.svg \
  mines/ui/resources/digit-0.bmp \
  mines/ui/resources/digit-1.bmp \
  mines/ui/resources/digit-2.bmp \
  mines/ui/resources/digit-3.bmp \
  mines/ui/resources/digit-4.bmp \
  mines/ui/resources/digit-5.bmp \
  mines/ui/resources/digit-6.bmp \
  mines/ui/resources/digit-7.bmp \
  mines/ui/resources/digit-8.bmp \
  mines/ui/resources/digit-9.bmp \
  mines/ui/resources/digit-off.bmp

BUILT_SOURCES = \
  mines/ui/resources.cpp \
  mines/ui/resources.h
EXTRA_DIST = mines/ui/ui.gresource.xml $(RESOURCE_FILES)

mines/ui/resources.cpp: mines/ui/ui.gresource.xml $(RESOURCE_FILES)
	@GLIB_COMPILE_RESOURCES@ --generate-source --manual-register --internal --target="$@" --sourcedir="$(top_srcdir)/mines/ui/resources" $<

mines/ui/resources.h: mines/ui/ui.gresource.xml $(RESOURCE_FILES)
	@GLIB_COMPILE_RESOURCES@ --generate-header --manual-register --internal --target="$@" --sourcedir="$(top_srcdir)/mines/ui/resources" $<
