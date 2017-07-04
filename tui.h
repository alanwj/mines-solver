#ifndef MINES_TUI_H_
#define MINES_TUI_H_

#include <iosfwd>
#include <memory>

#include "mines.h"

namespace mines {

namespace tui {

// Create a new text user interface.
std::unique_ptr<Ui> NewUi(std::istream& in, std::ostream& out);

}  // namespace tui
}  // namespace mines

#endif  // MINES_TUI_H_
