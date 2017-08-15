#include "mines/ui/game_window.h"

#include <ctime>
#include <vector>

#include <sigc++/functors/mem_fun.h>

#include "mines/ui/builder_widget.h"

namespace mines {
namespace ui {

GameWindow::GameWindow(BaseObjectType* cobj,
                       const Glib::RefPtr<Gtk::Builder>& builder)
    : Gtk::ApplicationWindow(cobj),
      field_(GetBuilderWidget<MineField>(builder, "mine-field")),
      reset_button_(GetBuilderWidget<ResetButton>(builder, "reset-button")),
      remaining_mines_counter_(GetBuilderWidget<RemainingMinesCounter>(
          builder, "remaining-mines-counter")),
      elapsed_time_counter_(GetBuilderWidget<ElapsedTimeCounter>(
          builder, "elapsed-time-counter")),
      solver_algorithm_(solver::Algorithm::NONE) {
  add_action("new", sigc::mem_fun(this, &GameWindow::NewGame));
  solver_action_ = add_action_radio_string(
      "solver", sigc::mem_fun(this, &GameWindow::NewSolverAlgorithm), "none");

  field_->signal_action().connect(
      sigc::mem_fun(this, &GameWindow::HandleAction));

  reset_button_->signal_clicked().connect(
      sigc::mem_fun(this, &GameWindow::NewGame));
  reset_button_->ConnectToMineField(*field_);

  NewGame();
}

void GameWindow::NewGame() {
  game_ = mines::NewGame(16, 30, 99, std::time(nullptr));
  solver_ = solver::New(solver_algorithm_, *game_);
  field_->Reset(*game_);
  reset_button_->Reset(*game_);
  remaining_mines_counter_->Reset(*game_);
  elapsed_time_counter_->Reset(*game_);
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
