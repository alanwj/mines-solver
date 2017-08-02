#ifndef MINES_GAME_GAME_H_
#define MINES_GAME_GAME_H_

#include <cstddef>
#include <memory>
#include <vector>

namespace mines {

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

    // The game was quit.
    QUIT,

    // A mine should be revealed. Only generated when a game is lost.
    SHOW_MINE,
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

// Represents the actions that may be performed in a Ui.
struct Action {
  enum class Type {
    // Uncover a cell.
    UNCOVER,

    // Chord a cell.
    CHORD,

    // Toggle the flag state of a cell.
    FLAG,

    // Quit the game.
    QUIT,
  };

  Type type;
  std::size_t row;
  std::size_t col;
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
};

// The interface through which a game is played.
class Game {
 public:
  virtual ~Game() = default;

  // Executes the supplied action and returns the resulting events.
  //
  // This is a convenience function that calls the correct Game method
  // based on the Action type.
  std::vector<Event> Execute(const Action& action);

  // Attempts to uncover the specified cell.
  //
  // Does nothing if the cell is flagged or already uncovered.
  //
  // If the cell contains zero adjacent mines, the adjacent cells will be
  // recursively uncovered.
  virtual std::vector<Event> Uncover(std::size_t row, std::size_t col) = 0;

  // Attempts to uncover all adjacent cells that are not flagged.
  //
  // Does nothing if the incorrect number of adjacent cells are flagged.
  virtual std::vector<Event> Chord(std::size_t row, std::size_t col) = 0;

  // Toggles the flag on the specified cell.
  //
  // Does nothing if the cell is already uncovered.
  virtual std::vector<Event> ToggleFlagged(std::size_t row,
                                           std::size_t col) = 0;

  // Quits the game.
  //
  // Returns a single Event of type QUIT.
  virtual std::vector<Event> Quit() = 0;

  // Returns the number of rows in the game.
  virtual std::size_t GetRows() const = 0;

  // Returns the number of columns in the game.
  virtual std::size_t GetCols() const = 0;

  // Returns the number of mines in the game.
  virtual std::size_t GetMines() const = 0;

  // Returns true if the game is still being played, and has not been won or
  // lost.
  virtual bool IsPlaying() const = 0;

  // Returns true if the game ended in a win.
  virtual bool IsWin() const = 0;

  // Returns true if the game ended in a loss.
  virtual bool IsLoss() const = 0;

  // Returns true if the game ended due to calling Quit.
  virtual bool IsQuit() const = 0;
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
