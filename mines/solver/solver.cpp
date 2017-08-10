#include "mines/solver/solver.h"

#include "mines/solver/local.h"
#include "mines/solver/nop.h"

namespace mines {
namespace solver {

std::unique_ptr<Solver> New(Algorithm alg, const Game& game) {
  switch (alg) {
    case Algorithm::NONE:
      return nop::New();
    case Algorithm::LOCAL:
      return local::New(game);
    default:
      return nullptr;
  }
}

}  // namespace solver
}  // namespace mines
