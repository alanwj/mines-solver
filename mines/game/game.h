#ifndef MINES_GAME_GAME_H_
#define MINES_GAME_GAME_H_

#include <cstddef>
#include <memory>
#include <vector>

namespace mines {

// Represents the actions that may be performed in a Ui.
struct Action {
  enum class Type {
    // Uncover a cell.
    UNCOVER,

    // Chord a cell.
    CHORD,

    // Toggle the flag state of a cell.
    FLAG,
  };

  Type type;
  std::size_t row;
  std::size_t col;
};

// Events are generated in response to actions taken in a game.
struct Event {
  enum class Type {
    // A cell was uncovered.
    UNCOVER,

    // A cell was flagged.
    FLAG,

    // A cell was unflagged.
    UNFLAG,

    // The game was won.
    WIN,

    // The game was lost.
    LOSS,

    // Identifies a mine location. Only generated when a game is lost.
    IDENTIFY_MINE,

    // Identifies a location that was flagged but is not a mine. Only generated
    // when a game is lost.
    IDENTIFY_BAD_FLAG,
  };

  // The type of event.
  Type type;

  // The row and column for which the event was generated.
  // For a LOSS event this was the mine that caused the loss.
  // For a WIN event this was the last square uncovered.
  std::size_t row;
  std::size_t col;

  // The number of mines in adjacent cells.
  // Only set for UNCOVERED events.
  std::size_t adjacent_mines;
};

// Represents the states that a cell can take from a player's point of view.
//
// This exists primarily as a convenience so that an equivalent does not need to
// be redefined in each place knowledge about a cell is stored.
enum class CellState {
  // The cell is uncovered.
  UNCOVERED,

  // The cell is covered (but not flagged).
  COVERED,

  // The cell is flagged.
  FLAGGED,

  // The cell is a mine (revealed when the game is lost).
  MINE,

  // The cell is the mine that caused a loss.
  LOSING_MINE,

  // The cell is flagged but does not contain a mine (revealed when game is
  // lost).
  BAD_FLAG,
};

// Implementations of EventSubscriber may call Game::Subscribe to receive
// event updates as actions are executed.
class EventSubscriber {
 public:
  virtual ~EventSubscriber() = default;

  // Notifies the subscriber that an event occurred.
  virtual void NotifyEvent(const Event& event) = 0;
};

// The interface through which a game is played.
class Game {
 public:
  // Current game state.
  enum class State {
    // A new game is ready but the first action has not occurred.
    NEW,

    // The game is ongoing.
    PLAYING,

    // The game ended in a win.
    WIN,

    // The game ended in a loss.
    LOSS,
  };

  virtual ~Game() = default;

  // Executes the supplied action and updates all subscribers.
  virtual void Execute(const Action& action) = 0;

  // Executes all of the supplied actions.
  //
  // This is a convenience function that repeatedly calls the Execute method
  // above.
  void Execute(const std::vector<Action>& actions) {
    for (const Action& action : actions) {
      Execute(action);
    }
  }

  // Subscribes the given subscriber to receive event updates when actions are
  // executed.
  virtual void Subscribe(EventSubscriber* subscriber) = 0;

  // Returns the number of rows in the game.
  virtual std::size_t GetRows() const = 0;

  // Returns the number of columns in the game.
  virtual std::size_t GetCols() const = 0;

  // Returns the number of mines in the game.
  virtual std::size_t GetMines() const = 0;

  // Returns the current game state.
  virtual State GetState() const = 0;

  // Returns true if the game is over.
  bool IsGameOver() const {
    const State state = GetState();
    return state == State::WIN || state == State::LOSS;
  }
};

// Creates a new game.
//   rows - The number of rows.
//   cols - The number of columns.
//   mines - The number of mines.
//   seed - Seed for the PRNG to generate the mine locations.
std::unique_ptr<Game> NewGame(std::size_t rows, std::size_t cols,
                              std::size_t mines, unsigned seed);

}  // namespace mines

#endif  // MINES_GAME_GAME_H_
