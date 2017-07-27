#include "assisted_player.h"

#include <cstddef>
#include <queue>
#include <tuple>
#include <vector>

#include "compat/make_unique.h"

namespace mines {
namespace assisted {

namespace {

// A knowledge implementatin that can do simple single cell analysis.
class AssistedKnowledge : public Knowledge {
 public:
  AssistedKnowledge(std::size_t rows, std::size_t cols)
      : rows_(rows), cols_(cols), cells_(rows, std::vector<Cell>(cols)) {
    for (std::size_t r = 0; r < rows_; ++r) {
      for (std::size_t c = 0; c < cols_; ++c) {
        cells_[r][c].adjacent_covered = ComputeAdjacentCells(r, c);
      }
    }
  }

  ~AssistedKnowledge() final = default;

  std::size_t GetRows() const final { return rows_; }

  std::size_t GetCols() const final { return cols_; }

  CellState GetState(std::size_t row, std::size_t col) const final {
    if (!IsValid(row, col)) {
      return CellState::UNCOVERED;
    }
    return cells_[row][col].state;
  }

  std::size_t GetAdjacentMines(std::size_t row, std::size_t col) const final {
    if (!IsValid(row, col)) {
      return 0;
    }
    return cells_[row][col].adjacent_mines;
  }

  // Analyzes the current state and returns a (possibly empty) set of actions to
  // perform.
  std::vector<Action> Analyze() {
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

  // Updates player knowledge based on the event.
  void Update(const Event& ev) {
    if (!IsValid(ev.row, ev.col)) {
      return;
    }
    Cell& cell = cells_[ev.row][ev.col];
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

 private:
  template <class Fn>
  std::size_t ForEachAdjacent(std::size_t row, std::size_t col, Fn fn) const {
    // Note: This relies on the fact that unsigned underflow is well defined.
    std::size_t count = 0;
    count += IsValid(row - 1, col - 1) && fn(row - 1, col - 1) ? 1 : 0;
    count += IsValid(row - 1, col - 0) && fn(row - 1, col - 0) ? 1 : 0;
    count += IsValid(row - 1, col + 1) && fn(row - 1, col + 1) ? 1 : 0;
    count += IsValid(row - 0, col - 1) && fn(row - 0, col - 1) ? 1 : 0;
    count += IsValid(row - 0, col + 1) && fn(row - 0, col + 1) ? 1 : 0;
    count += IsValid(row + 1, col - 1) && fn(row + 1, col - 1) ? 1 : 0;
    count += IsValid(row + 1, col - 0) && fn(row + 1, col - 0) ? 1 : 0;
    count += IsValid(row + 1, col + 1) && fn(row + 1, col + 1) ? 1 : 0;
    return count;
  }

  // Returns true if the row and column are valid.
  bool IsValid(std::size_t row, std::size_t col) const {
    return row < rows_ && col < cols_;
  }

  // Updates the adjacent cells to subtract one from their adjacent_covered
  // count. This should be called in response to an UNCOVER event.
  void UpdateAdjacentCovered(std::size_t row, std::size_t col) {
    ForEachAdjacent(row, col, [this](std::size_t row, std::size_t col) {
      --cells_[row][col].adjacent_covered;
      return false;
    });
  }

  // Updates the adjacent cells to add or subtract one from their adjacent_flags
  // count. This should be called in response to a FLAG or UNFLAG event.
  void UpdateAdjacentFlags(std::size_t row, std::size_t col, bool flag) {
    ForEachAdjacent(row, col, [this, flag](std::size_t row, std::size_t col) {
      if (flag) {
        ++cells_[row][col].adjacent_flags;
      } else {
        --cells_[row][col].adjacent_flags;
      }
      return false;
    });
  }

  // Queues a cell to be analyzed. Does nothing for cells that are covered or
  // have no adjacent mines.
  void QueueAnalyze(std::size_t row, std::size_t col) {
    const Cell& cell = cells_[row][col];
    if (cell.adjacent_mines != 0 && cell.state == CellState::UNCOVERED) {
      aq_.push(std::make_tuple(row, col));
    }
  }

  // Queues analysis of adjacent cells.
  void QueueAnalyzeAdjacent(std::size_t row, std::size_t col) {
    ForEachAdjacent(row, col, [this](std::size_t row, std::size_t col) {
      QueueAnalyze(row, col);
      return false;
    });
  }

  // Flags adjacent cells that are uncovered.
  std::vector<Action> FlagAdjacentUncovered(std::size_t row,
                                            std::size_t col) const {
    std::vector<Action> a;
    ForEachAdjacent(row, col, [this, &a](std::size_t row, std::size_t col) {
      if (cells_[row][col].state == CellState::COVERED) {
        a.push_back(Action{Action::Type::FLAG, row, col});
      }
      return false;
    });
    return a;
  }

  // Computes the number of adjacent cells.
  std::size_t ComputeAdjacentCells(std::size_t row, std::size_t col) const {
    std::size_t v = 8;
    if (row == 0) {
      v -= 3;
    }
    if (row == rows_ - 1) {
      v -= 3;
    }
    if (col == 0) {
      v -= 3;
    }
    if (col == cols_ - 1) {
      v -= 3;
    }

    // Avoid double counting the corners.
    if (row == 0 && col == 0) {
      ++v;
    }
    if (row == 0 && col == cols_ - 1) {
      ++v;
    }
    if (row == rows_ - 1 && col == 0) {
      ++v;
    }
    if (row == rows_ - 1 && col == cols_ - 1) {
      ++v;
    }
    return v;
  }

  // Analyzes a cell and returns a (possibly empty) set of actions to perform.
  std::vector<Action> AnalyzeCell(std::size_t row, std::size_t col) {
    if (!IsValid(row, col)) {
      return std::vector<Action>();
    }
    const Cell& cell = cells_[row][col];

    // We only analyze cells that are uncovered with at least one adjacent mine.
    if (cell.adjacent_mines == 0 || cell.state != CellState::UNCOVERED) {
      return std::vector<Action>();
    }

    if (cell.adjacent_mines == cell.adjacent_flags) {
      return std::vector<Action>{Action{Action::Type::CHORD, row, col}};
    }
    if (cell.adjacent_mines == cell.adjacent_covered) {
      return FlagAdjacentUncovered(row, col);
    }
    return std::vector<Action>();
  }

  // Represents player knowledge about a cell.
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

  const std::size_t rows_;
  const std::size_t cols_;

  std::vector<std::vector<Cell>> cells_;

  // The queue of cells to analyze.
  std::queue<std::tuple<std::size_t, std::size_t>> aq_;
};

class AssistedPlayer : public Player {
 public:
  AssistedPlayer() = default;

  ~AssistedPlayer() final = default;

  AssistedPlayer(const AssistedPlayer&) = delete;
  AssistedPlayer& operator=(const AssistedPlayer&) = delete;

  static std::vector<Action> GetActions(AssistedKnowledge& k, Ui& ui) {
    std::vector<Action> actions = k.Analyze();
    if (actions.empty()) {
      // Analysis produced no actions. Get action from user.
      actions.push_back(ui.BlockUntilAction());
    }
    return actions;
  }

  void Play(Game& g, Ui& ui) final {
    const std::size_t rows = g.GetRows();
    const std::size_t cols = g.GetCols();

    AssistedKnowledge k(rows, cols);

    while (g.IsPlaying()) {
      ui.Update(k);

      for (const Action& action : GetActions(k, ui)) {
        for (const Event& ev : action.Dispatch(g)) {
          k.Update(ev);
        }
      }
    }

    ui.Update(k);
  }
};

}  // namespace

std::unique_ptr<Player> NewPlayer() {
  return std::make_unique<AssistedPlayer>();
}

}  // namespace assisted
}  // namespace mines
