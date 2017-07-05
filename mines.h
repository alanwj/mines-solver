#ifndef MINES_H_
#define MINES_H_

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

    // The game was lost.
    LOSS,

    // The game was won.
    WIN,

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

// The interface through which a game is played.
class Game {
 public:
  virtual ~Game();

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

  // Returns the number of rows in the game.
  virtual std::size_t GetRows() const = 0;

  // Returns the number of columns in the game.
  virtual std::size_t GetCols() const = 0;

  // Returns the number of mines in the game.
  virtual std::size_t GetMines() const = 0;

  // Returns true if the game is still being played, and has not been won or
  // lost.
  virtual bool IsPlaying() const = 0;

  // Returns true if the game ending in a win.
  virtual bool IsWin() const = 0;

  // Returns true if the game ending in a loss.
  virtual bool IsLoss() const = 0;
};

// Creates a new game.
//   rows - The number of rows.
//   cols - The number of columns.
//   mines - The number of mines.
//   seed - Seed for the PRNG to generate the mine locations.
std::unique_ptr<Game> NewGame(std::size_t rows, std::size_t cols,
                              std::size_t mines, unsigned seed);

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

  // Returns true if this is a quit action.
  constexpr bool IsQuit() const { return type == Type::QUIT; }

  // Dispatches the action to the game.
  //
  // This is a convenience function to be shared among player implementations.
  //
  // Does NOT handle the QUIT action. This action should be explicitly handled
  // in the player implementation.
  std::vector<Event> Dispatch(Game& g) const;

  Type type;
  std::size_t row;
  std::size_t col;
};

// An abstract representation of a player's knowledge about the state of the
// game.
class Knowledge {
 public:
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

  virtual ~Knowledge();

  // Returns the number of rows.
  virtual std::size_t GetRows() const = 0;

  // Returns the number of columns.
  virtual std::size_t GetCols() const = 0;

  // Gets the state of a particular cell.
  virtual CellState GetState(std::size_t row, std::size_t col) const = 0;

  // Gets the number of adjacent mines.
  // This value is only meaningful if the state is UNCOVERED.
  virtual std::size_t GetAdjacentMines(std::size_t row,
                                       std::size_t col) const = 0;
};

// Abstract representation of a user interface.
class Ui {
 public:
  virtual ~Ui();

  // Blocks until a action is requested.
  virtual Action BlockUntilAction() = 0;

  // Updates the UI based on the available player knowledge.
  virtual void Update(const Knowledge& k) const = 0;
};

// Abstract representation of a player.
class Player {
 public:
  virtual ~Player();

  virtual bool Play(Game& g, Ui& ui) = 0;
};

}  // namespace mines

#endif  // MINES_H_
