// Main program to play with a text user interface.
//
// Uses a solver that automates single cell analysis.

#include <ctime>
#include <iostream>
#include <memory>

#include "mines/game/game.h"
#include "mines/solver/single_cell.h"
#include "mines/ui/text_ui.h"

int main() {
  auto game = mines::NewGame(9, 9, 10, std::time(nullptr));
  auto solver = mines::solver::single_cell::New(*game);
  auto ui = mines::ui::NewTextUi(std::cin, std::cout);

  ui->Play(*game, *solver);

  return 0;
}
