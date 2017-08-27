#ifndef MINES_SOLVER_LOCAL_H_
#define MINES_SOLVER_LOCAL_H_

#include <memory>

#include "mines/game/game.h"
#include "mines/solver/solver.h"

namespace mines {
namespace solver {
namespace local {

// Provides a solver that produces actions from local analysis of a cell and
// its immediate neighbors.
//
// This solver is capable of:
//  - Flagging cells when the number of uncovered adjacent cells matches the
//    number of adjacent mines.
//  - Uncovering adjacent cells when the number of flagged adjacent cells
//    matched the number of adjacent mines.
//
// This solver is useful for automating "obvious" actions, but will not find
// solutions that require reasoning about two or more cells simultaneously.
std::unique_ptr<Solver> New(Game& game);

}  // namespace local
}  // namespace solver
}  // namespace mines

#endif  // MINES_SOLVER_LOCAL_H_
