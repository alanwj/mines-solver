#include "mines/ui/ui.h"

#include <algorithm>
#include <memory>

#include <giomm/menumodel.h>
#include <giomm/simpleaction.h>
#include <glibmm/ustring.h>
#include <gtkmm/application.h>
#include <gtkmm/builder.h>
#include <gtkmm/window.h>
#include <sigc++/bind.h>
#include <sigc++/functors/mem_fun.h>
#include <sigc++/functors/ptr_fun.h>

#include "mines/ui/game_window.h"
#include "mines/ui/resources.h"

namespace mines {
namespace ui {

namespace {

constexpr const char* kApplicationId = "com.alanwj.mines-solver";
constexpr const char* kMenuResourcePath = "/com/alanwj/mines-solver/menu.ui";
constexpr const char* kGameWindowResourcePath =
    "/com/alanwj/mines-solver/game_window.ui";

// The master GTK application.
class MinesApplication : public Gtk::Application {
 public:
  MinesApplication() : Gtk::Application(kApplicationId) {}

 private:
  // Handler for the startup signal. This handles registering global resources
  // and building any menus and other resources required.
  void on_startup() final {
    Gtk::Application::on_startup();
    ui_register_resource();

    menu_builder_ = Gtk::Builder::create_from_resource(kMenuResourcePath);
    auto menu = Glib::RefPtr<Gio::MenuModel>::cast_dynamic(
        menu_builder_->get_object("menu"));

    difficulty_action_ = add_action_radio_string(
        "difficulty", sigc::mem_fun(this, &MinesApplication::NewDifficulty),
        "expert");

    set_menubar(menu);
  }

  // Handler for the activate signal. Creates the game window.
  void on_activate() final {
    Gtk::Application::on_activate();
    NewGameWindow();
  }

  // Creates a new game window, destroying any existing game windows.
  void NewGameWindow() {
    auto builder = Gtk::Builder::create_from_resource(kGameWindowResourcePath);
    GameWindow* window = GameWindow::Get(builder, difficulty_);
    add_window(*window);
    window->signal_hide().connect(sigc::bind<GameWindow*>(
        sigc::ptr_fun(&MinesApplication::OnHideWindow), window));
    window->present();

    // Destroy all old windows.
    for (Gtk::Window* old_window : get_windows()) {
      if (old_window != window) {
        old_window->hide();
      }
    }

    game_window_builder_ = builder;
  }

  // Deletes a game window when it received a hide signal.
  static void OnHideWindow(GameWindow* window) { delete window; }

  // Updates the difficulty level, and creates a new window as appropriate.
  void NewDifficulty(const Glib::ustring& target) {
    Glib::ustring state;
    difficulty_action_->get_state(state);

    // Do nothing if the difficulty isn't changing.
    if (target == state) {
      return;
    }

    if (target == "beginner") {
      difficulty_ = GameWindow::kBeginnerDifficulty;
    } else if (target == "intermediate") {
      difficulty_ = GameWindow::kIntermediateDifficulty;
    } else {
      difficulty_ = GameWindow::kExpertDifficulty;
    }

    difficulty_action_->change_state(target);
    NewGameWindow();
  }

  Glib::RefPtr<Gtk::Builder> menu_builder_;
  Glib::RefPtr<Gtk::Builder> game_window_builder_;
  Glib::RefPtr<Gio::SimpleAction> difficulty_action_;

  GameWindow::Difficulty difficulty_ = GameWindow::kExpertDifficulty;
};

}  // namespace

Glib::RefPtr<Gtk::Application> New() {
  return Glib::RefPtr<Gtk::Application>(new MinesApplication());
}

}  // namespace ui
}  // namespace mines
