#ifndef MINES_UI_MINES_WINDOW_H_
#define MINES_UI_MINES_WINDOW_H_

#include <cstddef>
#include <memory>

#include <giomm/simpleaction.h>
#include <glibmm/refptr.h>
#include <glibmm/ustring.h>
#include <gtkmm/applicationwindow.h>
#include <gtkmm/builder.h>

#include "mines/game/game.h"
#include "mines/solver/solver.h"
#include "mines/ui/elapsed_time_counter.h"
#include "mines/ui/mine_field.h"
#include "mines/ui/remaining_mines_counter.h"
#include "mines/ui/reset_button.h"

namespace mines {
namespace ui {

// The main window for the game.
class GameWindow : public Gtk::ApplicationWindow {
 public:
  // The difficulty setting for the game.
  struct Difficulty {
    std::size_t rows;
    std::size_t cols;
    std::size_t mines;
  };

  // Premade common difficulties.
  static const Difficulty kBeginnerDifficulty;
  static const Difficulty kIntermediateDifficulty;
  static const Difficulty kExpertDifficulty;

  // Gets the GameWindow from the builder.
  static GameWindow* Get(const Glib::RefPtr<Gtk::Builder>& builder,
                         const Difficulty& difficulty);

  // Contructs a GameWindow from the underlying C object and a builder.
  //
  // This constructor supports Gtk::Builder::get_widget_derived.
  GameWindow(BaseObjectType* cobj, const Glib::RefPtr<Gtk::Builder>& builder);

 private:
  // Starts a new game.
  void NewGame();

  // Changes the solver algorithm and starts a new game.
  void NewSolverAlgorithm(const Glib::ustring& target);

  // Handles an action on the mine field.
  //
  // Executes the action, updates the mine field, and runs the solver.
  void HandleAction(Action action);

  // The mine field.
  MineField* mine_field_;

  // The reset button.
  ResetButton* reset_button_;

  // The remaining mines counter.
  RemainingMinesCounter* remaining_mines_counter_;

  // The elapsed time counter.
  ElapsedTimeCounter* elapsed_time_counter_;

  // Menu action for selection the solver algorithm.
  Glib::RefPtr<Gio::SimpleAction> solver_action_;

  // The algorithm used in current and new games.
  solver::Algorithm solver_algorithm_;

  // Parameters for creating new games.
  Difficulty difficulty_ = kExpertDifficulty;

  // The current game.
  std::unique_ptr<Game> game_;

  // The current solver.
  std::unique_ptr<solver::Solver> solver_;
};

}  // namespace ui
}  // namespace mines

#endif  // MINES_UI_MINES_WINDOW_H_
