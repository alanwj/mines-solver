#ifndef MINES_UI_BUILDER_WIDGET_H_
#define MINES_UI_BUILDER_WIDGET_H_

#include <glibmm/refptr.h>
#include <glibmm/ustring.h>
#include <gtkmm/builder.h>

namespace mines {
namespace ui {

// Gets the named widget from the builder.
//
// This is a convenience function to make calling get_widget_derived easier.
//
// The caller is responsible for correct memory management of the returned
// Widget. See Gtk::Builder::get_widget_derived for details.
template <typename T>
T* GetBuilderWidget(const Glib::RefPtr<Gtk::Builder>& builder,
                    const Glib::ustring& name) {
  T* widget = nullptr;
  builder->get_widget_derived(name, widget);
  return widget;
}

}  // namespace ui
}  // namespace mines

#endif  // MINES_UI_BUILDER_WIDGET_H_
