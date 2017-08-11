bin_PROGRAMS = mines-solver

AM_CXXFLAGS = -std=c++11 -Wall -Werror -pedantic
CLEANFILES = $(BUILT_SOURCES)

.PHONY: format
format:
	find -name "*.h" -exec clang-format -style=google -i {} \+
	find -name "*.cpp" -exec clang-format -style=google -i {} \+


mines_solver_SOURCES = \
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
  mines/ui/gtk_ui.cpp \
  mines/ui/gtk_ui.h \
  mines/ui/resources.cpp \
  mines/ui/resources.h

mines_solver_CPPFLAGS = @GTKMM_CFLAGS@
mines_solver_LDADD = @GTKMM_LIBS@


RESOURCE_FILES = \
  mines/ui/menu.ui \
  mines/ui/mine.svg \
  mines/ui/flag.svg

BUILT_SOURCES = \
  mines/ui/resources.cpp \
  mines/ui/resources.h
EXTRA_DIST = mines/ui/ui.gresource.xml $(RESOURCE_FILES)

mines/ui/resources.cpp: mines/ui/ui.gresource.xml $(RESOURCE_FILES)
	@GLIB_COMPILE_RESOURCES@ --generate-source --manual-register --internal --target="$@" --sourcedir="$(top_srcdir)/mines/ui" $<

mines/ui/resources.h: mines/ui/ui.gresource.xml $(RESOURCE_FILES)
	@GLIB_COMPILE_RESOURCES@ --generate-header --manual-register --internal --target="$@" --sourcedir="$(top_srcdir)/mines/ui" $<
