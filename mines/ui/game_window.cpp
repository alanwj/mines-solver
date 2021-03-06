#include "mines/ui/game_window.h"

#include <ctime>
#include <vector>

#include <sigc++/functors/mem_fun.h>

namespace mines {
namespace ui {

const GameWindow::Difficulty GameWindow::kBeginnerDifficulty = {8, 8, 10};
const GameWindow::Difficulty GameWindow::kIntermediateDifficulty = {16, 16, 40};
const GameWindow::Difficulty GameWindow::kExpertDifficulty = {16, 30, 99};

GameWindow* GameWindow::Get(const Glib::RefPtr<Gtk::Builder>& builder,
                            const Difficulty& difficulty) {
  GameWindow* window = nullptr;
  builder->get_widget_derived("game-window", window);

  // When support for Ubuntu 14.04 is dropped we can pass the difficulty as a
  // constructor parameter via get_widget_derived.
  window->difficulty_ = difficulty;
  window->NewGame();

  return window;
}

GameWindow::GameWindow(BaseObjectType* cobj,
                       const Glib::RefPtr<Gtk::Builder>& builder)
    : Gtk::ApplicationWindow(cobj),
      mine_field_(MineField::Get(builder)),
      reset_button_(ResetButton::Get(builder, *mine_field_)),
      remaining_mines_counter_(RemainingMinesCounter::Get(builder)),
      elapsed_time_counter_(ElapsedTimeCounter::Get(builder)),
      solver_algorithm_(solver::Algorithm::NONE) {
  add_action("new", sigc::mem_fun(this, &GameWindow::NewGame));
  solver_action_ = add_action_radio_string(
      "solver", sigc::mem_fun(this, &GameWindow::NewSolverAlgorithm), "none");

  mine_field_->signal_action().connect(
      sigc::mem_fun(this, &GameWindow::HandleAction));

  reset_button_->signal_clicked().connect(
      sigc::mem_fun(this, &GameWindow::NewGame));
}

void GameWindow::NewGame() {
  game_ = mines::NewGame(difficulty_.rows, difficulty_.cols, difficulty_.mines,
                         std::time(nullptr));
  solver_ = solver::New(solver_algorithm_, *game_);

  // Subscribe UI widgets to the new game, causing them to reset.
  game_->Subscribe(mine_field_);
  game_->Subscribe(reset_button_);
  game_->Subscribe(remaining_mines_counter_);
  game_->Subscribe(elapsed_time_counter_);
}

void GameWindow::NewSolverAlgorithm(const Glib::ustring& target) {
  solver_action_->change_state(target);

  if (target == "none") {
    solver_algorithm_ = solver::Algorithm::NONE;
  } else if (target == "local") {
    solver_algorithm_ = solver::Algorithm::LOCAL;
  } else {
    solver_algorithm_ = solver::Algorithm::NONE;
  }

  NewGame();
}

void GameWindow::HandleAction(Action action) {
  game_->Execute(action);

  // Execute all actions recommended by the solver.
  std::vector<Action> actions;
  do {
    actions = solver_->Analyze();
    game_->Execute(actions);
  } while (!actions.empty());
}

}  // namespace ui
}  // namespace mines
