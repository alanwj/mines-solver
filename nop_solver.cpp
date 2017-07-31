#include "nop_solver.h"

#include <utility>
#include <vector>

#include "compat/make_unique.h"
#include "mines.h"

namespace mines {
namespace solver {
namespace nop {

namespace {

class NopSolver : public Solver {
 public:
  NopSolver() = default;
  ~NopSolver() final = default;

  void Update(const std::vector<Event>&) final {}

  std::vector<Action> Analyze() final { return std::vector<Action>(); }
};

}  // namespace

std::unique_ptr<Solver> New() { return std::make_unique<NopSolver>(); }

}  // namespace nop
}  // namespace solver
}  // namespace mines