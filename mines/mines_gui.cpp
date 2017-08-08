#include <ctime>
#include <memory>

#include "mines/game/game.h"
#include "mines/solver/nop.h"
#include "mines/ui/gtk_ui.h"

int main(int argc, char* argv[]) {
  auto game = mines::NewGame(16, 30, 99, std::time(nullptr));
  auto solver = mines::solver::nop::New();
  auto ui = mines::ui::NewGtkUi(argc, argv);

  ui->Play(*game, *solver);

  return 0;
}