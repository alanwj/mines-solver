#ifndef MINES_SOLVER_CONSTRAINT_ANALYZER_H_
#define MINES_SOLVER_CONSTRAINT_ANALYZER_H_

#include <cstddef>
#include <utility>
#include <vector>

#include "mines/game/game.h"

namespace mines {
namespace solver {

// Represents the row/col location of a cell.
struct CellLocation {
  std::size_t row;
  std::size_t col;
};

inline bool operator==(const CellLocation& a, const CellLocation& b) {
  return a.row == b.row && a.col == b.col;
}

inline bool operator!=(const CellLocation& a, const CellLocation& b) {
  return a.row != b.row || a.col != b.col;
}

// Represents a known constraint on a game. Each constraint represents a set of
// cells and the number of mines that must be in those cells.
class Constraint {
 public:
  Constraint(std::vector<std::size_t> location_indexes, std::size_t mines)
      : location_indexes_(std::move(location_indexes)), mines_(mines) {}

  // A constraint is considered stable if it can have no more effect on the
  // game.
  //
  // This occurs when all adjacent cells are uncovered except those that are
  // flagged. The only action that can change a stable constraint is unflagging
  // a previously flagged cell.
  bool IsStable() const { return mines_ == 0 && location_indexes_.empty(); }

  // Returns the locations affected by the constraint.
  //
  // These are indexes into the constraint analysis location vector.
  const std::vector<std::size_t>& GetLocationIndexes() const {
    return location_indexes_;
  }

  // Returns the number of mines in the locations affected by this cell.
  std::size_t GetMines() const { return mines_; }

 private:
  std::vector<std::size_t> location_indexes_;
  std::size_t mines_;
};

// Represents a set of constraints that are disjoint in effect. Constraints in
// one region may be logically analyzed separately from other regions.
class Region {
 public:
  explicit Region(std::vector<Constraint> constraints)
      : constraints_(std::move(constraints)) {}

  // Returns the constraints in this region.
  const std::vector<Constraint>& GetConstraints() const { return constraints_; }

 public:
  std::vector<Constraint> constraints_;
};

class ConstraintAnalysis {
 public:
  ConstraintAnalysis() = default;
  ConstraintAnalysis(std::vector<CellLocation> locations,
                     std::vector<Region> regions)
      : locations_(std::move(locations)), regions_(std::move(regions)) {}

  const std::vector<CellLocation>& GetLocations() const { return locations_; }
  const std::vector<Region>& GetRegions() const { return regions_; }

 private:
  std::vector<CellLocation> locations_;
  std::vector<Region> regions_;
};

class ConstraintAnalyzer : public EventSubscriber {
 public:
  virtual ~ConstraintAnalyzer() = default;

  virtual ConstraintAnalysis Analyze() = 0;
};

// Returns a new ConstraintAnalyzer.
std::unique_ptr<ConstraintAnalyzer> NewConstraintAnalyzer(Game& game);

}  // namespace solver
}  // namespace mines

#endif  // MINES_SOLVER_CONSTRAINT_ANALYZER_H_
