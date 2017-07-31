#include "single_cell_solver.h"

#include <cstddef>
#include <queue>
#include <tuple>
#include <utility>
#include <vector>

#include "compat/make_unique.h"
#include "grid.h"

namespace mines {
namespace solver {
namespace single_cell {

namespace {

class SingleCellSolver : public Solver {
 public:
  SingleCellSolver(const Game& game) : grid_(game.GetRows(), game.GetCols()) {
    grid_.ForEach([this](std::size_t row, std::size_t col, Cell& cell) {
      cell.adjacent_covered = CountAdjacentCells(row, col);
    });
  }

  ~SingleCellSolver() final = default;

  void Update(const std::vector<Event>& events) final {
    for (const Event& ev : events) {
      if (!grid_.IsValid(ev.row, ev.col)) {
        continue;
      }
      Cell& cell = grid_(ev.row, ev.col);
      cell.adjacent_mines = ev.adjacent_mines;
      switch (ev.type) {
        case Event::Type::UNCOVER:
          cell.state = CellState::UNCOVERED;
          UpdateAdjacentCovered(ev.row, ev.col);
          QueueAnalyze(ev.row, ev.col);
          QueueAnalyzeAdjacent(ev.row, ev.col);
          break;
        case Event::Type::FLAG:
          cell.state = CellState::FLAGGED;
          UpdateAdjacentFlags(ev.row, ev.col, true);
          QueueAnalyzeAdjacent(ev.row, ev.col);
          break;
        case Event::Type::UNFLAG:
          cell.state = CellState::COVERED;
          UpdateAdjacentFlags(ev.row, ev.col, false);
          QueueAnalyzeAdjacent(ev.row, ev.col);
          break;
        case Event::Type::WIN:
          // No new knowledge.
          break;
        case Event::Type::LOSS:
          cell.state = CellState::LOSING_MINE;
          break;
        case Event::Type::QUIT:
          // No new knowledge.
          break;
        case Event::Type::SHOW_MINE:
          cell.state = CellState::MINE;
          break;
      }
    }
  }

  std::vector<Action> Analyze() final {
    std::vector<Action> actions;
    while (!aq_.empty()) {
      std::size_t row = std::get<0>(aq_.front());
      std::size_t col = std::get<1>(aq_.front());
      aq_.pop();

      actions = AnalyzeCell(row, col);
      if (!actions.empty()) {
        break;
      }
    }
    return actions;
  }

 private:
  // Counts the number of adjacent cells.
  std::size_t CountAdjacentCells(std::size_t row, std::size_t col) const {
    return grid_.ForEachAdjacent(
        row, col, [this](std::size_t row, std::size_t col) { return true; });
  }

  // Updates the adjacent cells to subtract one from their adjacent_covered
  // count. This should be called in response to an UNCOVER event.
  void UpdateAdjacentCovered(std::size_t row, std::size_t col) {
    grid_.ForEachAdjacent(row, col, [this](std::size_t row, std::size_t col) {
      --grid_(row, col).adjacent_covered;
      return false;
    });
  }

  // Updates the adjacent cells to add or subtract one from their adjacent_flags
  // count. This should be called in response to a FLAG or UNFLAG event.
  void UpdateAdjacentFlags(std::size_t row, std::size_t col, bool flag) {
    grid_.ForEachAdjacent(row, col,
                          [this, flag](std::size_t row, std::size_t col) {
                            grid_(row, col).adjacent_flags += flag ? 1 : -1;
                            return false;
                          });
  }

  // Flags adjacent cells that are covered.
  std::vector<Action> FlagAdjacentCovered(std::size_t row,
                                          std::size_t col) const {
    std::vector<Action> a;
    grid_.ForEachAdjacent(row, col,
                          [this, &a](std::size_t row, std::size_t col) {
                            if (grid_(row, col).state == CellState::COVERED) {
                              a.push_back(Action{Action::Type::FLAG, row, col});
                            }
                            return false;
                          });
    return a;
  }

  // Queues a cell to be analyzed. Does nothing for cells that are covered or
  // have no adjacent mines.
  void QueueAnalyze(std::size_t row, std::size_t col) {
    const Cell& cell = grid_(row, col);
    if (cell.adjacent_mines != 0 && cell.state == CellState::UNCOVERED) {
      aq_.push(std::make_tuple(row, col));
    }
  }

  // Queues analysis of adjacent cells.
  void QueueAnalyzeAdjacent(std::size_t row, std::size_t col) {
    grid_.ForEachAdjacent(row, col, [this](std::size_t row, std::size_t col) {
      QueueAnalyze(row, col);
      return false;
    });
  }

  // Analyzes a cell and returns a (possibly empty) set of actions to perform.
  std::vector<Action> AnalyzeCell(std::size_t row, std::size_t col) {
    if (!grid_.IsValid(row, col)) {
      return std::vector<Action>();
    }
    const Cell& cell = grid_(row, col);

    // We only analyze cells that are uncovered with at least one adjacent mine.
    if (cell.adjacent_mines == 0 || cell.state != CellState::UNCOVERED) {
      return std::vector<Action>();
    }

    if (cell.adjacent_mines == cell.adjacent_flags) {
      return std::vector<Action>{Action{Action::Type::CHORD, row, col}};
    }
    if (cell.adjacent_mines == cell.adjacent_covered) {
      return FlagAdjacentCovered(row, col);
    }
    return std::vector<Action>();
  }

  // Represents the solver's knowledge about a cell.
  struct Cell {
    CellState state = CellState::COVERED;

    // The number of adjacent mines.
    // Only valid if the state is UNCOVERED.
    std::size_t adjacent_mines = 0;

    // The number of adjacent cells that are flagged.
    std::size_t adjacent_flags = 0;

    // The number of adjacent cells that are still covered.
    // Note: This number will be computed in in the constructor.
    std::size_t adjacent_covered = 0;
  };

  Grid<Cell> grid_;

  // The queue of cells to analyze.
  std::queue<std::tuple<std::size_t, std::size_t>> aq_;
};

}  // namespace

std::unique_ptr<Solver> New(const Game& game) {
  return std::make_unique<SingleCellSolver>(game);
}

}  // namespace single_cell
}  // namespace solver
}  // namespace mines
