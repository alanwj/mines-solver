// Main program to play with a text user interface.

#include <ctime>
#include <iostream>
#include <memory>

#include "manual_player.h"
#include "mines.h"
#include "tui.h"

int main() {
  auto game = mines::NewGame(9, 9, 10, std::time(nullptr));
  auto ui = mines::tui::NewUi(std::cin, std::cout);
  auto player = mines::manual::NewPlayer();
  player->Play(*game, *ui);

  if (game->IsWin()) {
    std::cout << "You win!\n\n";
  } else if (game->IsLoss()) {
    std::cout << "You lose.\n\n";
  }

  return 0;
}
