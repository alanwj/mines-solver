#ifndef MINES_SOLVER_H_
#define MINES_SOLVER_H_

#include <vector>

#include "mines.h"

namespace mines {
namespace solver {

class Solver {
 public:
  virtual ~Solver() = default;

  // Updates the Solver's knowledge of the current game.
  //
  // Any time actions are executed, the result should be provided to the Solver
  // via this method.
  virtual void Update(const std::vector<Event>& events) = 0;

  // Recommends actions based on the Solver's current knowledge of the game.
  //
  // The Solver is NOT required to produce a complete set of actions, nor is it
  // required to be idempotent.
  //
  // The Solver must return an empty vector to indicate that no progress can be
  // made.
  virtual std::vector<Action> Analyze() = 0;
};

}  // namespace solver
}  // namespace mines

#endif  // MINES_SOLVER_H_
