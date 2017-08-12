#ifndef MINES_SOLVER_SOLVER_H_
#define MINES_SOLVER_SOLVER_H_

#include <memory>
#include <vector>

#include "mines/game/game.h"

namespace mines {
namespace solver {

// Solving algorithms.
enum class Algorithm {
  // Do nothing.
  NONE,

  // Perform local analysis of cells and their immediate neighbors.
  LOCAL,
};

class Solver : public EventSubscriber {
 public:
  virtual ~Solver() = default;

  // Recommends actions based on the Solver's current knowledge of the game.
  //
  // The Solver is NOT required to produce a complete set of actions, nor is it
  // required to be idempotent.
  //
  // The Solver must return an empty vector to indicate that no progress can be
  // made.
  virtual std::vector<Action> Analyze() = 0;
};

// Creates a new solver for the specified algorithm.
//
// This solver will be automatically subscribed to the provided game.
std::unique_ptr<Solver> New(Algorithm alg, Game& game);

}  // namespace solver
}  // namespace mines

#endif  // MINES_SOLVER_SOLVER_H_
