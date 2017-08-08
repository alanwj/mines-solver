#include <ctime>
#include <memory>

#include "mines/game/game.h"
#include "mines/solver/solver.h"
#include "mines/ui/gtk_ui.h"

int main(int argc, char* argv[]) {
  auto game = mines::NewGame(16, 30, 99, std::time(nullptr));
  auto solver = mines::solver::New(mines::solver::Algorithm::LOCAL, *game);

  mines::ui::NewGtkUi(*game, *solver)->run(argc, argv);

  return 0;
}
