#include "mines/solver/local.h"

#include <cstddef>
#include <vector>

#include "mines/compat/make_unique.h"
#include "mines/solver/constraint_analyzer.h"

namespace mines {
namespace solver {
namespace local {

namespace {

class LocalSolver : public Solver {
 public:
  LocalSolver(Game& game) : analyzer_(NewConstraintAnalyzer(game)) {}

  ~LocalSolver() final = default;

  std::vector<Action> Analyze() final {
    const ConstraintAnalysis analysis = analyzer_->Analyze();

    const auto& locations = analysis.GetLocations();

    // We track which cells we already have an action for. This is because
    // analysis may produce duplicate actions. This is especially problematic
    // for FLAG actions since it is a toggle rather than an absolute action.
    std::vector<bool> have_action(locations.size(), false);

    std::vector<Action> actions;
    actions.reserve(locations.size());

    for (const auto& region : analysis.GetRegions()) {
      for (const auto& constraint : region.GetConstraints()) {
        const std::size_t mines = constraint.GetMines();
        const auto& location_indexes = constraint.GetLocationIndexes();

        // If no mines are in a set of cells, then those cells may be uncovered.
        if (mines == 0 && !location_indexes.empty()) {
          for (std::size_t index : location_indexes) {
            if (!have_action[index]) {
              have_action[index] = true;
              const CellLocation& location = locations[index];
              actions.push_back(
                  Action{Action::Type::UNCOVER, location.row, location.col});
            }
          }
        }

        // If all the cells in a set are mines then those may be flagged.
        if (mines > 0 && mines == location_indexes.size()) {
          for (std::size_t index : location_indexes) {
            if (!have_action[index]) {
              have_action[index] = true;
              const CellLocation& location = locations[index];
              actions.push_back(
                  Action{Action::Type::FLAG, location.row, location.col});
            }
          }
        }
      }
    }
    return actions;
  }

 private:
  std::unique_ptr<ConstraintAnalyzer> analyzer_;
};

}  // namespace

std::unique_ptr<Solver> New(Game& game) {
  return MakeUnique<LocalSolver>(game);
}

}  // namespace local
}  // namespace solver
}  // namespace mines
