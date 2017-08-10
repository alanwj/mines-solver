#ifndef MINES_UI_GTK_UI_H_
#define MINES_UI_GTK_UI_H_

#include <glibmm/refptr.h>
#include <gtkmm/application.h>

namespace mines {
namespace ui {

// Creates a new GUI.
Glib::RefPtr<Gtk::Application> NewGtkUi();

}  // namespace ui
}  // namespace mines

#endif  // MINES_UI_GTK_UI_H_
