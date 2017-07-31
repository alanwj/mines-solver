#ifndef MINES_SINGLE_CELL_SOLVER_H_
#define MINES_SINGLE_CELL_SOLVER_H_

#include <memory>

#include "mines.h"
#include "solver.h"

namespace mines {
namespace solver {
namespace single_cell {

// Provides a solver that produces actions from local analysis considering a
// single cell at a time.
//
// This solver is capable of:
//  - Flagging cells when the number of uncovered adjacent cells matches the
//    number of adjacent mines.
//  - Uncovering adjacent cells when the number of flagged adjacent cells
//    matched the number of adjacent mines.
//
// This solver is useful for automating "obvious" actions, but will not find
// solutions that require reasoning about two or more cells simultaneously.
std::unique_ptr<Solver> New(const Game& game);

}  // namespace single_cell
}  // namespace solver
}  // namespace mines

#endif  // MINES_SINGLE_CELL_SOLVER_H_
