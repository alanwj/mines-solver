#include "mines/ui/ui.h"

#include <memory>

#include <giomm/menumodel.h>
#include <gtkmm/application.h>
#include <gtkmm/builder.h>
#include <sigc++/functors/mem_fun.h>

#include "mines/ui/builder_widget.h"
#include "mines/ui/game_window.h"
#include "mines/ui/resources.h"

namespace mines {
namespace ui {

namespace {

constexpr const char* kApplicationId = "com.alanwj.mines-solver";
constexpr const char* kUiResourcePath = "/com/alanwj/mines-solver/mines-solver.ui";

// The master GTK application.
class MinesApplication : public Gtk::Application {
 public:
  MinesApplication() : Gtk::Application(kApplicationId) {}

 protected:
  // Handler for the startup signal. This handles registering global resources
  // and building any menus and other resources required.
  void on_startup() final {
    Gtk::Application::on_startup();
    ui_register_resource();

    builder_ = Gtk::Builder::create_from_resource(kUiResourcePath);
    auto menu = Glib::RefPtr<Gio::MenuModel>::cast_dynamic(
        builder_->get_object("menu"));

    set_menubar(menu);
  }

  // Handler for the activate signal. Creates the game window.
  void on_activate() final {
    Gtk::Application::on_activate();

    window_.reset(GetBuilderWidget<GameWindow>(builder_, "app-window"));
    add_window(*window_);
    window_->signal_hide().connect(
        sigc::mem_fun(this, &MinesApplication::OnHideWindow));
    window_->present();
  }

 private:
  // Deletes the game window when it received a hide signal.
  void OnHideWindow() { window_.reset(); }

  Glib::RefPtr<Gtk::Builder> builder_;
  std::unique_ptr<GameWindow> window_;
};

}  // namespace

Glib::RefPtr<Gtk::Application> New() {
  return Glib::RefPtr<Gtk::Application>(new MinesApplication());
}

}  // namespace ui
}  // namespace mines
