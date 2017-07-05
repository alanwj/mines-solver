#ifndef MINES_ASSISTED_PLAYER_H_
#define MINES_ASSISTED_PLAYER_H_

#include <memory>

#include "mines.h"

namespace mines {
namespace assisted {

// Returns a new player that assists in simple tasks.
//
// When a cell is uncovered, this player will automatically place flags
// and chord cells that can be determined via single cell analysis.
std::unique_ptr<Player> NewPlayer();

}  // namespace assisted
}  // namespace mines

#endif  // MINES_ASSISTED_PLAYER_H_
