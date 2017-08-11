#include "mines/game/game.h"

#include <algorithm>
#include <functional>
#include <queue>
#include <random>
#include <tuple>

#include "mines/compat/make_unique.h"
#include "mines/game/grid.h"

namespace mines {

namespace {

// Convenience function to create an UNCOVER event.
constexpr Event UncoverEvent(std::size_t row, std::size_t col,
                             std::size_t adjacent_mines) {
  return Event{Event::Type::UNCOVER, row, col, adjacent_mines};
}

// Convenience function to create a FLAG event.
constexpr Event FlagEvent(std::size_t row, std::size_t col) {
  return Event{Event::Type::FLAG, row, col, 0};
}

// Convenience function to create an UNFLAG event.
constexpr Event UnflagEvent(std::size_t row, std::size_t col) {
  return Event{Event::Type::UNFLAG, row, col, 0};
}

// Convenience function to create a WIN event.
constexpr Event WinEvent(std::size_t row, std::size_t col) {
  return Event{Event::Type::WIN, row, col, 0};
}

// Convenience function to create a LOSS event.
constexpr Event LossEvent(std::size_t row, std::size_t col) {
  return Event{Event::Type::LOSS, row, col, 0};
}

// Convenience function to create a SHOW_MINE event.
constexpr Event ShowMineEvent(std::size_t row, std::size_t col) {
  return Event{Event::Type::SHOW_MINE, row, col, 0};
}

// A single cell in a game.
class Cell {
 public:
  // Returns true if the cell contains a mine.
  bool IsMine() const { return is_mine_; }

  // Sets this cell as a mine.
  //
  // Returns false if the cell was already a mine.
  bool SetMine() {
    if (is_mine_) {
      return false;
    }
    is_mine_ = true;
    return true;
  }

  // Returns true if the cell is flagged.
  bool IsFlagged() const { return state_ == State::FLAGGED; }

  // Returns true if the cell is covered.
  bool IsCovered() const { return state_ == State::COVERED; }

  // Toggles a cell between flagged and covered.
  //
  // Returns false if the cell is uncovered.
  bool ToggleFlagged() {
    if (state_ == State::UNCOVERED) {
      return false;
    }
    switch (state_) {
      case State::COVERED:
        state_ = State::FLAGGED;
        return true;
      case State::FLAGGED:
        state_ = State::COVERED;
        return true;
      case State::UNCOVERED:
      default:
        return false;
    }
  }

  // Returns true if the cell is flagged or covered.
  bool IsFlaggedOrCovered() const {
    return state_ == State::COVERED || state_ == State::FLAGGED;
  }

  // Uncovers the cell if it is covered.
  //
  // Returns false (and does nothing) if the cell is flagged or uncovered.
  bool Uncover() {
    // Cannot uncover cells that are flagged or already uncovered.
    if (state_ == State::FLAGGED || state_ == State::UNCOVERED) {
      return false;
    }
    state_ = State::UNCOVERED;
    return true;
  }

 private:
  enum class State {
    // The cell is covered.
    COVERED,

    // The cell is uncovered.
    UNCOVERED,

    // The cell is flagged.
    FLAGGED,
  };

  bool is_mine_ = false;
  State state_ = State::COVERED;
};

// The game implementation.
class GameImpl : public Game {
 public:
  GameImpl(std::size_t rows, std::size_t cols, std::size_t mines, unsigned seed)
      : mines_(mines),
        state_(State::NEW),
        remaining_covered_(rows * cols - mines),
        grid_(rows, cols) {
    // Assign the mines.
    std::default_random_engine g;
    g.seed(seed);
    std::uniform_int_distribution<std::size_t> d(0, rows * cols - 1);
    auto rng = std::bind(d, g);

    for (std::size_t remaining_mines = mines; remaining_mines > 0;) {
      const std::size_t rnd = rng();
      Cell& cell = grid_(rnd / cols, rnd % cols);
      if (cell.SetMine()) {
        --remaining_mines;
      }
    }

    // Choose a backup cell to be a mine if the first cell uncovered is a mine.
    do {
      const std::size_t rnd = rng();
      backup_cell_ = &grid_(rnd / cols, rnd % cols);
    } while (backup_cell_->IsMine());
  }

  ~GameImpl() final = default;

  void Execute(const Action& action) final {
    if (IsGameOver() || !grid_.IsValid(action.row, action.col)) {
      return;
    }

    std::vector<Event> events;
    switch (action.type) {
      case Action::Type::UNCOVER:
        Uncover(action.row, action.col, events);
        break;
      case Action::Type::CHORD:
        Chord(action.row, action.col, events);
        break;
      case Action::Type::FLAG:
        ToggleFlagged(action.row, action.col, events);
        break;
      default:
        break;
    }

    if (state_ == State::NEW) {
      state_ = State::PLAYING;
    }

    for (const Event& event : events) {
      for (EventSubscriber* subscriber : subscribers_) {
        subscriber->NotifyEvent(event);
      }
    }
  }

  void Subscribe(EventSubscriber* subscriber) final {
    subscribers_.push_back(subscriber);
  }

  std::size_t GetRows() const final { return grid_.GetRows(); }

  std::size_t GetCols() const final { return grid_.GetCols(); }

