#include "mines/ui/gtk_ui.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <ctime>
#include <memory>
#include <queue>
#include <string>
#include <utility>

#include <gdkmm/general.h>
#include <gdkmm/pixbuf.h>
#include <glibmm/main.h>
#include <glibmm/refptr.h>
#include <glibmm/ustring.h>
#include <gtkmm/applicationwindow.h>
#include <gtkmm/box.h>
#include <gtkmm/builder.h>
#include <gtkmm/button.h>
#include <gtkmm/drawingarea.h>
#include <gtkmm/image.h>
#include <gtkmm/label.h>
#include <sigc++/sigc++.h>

#include "mines/compat/gdk_pixbuf.h"
#include "mines/compat/make_unique.h"
#include "mines/game/game.h"
#include "mines/game/grid.h"
#include "mines/solver/solver.h"
#include "mines/ui/elapsed_time_counter.h"
#include "mines/ui/mine_field.h"
#include "mines/ui/remaining_mines_counter.h"
#include "mines/ui/resources.h"

namespace mines {
namespace ui {

namespace {

// Gets the named widget from the builder.
//
// This is a convenience function to make calling get_widget_derived easier.
template <typename T>
T* GetBuilderWidget(const Glib::RefPtr<Gtk::Builder>& builder,
                    const Glib::ustring& name) {
  T* widget = nullptr;
  builder->get_widget_derived(name, widget);
  return widget;
}

// The reset (and status) button.
//
// This button is used to start a new game, and provide real-time feedback on
// the game state via the image displayed.
class ResetButton : public Gtk::Button, public EventSubscriber {
 public:
  ResetButton(BaseObjectType* cobj, const Glib::RefPtr<Gtk::Builder>&)
      : Gtk::Button(cobj),
        smiley_happy_(CreateSmiley(kSmileyHappyResourcePath)),
        smiley_cry_(CreateSmiley(kSmileyCryResourcePath)),
        smiley_cool_(CreateSmiley(kSmileyCoolResourcePath)),
        smiley_scared_(CreateSmiley(kSmileyScaredResourcePath)) {
    set_image(smiley_happy_);
  }

  // Connects the button to Minefield mouse events, allowing it to change images
  // based on mouse state.
  void ConnectToMineField(MineField& mine_field) {
    mine_field.signal_button_press_event().connect(
        sigc::mem_fun(this, &ResetButton::OnMineFieldButtonDown));
    mine_field.signal_button_release_event().connect(
        sigc::mem_fun(this, &ResetButton::OnMineFieldButtonRelease));
  }

  // Resets the button for a new game.
  //
  // The button will subscribe to the game for event notifications.
  void Reset(Game& game) {
    game_ = &game;
    game_->Subscribe(this);
    UpdateImage();
  }

  // Updates the button image based on event notifications.
  void NotifyEvent(const Event& event) final {
    switch (event.type) {
      case Event::Type::WIN:
      case Event::Type::LOSS:
        UpdateImage();
        break;
      default:
        // We don't care about other event types.
        break;
    }
  }

 private:
  static constexpr std::size_t kPixbufSize = 30;
  static constexpr const char* kSmileyHappyResourcePath =
      "/com/alanwj/mines-solver/smiley-happy.svg";
  static constexpr const char* kSmileyCryResourcePath =
      "/com/alanwj/mines-solver/smiley-cry.svg";
  static constexpr const char* kSmileyCoolResourcePath =
      "/com/alanwj/mines-solver/smiley-cool.svg";
  static constexpr const char* kSmileyScaredResourcePath =
      "/com/alanwj/mines-solver/smiley-scared.svg";

  // Creates the pixbuf for a smiley from the provided resource path.
  static Glib::RefPtr<Gdk::Pixbuf> CreateSmiley(const char* resource_path) {
    return compat::CreatePixbufFromResource(resource_path, kPixbufSize,
                                            kPixbufSize);
  }

  // Updates the button image based on game state.
  void UpdateImage() {
    if (game_ == nullptr) {
      set_image(smiley_happy_);
      return;
    }

    switch (game_->GetState()) {
      case Game::State::NEW:
      case Game::State::PLAYING:
        set_image(smiley_happy_);
        break;
      case Game::State::WIN:
        set_image(smiley_cool_);
        break;
      case Game::State::LOSS:
        set_image(smiley_cry_);
        break;
    }
  }

  // Updates the image when a mouse button is pressed on the mine field.
  bool OnMineFieldButtonDown(GdkEventButton*) {
    if (game_ != nullptr && !game_->IsGameOver()) {
      set_image(smiley_scared_);
    }
    return false;
  }

  // Updates the image when a mouse button is released on the mine field.
  bool OnMineFieldButtonRelease(GdkEventButton*) {
    UpdateImage();
    return false;
  }

