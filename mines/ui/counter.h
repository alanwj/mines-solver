#ifndef MINES_UI_COUNTER_H_
#define MINES_UI_COUNTER_H_

#include <cstddef>

#include <cairomm/context.h>
#include <cairomm/refptr.h>
#include <gdkmm/pixbuf.h>
#include <glibmm/refptr.h>
#include <gtkmm/builder.h>
#include <gtkmm/drawingarea.h>

namespace mines {
namespace ui {

// A counter widget in the style of a seven segment display.
class Counter : public Gtk::DrawingArea {
 public:
  // Contructs a Counter from the underlying C object and a builder.
  //
  // This constructor supports Gtk::Builder::get_widget_derived.
  Counter(BaseObjectType* cobj, const Glib::RefPtr<Gtk::Builder>&);

  // Sets the value to display in the widget.
  void SetValue(std::size_t value);

 private:
  // Handler for the realize signal.
  void on_realize() final;

  // Handler for the draw signal.
  bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) final;

  // Loads all of the necessary pixbufs from the global resources.
  void LoadPixbufs();

  // Pixbufs necessary for rendering.
  Glib::RefPtr<Gdk::Pixbuf> digit_[10];
  Glib::RefPtr<Gdk::Pixbuf> digit_off_;

  // The current counter value.
  std::size_t value_ = 0;
};

}  // namespace ui
}  // namespace mines

#endif  // MINES_UI_COUNTER_H_