  std::size_t GetMines() const final { return mines_; }

  State GetState() const final { return state_; }

 private:
  // Attempts to uncover the specified cell.
  //
  // Does nothing if the cell is flagged or already uncovered.
  //
  // If the cell contains zero adjacent mines, the adjacent cells will be
  // recursively uncovered.
  void Uncover(std::size_t row, std::size_t col, std::vector<Event>& events) {
    Cell& cell = grid_(row, col);

    if (state_ == State::NEW && cell.IsMine()) {
      std::swap(cell, *backup_cell_);
    }

    UncoverAdjacent(row, col, true, events);
  }

  // Attempts to uncover all adjacent cells that are not flagged.
  //
  // Does nothing if the incorrect number of adjacent cells are flagged.
  void Chord(std::size_t row, std::size_t col, std::vector<Event>& events) {
    Cell& cell = grid_(row, col);

    // Cannot chord a flagged or covered cell.
    if (cell.IsFlaggedOrCovered()) {
      return;
    }

    // Cannot chord if the the wrong number of cells are flagged.
    if (CountAdjacentMines(row, col) != CountAdjacentFlagged(row, col)) {
      return;
    }

    UncoverAdjacent(row, col, false, events);
  }

  // Toggles the flag on the specified cell.
  //
  // Does nothing if the cell is already uncovered.
  void ToggleFlagged(std::size_t row, std::size_t col,
                     std::vector<Event>& events) {
    Cell& cell = grid_(row, col);
    if (cell.ToggleFlagged()) {
      events.push_back(cell.IsFlagged() ? FlagEvent(row, col)
                                        : UnflagEvent(row, col));
    }
  }

  // Counts the number of adjacent mines.
  std::size_t CountAdjacentMines(std::size_t row, std::size_t col) const {
    return grid_.ForEachAdjacent(row, col,
                                 [this](std::size_t row, std::size_t col) {
                                   return grid_(row, col).IsMine();
                                 });
  }

  // Counts the number of adjacent flagged cells.
  std::size_t CountAdjacentFlagged(std::size_t row, std::size_t col) const {
    return grid_.ForEachAdjacent(row, col,
                                 [this](std::size_t row, std::size_t col) {
                                   return grid_(row, col).IsFlagged();
                                 });
  }

  // Uncovers adjacent cells in a breadth first manner.
  //
  // If start_at_current is true, the cell identified by row and col is the
  // first node uncovered. Otherwise the adjacent nodes are uncovered.
  //
  // If an uncovered cell has zero adjacent mines, its adjacent cells will also
  // be uncovered.
  void UncoverAdjacent(std::size_t row, std::size_t col, bool start_at_current,
                       std::vector<Event>& events) {
    std::queue<std::tuple<std::size_t, std::size_t>> uncover_queue;
    auto queue_cell = [this, &uncover_queue](std::size_t row, std::size_t col) {
      if (grid_(row, col).IsCovered()) {
        uncover_queue.push(std::make_tuple(row, col));
      }
      return false;
    };
    if (start_at_current) {
      uncover_queue.push(std::make_tuple(row, col));
    } else {
      grid_.ForEachAdjacent(row, col, queue_cell);
    }

    while (!uncover_queue.empty()) {
      std::tie(row, col) = uncover_queue.front();
      uncover_queue.pop();
      Cell& cell = grid_(row, col);

      if (!cell.Uncover()) {
        // Cell was flagged or already uncovered.
        continue;
      }

      // If a mine was uncovered this is a loss.
      if (cell.IsMine()) {
        ShowAllMinesAndLose(row, col, events);
        return;
      }

      const std::size_t adjacent_mines = CountAdjacentMines(row, col);
      events.push_back(UncoverEvent(row, col, adjacent_mines));
      --remaining_covered_;

      // If there are no more mines to uncover this is a win.
      if (remaining_covered_ == 0) {
        events.push_back(WinEvent(row, col));
        state_ = State::WIN;
        return;
      }

      // Automatically expand empty areas.
      if (adjacent_mines == 0) {
        grid_.ForEachAdjacent(row, col, queue_cell);
      }
    }
  }

  // Generates events to show all mines, followed by a lose event at the given
  // location.
  void ShowAllMinesAndLose(std::size_t row, std::size_t col,
                           std::vector<Event>& events) {
    grid_.ForEach(
        [&events](std::size_t row, std::size_t col, const Cell& cell) {
          if (cell.IsMine() && !cell.IsFlagged()) {
            events.push_back(ShowMineEvent(row, col));
          }
        });

    events.push_back(LossEvent(row, col));
    state_ = State::LOSS;
  }

  const std::size_t mines_;
  State state_;
  std::size_t remaining_covered_;
  Grid<Cell> grid_;
  Cell* backup_cell_;
  std::vector<EventSubscriber*> subscribers_;
};

}  // namespace

std::unique_ptr<Game> NewGame(std::size_t rows, std::size_t cols,
                              std::size_t mines, unsigned seed) {
  if (rows == 0 || cols == 0 || mines > rows * cols) {
    return nullptr;
  }
  return MakeUnique<GameImpl>(rows, cols, mines, seed);
}

}  // namespace mines
