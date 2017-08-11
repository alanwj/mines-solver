#include "mines/solver/nop.h"

#include <vector>

#include "mines/compat/make_unique.h"
#include "mines/game/game.h"

namespace mines {
namespace solver {
namespace nop {

namespace {

class NopSolver : public Solver {
 public:
  NopSolver() = default;
  ~NopSolver() final = default;

  // Does nothing.
  void NotifyEvent(const Event&) final {}

  std::vector<Action> Analyze() final { return std::vector<Action>(); }
};

}  // namespace

std::unique_ptr<Solver> New() { return MakeUnique<NopSolver>(); }

}  // namespace nop
}  // namespace solver
}  // namespace mines
