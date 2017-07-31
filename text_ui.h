#ifndef MINES_TEXT_UI_H_
#define MINES_TEXT_UI_H_

#include <iosfwd>
#include <memory>

#include "mines.h"
#include "solver.h"

namespace mines {
namespace ui {

// A text user interface based on iostreams.
class TextUi {
 public:
  virtual ~TextUi() = default;

  // Plays the game.
  //
  // After execution of each action, the solver will be queried, and any actions
  // it recommends will be executed. If the solver does not recommend any
  // actions, the user is prompted for an action.
  virtual void Play(Game& game, solver::Solver& solver) = 0;
};

// Returns a new text user interface for the given iostreams.
std::unique_ptr<TextUi> NewTextUi(std::istream& in, std::ostream& out);

}  // namespace ui
}  // namespace mines

#endif  // MINES_TEXT_UI_H_
