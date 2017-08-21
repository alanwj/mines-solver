#include "mines/solver/constraint_analyzer.h"

#include <map>
#include <set>

#include "mines/game/grid.h"

namespace mines {
namespace solver {

namespace {

class ConstraintAnalyzerImpl : public ConstraintAnalyzer {
 public:
  ConstraintAnalyzerImpl(std::size_t rows, std::size_t cols)
      : grid_(rows, cols) {}

  std::vector<Constraint> AnalyzeConstraints() final {
    std::vector<Constraint> constraints;
    for (auto it = cell_loc_.begin(); it != cell_loc_.end();) {
      auto cur = it++;
      Constraint constraint{MakeConstraint(*cur)};
      if (constraint.IsStable()) {
        // The constraint has become stable, so we may remove it from analysis.
        cell_loc_.erase(cur);
      } else {
        constraints.push_back(std::move(constraint));
      }
    }
    return constraints;
  }

  std::vector<Region> AnalyzeRegions(
      std::vector<Constraint>& constraints) final {
    for (const Constraint& constraint : constraints) {
      const std::vector<CellLocation>& locations = constraint.GetLocations();
      if (locations.empty()) {
        continue;
      }
      auto first = locations.begin();
      for (auto it = first + 1; it != locations.end(); ++it) {
        Union(*first, *it);
      }
    }

    std::map<CellLocation, std::vector<const Constraint*>> constraint_map;
    for (Constraint& constraint : constraints) {
      const std::vector<CellLocation>& locations = constraint.GetLocations();
      if (locations.empty()) {
        continue;
      }
      CellLocation loc = Find(locations[0]);
      constraint_map[loc].push_back(&constraint);
    }

    std::vector<Region> regions;
    for (auto& p : constraint_map) {
      regions.push_back(Region(std::move(p.second)));
    }

    return regions;
  }

  void NotifyEvent(const Event& event) final {
    if (!grid_.IsValid(event.row, event.col)) {
      return;
    }
    Cell& cell = grid_(event.row, event.col);
    cell.adjacent_mines = event.adjacent_mines;
    switch (event.type) {
      case Event::Type::UNCOVER:
        cell.state = CellState::UNCOVERED;
        if (event.adjacent_mines != 0) {
          cell_loc_.insert({event.row, event.col});
        }
        break;
      case Event::Type::FLAG:
        cell.state = CellState::FLAGGED;
        break;
      case Event::Type::UNFLAG:
        cell.state = CellState::COVERED;
        grid_.ForEachAdjacent(event.row, event.col, [this](std::size_t row,
                                                           std::size_t col) {
          const Cell& cell = grid_(row, col);
          if (cell.state == CellState::UNCOVERED && cell.adjacent_mines != 0) {
            cell_loc_.insert({row, col});
          }
          return false;
        });
        break;
      case Event::Type::WIN:
        // No new knowledge.
        break;
      case Event::Type::LOSS:
        cell.state = CellState::LOSING_MINE;
        break;
      case Event::Type::IDENTIFY_MINE:
        cell.state = CellState::MINE;
        break;
      case Event::Type::IDENTIFY_BAD_FLAG:
        cell.state = CellState::BAD_FLAG;
        break;
    }
  }

 private:
  struct Cell {
    CellState state = CellState::COVERED;
    std::size_t adjacent_mines = 0;

    CellLocation ds_parent{0, 0};
    std::size_t ds_rank = 0;
  };

  CellLocation Find(const CellLocation& loc) {
    Cell& cell = grid_(loc.row, loc.col);
    if (cell.ds_parent != loc) {
      cell.ds_parent = Find(cell.ds_parent);
    }
    return cell.ds_parent;
  }

  void Union(const CellLocation& x, const CellLocation& y) {
    CellLocation x_root = Find(x);
    CellLocation y_root = Find(y);

    // Already part of the same set.
    if (x_root == y_root) {
      return;
    }

    Cell& x_root_cell = grid_(x_root.row, x_root.col);
    Cell& y_root_cell = grid_(y_root.row, y_root.col);

    // Attach the shorter tree to the longer one.
    if (x_root_cell.ds_rank < y_root_cell.ds_rank) {
      x_root_cell.ds_parent = y_root;
    } else if (x_root_cell.ds_rank > y_root_cell.ds_rank) {
      y_root_cell.ds_parent = x_root;
    } else {
      x_root_cell.ds_parent = y_root;
      x_root_cell.ds_rank = x_root_cell.ds_rank + 1;
    }
  }

  Constraint MakeConstraint(const CellLocation& loc) {
    const Cell& cell = grid_(loc.row, loc.col);

    std::vector<CellLocation> locs;
    locs.reserve(cell.adjacent_mines);
    std::size_t flags = 0;

    grid_.ForEachAdjacent(
        loc.row, loc.col,
        [this, &locs, &flags](std::size_t row, std::size_t col) {
          Cell& cell = grid_(row, col);
          if (cell.state == CellState::COVERED) {
            locs.push_back({row, col});
            cell.ds_parent = CellLocation{row, col};
            cell.ds_rank = 0;
          } else if (cell.state == CellState::FLAGGED) {
            ++flags;
          }
          return false;
        });

    std::size_t mines =
        cell.adjacent_mines <= flags ? cell.adjacent_mines - flags : 0;

    return Constraint(std::move(locs), mines);
  }

  std::set<CellLocation> cell_loc_;
  Grid<Cell> grid_;
};

}  // namespace

std::unique_ptr<ConstraintAnalyzer> NewConstrantAnalyzer(Game& game) {
  std::unique_ptr<ConstraintAnalyzer> constraint_analyzer{
      new ConstraintAnalyzerImpl(game.GetRows(), game.GetCols())};
  game.Subscribe(constraint_analyzer.get());
  return constraint_analyzer;
}

}  // namespace solver
}  // namespace mines
