#include "mines/ui/text_ui.h"

#include <cstddef>
#include <iostream>
#include <vector>

#include "mines/compat/make_unique.h"
#include "mines/game/grid.h"

namespace mines {
namespace ui {

namespace {

// Represents player knowledge about the current game.
class Knowledge {
 public:
  Knowledge(std::size_t rows, std::size_t cols) : grid_(rows, cols) {}

  // Updates player knowledge based on the event.
  void Update(const std::vector<Event>& events) {
    for (const Event& ev : events) {
      if (!grid_.IsValid(ev.row, ev.col)) {
        continue;
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
  }

  void Print(std::ostream& out) const {
    const std::size_t rows = grid_.GetRows();
    const std::size_t cols = grid_.GetCols();

    for (std::size_t row = 0; row < rows; ++row) {
      for (std::size_t col = 0; col < cols; ++col) {
        const Cell& cell = grid_(row, col);
        switch (cell.state) {
          case CellState::UNCOVERED:
            out << cell.adjacent_mines;
            break;
          case CellState::COVERED:
            out << '-';
            break;
          case CellState::FLAGGED:
            out << 'F';
            break;
          case CellState::MINE:
            out << '*';
            break;
          case CellState::LOSING_MINE:
            out << 'X';
            break;
        }
        out << ' ';
      }
      out << '\n';
    }
    out << '\n';
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

class TextUiImpl : public TextUi {
 public:
  TextUiImpl(std::istream& in, std::ostream& out) : in_(in), out_(out) {}
  ~TextUiImpl() final = default;

  void Play(Game& game, solver::Solver& solver) final {
    Knowledge knowledge(game.GetRows(), game.GetCols());

    while (game.IsPlaying()) {
      knowledge.Print(out_);

      for (const Action& action : GetActions(solver)) {
        const std::vector<Event> events = game.Execute(action);
        knowledge.Update(events);
        solver.Update(events);
      }
    }

    knowledge.Print(out_);

    if (game.IsWin()) {
      out_ << "You win!\n\n";
    } else if (game.IsLoss()) {
      out_ << "You lose.\n\n";
    }
  }

 private:
  std::vector<Action> GetActions(solver::Solver& solver) {
    std::vector<Action> actions = solver.Analyze();
    if (actions.empty()) {
      // Analysis produced no actions. Get action from user.
      actions.push_back(GetActionFromPlayer());
    }
    return actions;
  }

  Action GetActionFromPlayer() {
    Action a;

    for (;;) {
      out_ << "Command: ";

      int c = in_.get();
      bool fail = false;
      switch (c) {
        case 'u':
        case 'U':
          a.type = Action::Type::UNCOVER;
          in_ >> a.row >> a.col;
          break;
        case 'c':
        case 'C':
          a.type = Action::Type::CHORD;
          in_ >> a.row >> a.col;
          break;
        case 'f':
        case 'F':
          a.type = Action::Type::FLAG;
          in_ >> a.row >> a.col;
          break;
        case 'q':
        case 'Q':
          a.type = Action::Type::QUIT;
          break;
        default:
          fail = true;
      }
      fail = fail || !in_;

      in_.clear();
      in_.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

      if (!fail) {
        break;
      }
      out_ << "Invalid command.\n";
    }
    return a;
  }

  std::istream& in_;
  std::ostream& out_;
};

}  // namespace

std::unique_ptr<TextUi> NewTextUi(std::istream& in, std::ostream& out) {
  return std::make_unique<TextUiImpl>(in, out);
}

}  // namespace ui
}  // namespace mines
