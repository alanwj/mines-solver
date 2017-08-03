#include <ctime>
#include <memory>

#include "mines/game/game.h"
#include "mines/solver/single_cell.h"
#include "mines/ui/gtk_ui.h"

int main(int argc, char* argv[]) {
  auto game = mines::NewGame(16, 30, 99, std::time(nullptr));
  auto solver = mines::solver::single_cell::New(*game);
  auto ui = mines::ui::NewGtkUi(argc, argv);

  ui->Play(*game, *solver);

  return 0;
}
