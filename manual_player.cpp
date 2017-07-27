#include "manual_player.h"

#include <cstddef>
#include <vector>

#include "compat/make_unique.h"

namespace mines {
namespace manual {

namespace {

// Implemenation of knowledge that only remembers the minimum necessary for
// display.
class BasicKnowledge : public Knowledge {
 public:
  BasicKnowledge(std::size_t rows, std::size_t cols)
      : rows_(rows), cols_(cols), cells_(rows, std::vector<Cell>(cols)) {}

  ~BasicKnowledge() final = default;

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

  // Returns true if the row and column are valid.
  bool IsValid(std::size_t row, std::size_t col) const {
    return row < rows_ && col < cols_;
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
        break;
      case Event::Type::FLAG:
        cell.state = CellState::FLAGGED;
        break;
      case Event::Type::UNFLAG:
        cell.state = CellState::COVERED;
        break;
      case Event::Type::WIN:
        // No new knowledge.
        break;
      case Event::Type::LOSS:
        cell.state = CellState::LOSING_MINE;
        break;
      case Event::Type::SHOW_MINE:
        cell.state = CellState::MINE;
        break;
    }
  }

 private:
  // Represents player knowledge about a cell.
  struct Cell {
    CellState state = CellState::COVERED;

    // The number of adjacent mines.
    // Only valid if the state is UNCOVERED.
    std::size_t adjacent_mines = 0;
  };

  const std::size_t rows_;
  const std::size_t cols_;

  std::vector<std::vector<Cell>> cells_;
};

class ManualPlayer : public Player {
 public:
  ManualPlayer() = default;

  ~ManualPlayer() final = default;

  ManualPlayer(const ManualPlayer&) = delete;
  ManualPlayer& operator=(const ManualPlayer&) = delete;

  bool Play(Game& g, Ui& ui) final {
    const std::size_t rows = g.GetRows();
    const std::size_t cols = g.GetCols();

    BasicKnowledge k(rows, cols);

    while (g.IsPlaying()) {
      ui.Update(k);

      Action a = ui.BlockUntilAction();
      if (a.IsQuit()) {
        return false;
      }
      std::vector<Event> events = a.Dispatch(g);

      for (const Event& ev : events) {
        k.Update(ev);
      }
    }

    ui.Update(k);
    return g.IsWin();
  }
};

}  // namespace

std::unique_ptr<Player> NewPlayer() { return std::make_unique<ManualPlayer>(); }

}  // namespace manual
}  // namespace mines
