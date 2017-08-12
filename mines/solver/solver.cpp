#include "mines/solver/solver.h"

#include "mines/solver/local.h"
#include "mines/solver/nop.h"

namespace mines {
namespace solver {

std::unique_ptr<Solver> New(Algorithm alg, Game& game) {
  std::unique_ptr<Solver> solver;
  switch (alg) {
    case Algorithm::NONE:
      solver = nop::New();
      break;
    case Algorithm::LOCAL:
      solver = local::New(game);
      break;
    default:
      return nullptr;
  }
  game.Subscribe(solver.get());
  return solver;
}

}  // namespace solver
}  // namespace mines
