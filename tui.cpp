#include "tui.h"

#include <cstddef>
#include <ios>
#include <istream>
#include <limits>
#include <ostream>

#include "compat/make_unique.h"

namespace mines {
namespace tui {

namespace {

// Text user interface that uses iostreams.
class Tui : public Ui {
 public:
  Tui(std::istream& in, std::ostream& out) : in_(in), out_(out) {}

  ~Tui() final = default;

  Action BlockUntilAction() final {
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

  void Update(const Knowledge& k) const final {
    const std::size_t rows = k.GetRows();
    const std::size_t cols = k.GetCols();

    for (std::size_t row = 0; row < rows; ++row) {
      for (std::size_t col = 0; col < cols; ++col) {
        switch (k.GetState(row, col)) {
          case Knowledge::CellState::UNCOVERED:
            out_ << k.GetAdjacentMines(row, col);
            break;
          case Knowledge::CellState::COVERED:
            out_ << '-';
            break;
          case Knowledge::CellState::FLAGGED:
            out_ << 'F';
            break;
          case Knowledge::CellState::MINE:
            out_ << '*';
            break;
          case Knowledge::CellState::LOSING_MINE:
            out_ << 'X';
            break;
        }
        out_ << ' ';
      }
      out_ << '\n';
    }
    out_ << '\n';
  }

 private:
  std::istream& in_;
  std::ostream& out_;
};

}  // namespace

std::unique_ptr<Ui> NewUi(std::istream& in, std::ostream& out) {
  return std::make_unique<Tui>(in, out);
}

}  // namespace tui
}  // namespace mines
