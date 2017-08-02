#include "mines/game/game.h"

#include <random>

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

// Convenience function to create a Quit event.
constexpr Event QuitEvent() { return Event{Event::Type::QUIT, 0, 0, 0}; }

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
        state_(State::PLAYING),
        remaining_covered_(rows * cols - mines),
        grid_(rows, cols) {
    // Assign the mines.
    std::default_random_engine g;
    g.seed(seed);
    std::uniform_int_distribution<std::size_t> d(0, rows * cols - 1);
    for (std::size_t remaining_mines = mines; remaining_mines > 0;) {
      std::size_t rnd = d(g);
      std::size_t row = rnd / cols;
      std::size_t col = rnd % cols;

      Cell& cell = grid_(row, col);
      if (cell.SetMine()) {
        --remaining_mines;
      }
    }
  }

  ~GameImpl() final = default;

  std::vector<Event> Uncover(std::size_t row, std::size_t col) final {
    std::vector<Event> events;
    Uncover(row, col, events);
    return events;
  }

  std::vector<Event> Chord(std::size_t row, std::size_t col) final {
    std::vector<Event> events;
    Chord(row, col, events);
    return events;
  }

  std::vector<Event> ToggleFlagged(std::size_t row, std::size_t col) final {
    std::vector<Event> events;
    ToggleFlagged(row, col, events);
    return events;
  }

  std::vector<Event> Quit() final {
    state_ = State::QUIT;
    return std::vector<Event>{QuitEvent()};
  }

  std::size_t GetRows() const final { return grid_.GetRows(); }

  std::size_t GetCols() const final { return grid_.GetCols(); }

  std::size_t GetMines() const final { return mines_; }

  bool IsPlaying() const final { return state_ == State::PLAYING; }

  bool IsWin() const final { return state_ == State::WIN; }

  bool IsLoss() const final { return state_ == State::LOSS; }

  bool IsQuit() const final { return state_ == State::QUIT; }

 private:
  // Current game state.
  enum class State {
    // The game is ongoing.
    PLAYING,

    // The game ended in a win.
    WIN,

    // The game ended in a loss.
    LOSS,

    // The game ended due to calling Quit.
    QUIT,
  };

  void Uncover(std::size_t row, std::size_t col, std::vector<Event>& events) {
    if (!IsPlaying() || !grid_.IsValid(row, col)) {
      return;
    }
    Cell& cell = grid_(row, col);

    if (!cell.Uncover()) {
      // Cell was flagged or already uncovered.
      return;
    }

    // If a mine was uncovered this is a loss.
    if (cell.IsMine()) {
      ShowAllMinesAndLose(row, col, events);
      return;
    }

    std::size_t adjacent_mines = CountAdjacentMines(row, col);
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
      UncoverAdjacent(row, col, events);
    }
  }

  void Chord(std::size_t row, std::size_t col, std::vector<Event>& events) {
    if (!IsPlaying() || !grid_.IsValid(row, col)) {
      return;
    }
    Cell& cell = grid_(row, col);

    // Cannot chord a flagged or covered cell.
    if (cell.IsFlaggedOrCovered()) {
      return;
    }

    // Cannot chord if the the wrong number of cells are flagged.
    if (CountAdjacentMines(row, col) != CountAdjacentFlagged(row, col)) {
      return;
    }

    UncoverAdjacent(row, col, events);
  }

  void ToggleFlagged(std::size_t row, std::size_t col,
                     std::vector<Event>& events) {
    if (!IsPlaying() || !grid_.IsValid(row, col)) {
      return;
    }
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

  // Recursively uncovers all adjacent cells.
  void UncoverAdjacent(std::size_t row, std::size_t col,
                       std::vector<Event>& events) {
    grid_.ForEachAdjacent(row, col,
                          [this, &events](std::size_t row, std::size_t col) {
                            Uncover(row, col, events);
                            return false;
                          });
  }

  // Generates events to show all mines, followed by a lose event at the given
  // location.
  void ShowAllMinesAndLose(std::size_t row, std::size_t col,
                           std::vector<Event>& events) {
    grid_.ForEach(
        [&events](std::size_t row, std::size_t col, const Cell& cell) {
          if (cell.IsMine()) {
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
};

}  // namespace

std::vector<Event> Game::Execute(const Action& action) {
  switch (action.type) {
    case Action::Type::UNCOVER:
      return Uncover(action.row, action.col);
    case Action::Type::CHORD:
      return Chord(action.row, action.col);
    case Action::Type::FLAG:
      return ToggleFlagged(action.row, action.col);
    case Action::Type::QUIT:
      return Quit();
    default:
      return std::vector<Event>();
  }
}

std::unique_ptr<Game> NewGame(std::size_t rows, std::size_t cols,
                              std::size_t mines, unsigned seed) {
  if (rows == 0 || cols == 0 || mines > rows * cols) {
    return nullptr;
  }
  return std::make_unique<GameImpl>(rows, cols, mines, seed);
}

}  // namespace mines