#ifndef MINES_SOLVER_REGION_ANALYZER_H_
#define MINES_SOLVER_REGION_ANALYZER_H_

#include <cstddef>
#include <map>
#include <set>
#include <utility>
#include <vector>

#include "mines/game/game.h"
#include "mines/game/grid.h"

namespace mines {
namespace solver {

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

class Rule {
 public:
  Rule(std::vector<CellLocation> locs, std::size_t mines, std::size_t flags)
      : locs_(std::move(locs)), mines_(mines), flags_(flags) {}

  ~Rule() = default;

  Rule(const Rule&) = default;
  Rule& operator=(const Rule&) = default;

  Rule(Rule&&) = default;
  Rule& operator=(Rule&&) = default;

  bool IsStable() const {
    return mines_ == 0 && locs_.empty();
  }

  const std::vector<CellLocation>& GetLocations() const {
    return locs_;
  }

 private:
  std::vector<CellLocation> locs_;
  std::size_t mines_;
  std::size_t flags_;
};

class RuleAnalyzer : public EventSubscriber {
 public:
  RuleAnalyzer() {}

  std::vector<Rule> Analyze() {
    std::vector<Rule> rules;
    for (auto it = cell_loc_.begin(); it != cell_loc_.end();) {
      auto cur = it++;
      Rule rule{MakeRule(*cur)};
      if (rule.IsStable()) {
        // The rule has become stable, so we may remove it from analysis.
        cell_loc_.erase(cur);
      } else {
        rules.push_back(std::move(rule));
      }
    }
    return rules;
  }

  std::vector<std::vector<Rule>> AnalyzeRegions(std::vector<Rule>& rules) {
    for (const Rule& rule : rules) {
      const std::vector<CellLocation>& locations = rule.GetLocations();
      if (locations.empty()) {
        continue;
      }
      auto first = locations.begin();
      for (auto it = first + 1; it != locations.end(); ++it) {
        Union(*first, *it);
      }
    }

    std::map<CellLocation, std::vector<Rule>> rule_map;
    for (Rule& rule : rules) {
      const std::vector<CellLocation>& locations = rule.GetLocations();
      if (locations.empty()) {
        continue;
      }
      CellLocation loc = Find(locations[0]);
      rule_map[loc].push_back(std::move(rule));
    }

    std::vector<std::vector<Rule>> regions;
    for (auto& p : rule_map) {
      regions.push_back(std::move(p.second));
    }

    return regions;
  }

 private:
  struct Cell {
    CellState state;
    std::size_t adjacent_mines;

    CellLocation ds_parent;
    std::size_t ds_rank;
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

  Rule MakeRule(const CellLocation& loc) {
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

    return Rule(std::move(locs), mines, flags);
  }

  void NotifyEvent(const Event& event) {
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
        grid_.ForEachAdjacent(event.row, event.col, [this](std::size_t row, std::size_t col) {
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

  std::set<CellLocation> cell_loc_;
  Grid<Cell> grid_;
};

}  // namespace solver
}  // namespace mines

#endif  // MINES_SOLVER_REGION_ANALYZER_H_
