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

inline bool operator<(const CellLocation& a, const CellLocation& b) {
  return a.row < b.row || (a.row == b.row && a.col < b.col);
}

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
  Constraint(std::vector<CellLocation> locs, std::size_t mines)
      : locs_(std::move(locs)), mines_(mines) {}

  ~Constraint() = default;

  Constraint(const Constraint&) = delete;
  Constraint& operator=(const Constraint&) = delete;

  Constraint(Constraint&&) = default;
  Constraint& operator=(Constraint&&) = default;

  // A constraint is considered stable if it can have no more effect on the
  // game.
  //
  // This occurs when all adjacent cells are uncovered except those that are
  // flagged. The only action that can change a stable constraint is unflagging
  // a previously flagged cell.
  bool IsStable() const { return mines_ == 0 && locs_.empty(); }

  // Returns the locations affected by the constraint.
  const std::vector<CellLocation>& GetLocations() const { return locs_; }

  // Returns the number of mines in the locations affected by this cell.
  std::size_t GetMines() const { return mines_; }

 private:
  std::vector<CellLocation> locs_;
  std::size_t mines_;
};

// Represesnts a set of constraints that are disjoint in effect. Constraints in
// one region may be logically analyzed separately from other regions.
//
// Note: Regions only contain pointers to constraints, which much continue to
// exist for the lifetime of the region.
class Region {
 public:
  explicit Region(std::vector<const Constraint*> constraints)
      : constraints_(std::move(constraints)) {}

  ~Region() = default;

  Region(const Region&) = delete;
  Region& operator=(const Region&) = delete;

  Region(Region&&) = default;
  Region& operator=(Region&&) = default;

  std::vector<const Constraint*> GetConstraints() const { return constraints_; }

 public:
  std::vector<const Constraint*> constraints_;
};

class ConstraintAnalyzer : public EventSubscriber {
 public:
  virtual std::vector<Constraint> AnalyzeConstraints() = 0;

  virtual std::vector<Region> AnalyzeRegions(
      std::vector<Constraint>& constraints) = 0;
};

std::unique_ptr<ConstraintAnalyzer> NewConstrantAnalyzer(Game& game);

}  // namespace solver
}  // namespace mines

#endif  // MINES_SOLVER_CONSTRAINT_ANALYZER_H_
