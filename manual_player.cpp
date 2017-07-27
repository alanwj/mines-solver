#include "manual_player.h"

#include <cstddef>
#include <vector>

#include "compat/make_unique.h"
#include "grid.h"

namespace mines {
namespace manual {

namespace {

// Implemenation of knowledge that only remembers the minimum necessary for
// display.
class BasicKnowledge : public Knowledge {
 public:
  BasicKnowledge(std::size_t rows, std::size_t cols) : grid_(rows, cols) {}

  ~BasicKnowledge() final = default;

  std::size_t GetRows() const final { return grid_.GetRows(); }

  std::size_t GetCols() const final { return grid_.GetCols(); }

  CellState GetState(std::size_t row, std::size_t col) const final {
    if (!grid_.IsValid(row, col)) {
      return CellState::UNCOVERED;
    }
    return grid_(row, col).state;
  }

  std::size_t GetAdjacentMines(std::size_t row, std::size_t col) const final {
    if (!grid_.IsValid(row, col)) {
      return 0;
    }
    return grid_(row, col).adjacent_mines;
  }

  // Updates player knowledge based on the event.
  void Update(const Event& ev) {
    if (!grid_.IsValid(ev.row, ev.col)) {
      return;
    }
    Cell& cell = grid_(ev.row, ev.col);
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
      case Event::Type::QUIT:
        // No new knowledge.
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

  Grid<Cell> grid_;
};

class ManualPlayer : public Player {
 public:
  ManualPlayer() = default;

  ~ManualPlayer() final = default;

  ManualPlayer(const ManualPlayer&) = delete;
  ManualPlayer& operator=(const ManualPlayer&) = delete;

  void Play(Game& g, Ui& ui) final {
    const std::size_t rows = g.GetRows();
    const std::size_t cols = g.GetCols();

    BasicKnowledge k(rows, cols);

    while (g.IsPlaying()) {
      ui.Update(k);

      Action action = ui.BlockUntilAction();
      for (const Event& ev : g.Execute(action)) {
        k.Update(ev);
      }
    }

    ui.Update(k);
  }
};

}  // namespace

std::unique_ptr<Player> NewPlayer() { return std::make_unique<ManualPlayer>(); }

}  // namespace manual
}  // namespace mines
