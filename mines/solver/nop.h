#ifndef MINES_SOLVER_NOP_H_
#define MINES_SOLVER_NOP_H_

#include <memory>

#include "mines/solver/solver.h"

namespace mines {
namespace solver {
namespace nop {

// Provides a solver that does nothing.
//
// This is useful for playing a game manually.
std::unique_ptr<Solver> New();

}  // namespace nop
}  // namespace solver
}  // namespace mines

#endif  // MINES_SOLVER_NOP_H_
