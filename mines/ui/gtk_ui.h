#ifndef MINES_UI_GTK_UI_H_
#define MINES_UI_GTK_UI_H_

#include <memory>

#include "mines/game/game.h"
#include "mines/solver/solver.h"

namespace mines {
namespace ui {

// A GUI based on GTK3.
class GtkUi {
 public:
  virtual ~GtkUi() = default;

  // Plays a game using the UI.
  //
  // Returns when the main window is closed.
  virtual void Play(Game& game, solver::Solver& solver) = 0;
};

// Creates a new GUI.
std::unique_ptr<GtkUi> NewGtkUi(int& argc, char**& argv);

}  // namespace ui
}  // namespace mines

#endif  // MINES_UI_GTK_UI_H_
