// Main program to play with a text user interface.

#include <ctime>
#include <iostream>
#include <memory>

#include "mines/game/game.h"
#include "mines/solver/solver.h"
#include "mines/ui/text_ui.h"

int main() {
  auto game = mines::NewGame(9, 9, 10, std::time(nullptr));
  auto solver = mines::solver::New(mines::solver::Algorithm::NONE, *game);
  auto ui = mines::ui::NewTextUi(std::cin, std::cout);

  ui->Play(*game, *solver);

  return 0;
}