  Gtk::Image smiley_happy_;
  Gtk::Image smiley_cry_;
  Gtk::Image smiley_cool_;
  Gtk::Image smiley_scared_;

  Game* game_ = nullptr;
};

// The main window for the game.
class MinesWindow : public Gtk::ApplicationWindow {
 public:
  MinesWindow(BaseObjectType* cobj, const Glib::RefPtr<Gtk::Builder>& builder)
      : Gtk::ApplicationWindow(cobj),
        field_(GetBuilderWidget<MineField>(builder, "mine-field")),
        reset_button_(GetBuilderWidget<ResetButton>(builder, "reset-button")),
        remaining_mines_counter_(GetBuilderWidget<RemainingMinesCounter>(
            builder, "remaining-mines-counter")),
        elapsed_time_counter_(GetBuilderWidget<ElapsedTimeCounter>(
            builder, "elapsed-time-counter")),
        solver_algorithm_(solver::Algorithm::NONE) {
    add_action("new", sigc::mem_fun(this, &MinesWindow::NewGame));
    solver_action_ = add_action_radio_string(
        "solver", sigc::mem_fun(this, &MinesWindow::NewSolverAlgorithm),
        "none");

    field_->signal_action().connect(
        sigc::mem_fun(this, &MinesWindow::HandleAction));

    reset_button_->signal_clicked().connect(
        sigc::mem_fun(this, &MinesWindow::NewGame));
    reset_button_->ConnectToMineField(*field_);

    NewGame();
  }

 private:
  // Starts a new game.
  void NewGame() {
    game_ = mines::NewGame(16, 30, 99, std::time(nullptr));
    solver_ = solver::New(solver_algorithm_, *game_);
    field_->Reset(*game_);
    reset_button_->Reset(*game_);
    remaining_mines_counter_->Reset(*game_);
    elapsed_time_counter_->Reset(*game_);
  }

  // Changes the solver algorithm and starts a new game.
  void NewSolverAlgorithm(const Glib::ustring& target) {
    solver_action_->change_state(target);

    if (target == "none") {
      solver_algorithm_ = solver::Algorithm::NONE;
    } else if (target == "local") {
      solver_algorithm_ = solver::Algorithm::LOCAL;
    } else {
      solver_algorithm_ = solver::Algorithm::NONE;
    }

    NewGame();
  }

  // Handles an action on the mine field.
  //
  // Executes the action, updates the mine field, and runs the solver.
  void HandleAction(Action action) {
    game_->Execute(action);

    // Execute all actions recommended by the solver.
    std::vector<Action> actions;
    do {
      actions = solver_->Analyze();
      game_->Execute(actions);
    } while (!actions.empty());
  }

  // The mine field.
  MineField* field_;

  // The reset button.
  ResetButton* reset_button_;

  // The remaining mines counter.
  RemainingMinesCounter* remaining_mines_counter_;

  // The elapsed time counter.
  ElapsedTimeCounter* elapsed_time_counter_;

  // Menu action for selection the solver algorithm.
  Glib::RefPtr<Gio::SimpleAction> solver_action_;

  // The algorithm used in current and new games.
  solver::Algorithm solver_algorithm_;

  // The current game.
  std::unique_ptr<Game> game_;

  // The current solver.
  std::unique_ptr<solver::Solver> solver_;
};

// The master GTK application.
class MinesApplication : public Gtk::Application {
 public:
  static constexpr const char* kApplicationId = "com.alanwj.mines-solver";
  static constexpr const char* kUiResourcePath =
      "/com/alanwj/mines-solver/mines-solver.ui";

  MinesApplication() : Gtk::Application(kApplicationId) {}

 protected:
  void on_startup() final {
    Gtk::Application::on_startup();
    ui_register_resource();

    builder_ = Gtk::Builder::create_from_resource(kUiResourcePath);
    auto menu = Glib::RefPtr<Gio::MenuModel>::cast_dynamic(
        builder_->get_object("menu"));

    set_menubar(menu);
  }

  void on_activate() final {
    Gtk::Application::on_activate();

    window_.reset(GetBuilderWidget<MinesWindow>(builder_, "app-window"));
    add_window(*window_);
    window_->signal_hide().connect(
        sigc::mem_fun(this, &MinesApplication::OnHideWindow));
    window_->present();
  }

 private:
  void OnHideWindow() { window_.reset(); }

  Glib::RefPtr<Gtk::Builder> builder_;
  std::unique_ptr<MinesWindow> window_;
};

}  // namespace

Glib::RefPtr<Gtk::Application> NewGtkUi() {
  return Glib::RefPtr<Gtk::Application>(new MinesApplication());
}

}  // namespace ui
}  // namespace mines
