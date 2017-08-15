#ifndef MINES_UI_UI_H_
#define MINES_UI_UI_H_

#include <glibmm/refptr.h>
#include <gtkmm/application.h>

namespace mines {
namespace ui {

// Creates a new GTK aplication that will create and manage the game.
//
// To create and start an application:
//   mines::ui::New()->run(argc, argv);
Glib::RefPtr<Gtk::Application> New();

}  // namespace ui
}  // namespace mines

#endif  // MINES_UI_UI_H_
