#include "mines/solver/constraint_analyzer.h"

#include <functional>
#include <unordered_map>
#include <unordered_set>

#include "mines/game/grid.h"

namespace std {

// Hash function for CellLocation.
template <>
struct hash<mines::solver::CellLocation> {
  // This is 2^64 / phi. Any irrational number would do. The goal is just
  // "random" bits.
  static constexpr std::size_t magic = 0x9e3779b97f4a7a97;

  std::size_t operator()(const mines::solver::CellLocation& location) const {
    const std::hash<std::size_t> h;
    std::size_t seed = h(location.row);
    seed ^= h(location.col) + magic + (seed << 6) + (seed >> 2);
    return seed;
  }
};

}  // namespace std

namespace mines {
namespace solver {

namespace {

class CellLocationMap {
 public:
  explicit CellLocationMap(std::size_t bucket_count)
      : index_map_(bucket_count) {
    locations_.reserve(bucket_count);
  }

  std::size_t operator[](const CellLocation& location) {
    auto result =
        index_map_.insert(std::make_pair(location, locations_.size()));
    if (result.second) {
      locations_.push_back(location);
    }
    return result.first->second;
  }

  std::vector<CellLocation>&& MoveLocations() { return std::move(locations_); }

 private:
  std::unordered_map<CellLocation, std::size_t> index_map_;
  std::vector<CellLocation> locations_;
};

class ConstraintAnalyzerImpl : public ConstraintAnalyzer {
 public:
  ConstraintAnalyzerImpl(std::size_t rows, std::size_t cols)
      : grid_(rows, cols), game_over_(false) {}

  ~ConstraintAnalyzerImpl() final = default;

  ConstraintAnalysis Analyze() final {
    if (game_over_) {
      return ConstraintAnalysis();
    }

    // Make all the constraints.
    CellLocationMap location_map(8 * cell_locations_.size());
    std::vector<Constraint> constraints;
    constraints.reserve(cell_locations_.size());
    for (auto it = cell_locations_.begin(); it != cell_locations_.end();) {
      auto cur = it++;
      Constraint constraint{MakeConstraint(*cur, location_map)};
      if (constraint.IsStable()) {
        // The constraint has become stable, so we may remove it from analysis.
        cell_locations_.erase(cur);
      } else {
        constraints.push_back(std::move(constraint));
      }
    }
    std::vector<CellLocation> locations{location_map.MoveLocations()};

    // Union all of the locations in each constraint.
    // This builds the disjoint set tree in place within the grid.
    for (const Constraint& constraint : constraints) {
      const auto& location_indexes = constraint.GetLocationIndexes();
      if (location_indexes.empty()) {
        continue;
      }
      auto first = location_indexes.begin();
      const CellLocation& first_location = locations[*first];
      for (auto it = first + 1; it != location_indexes.end(); ++it) {
        Union(first_location, locations[*it]);
      }
    }

    // Map the disjoint set tree into explicit sets.
    std::unordered_map<CellLocation, std::vector<Constraint>> constraint_map;
    for (Constraint& constraint : constraints) {
      const auto& location_indexes = constraint.GetLocationIndexes();
      if (location_indexes.empty()) {
        continue;
      }
      CellLocation location = Find(locations[location_indexes[0]]);
      constraint_map[location].push_back(std::move(constraint));
    }

    // Create regions from the explicit sets.
    std::vector<Region> regions;
    for (auto& p : constraint_map) {
      regions.push_back(Region(std::move(p.second)));
    }

    return ConstraintAnalysis(std::move(locations), std::move(regions));
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
          cell_locations_.insert({event.row, event.col});
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
            cell_locations_.insert({row, col});
          }
          return false;
        });
        break;
      case Event::Type::WIN:
        game_over_ = true;
        break;
      case Event::Type::LOSS:
        game_over_ = true;
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

  CellLocation Find(const CellLocation& location) {
    Cell& cell = grid_(location.row, location.col);
    if (cell.ds_parent != location) {
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

  Constraint MakeConstraint(const CellLocation& location,
                            CellLocationMap& location_map) {
    const Cell& cell = grid_(location.row, location.col);

    std::vector<std::size_t> location_indexes;
    location_indexes.reserve(cell.adjacent_mines);
    std::size_t flags = 0;

    auto f = [this, &location_map, &location_indexes, &flags](std::size_t row,
                                                              std::size_t col) {
      Cell& cell = grid_(row, col);
      if (cell.state == CellState::COVERED) {
        location_indexes.push_back(location_map[{row, col}]);
        cell.ds_parent = CellLocation{row, col};
        cell.ds_rank = 0;
      } else if (cell.state == CellState::FLAGGED) {
        ++flags;
      }
      return false;
    };
    grid_.ForEachAdjacent(location.row, location.col, f);

    std::size_t mines =
        flags <= cell.adjacent_mines ? cell.adjacent_mines - flags : 0;

    return Constraint(std::move(location_indexes), mines);
  }

  std::unordered_set<CellLocation> cell_locations_;
  Grid<Cell> grid_;
  bool game_over_;
};

}  // namespace

std::unique_ptr<ConstraintAnalyzer> NewConstraintAnalyzer(Game& game) {
  std::unique_ptr<ConstraintAnalyzer> constraint_analyzer{
      new ConstraintAnalyzerImpl(game.GetRows(), game.GetCols())};
  game.Subscribe(constraint_analyzer.get());
  return constraint_analyzer;
}

}  // namespace solver
}  // namespace mines
