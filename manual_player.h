#ifndef MINES_MANUAL_PLAYER_H_
#define MINES_MANUAL_PLAYER_H_

#include <memory>

#include "mines.h"

namespace mines {
namespace manual {

// Returns a new player that is played manually.
std::unique_ptr<Player> NewPlayer();

}  // namespace manual
}  // namespace mines

#endif  // MINES_MANUAL_PLAYER_H_
