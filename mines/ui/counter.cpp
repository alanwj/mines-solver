#include "mines/ui/counter.h"

#include <gdkmm/general.h>
#include <glibmm/ustring.h>

#include "mines/compat/gdk_pixbuf.h"

namespace mines {
namespace ui {

// The width of a counter digit.
constexpr std::size_t kDigitWidth = 23;

// The height of a counter digit.
constexpr std::size_t kDigitHeight = 40;

// The number of counter digits.
constexpr std::size_t kNumDigits = 3;

// The width of the drawn portion of a counter widget.
constexpr std::size_t kWidth = kNumDigits * kDigitWidth;

// The height of the drawn portion of a counter widget.
constexpr std::size_t kHeight = kDigitHeight;

// Format string for constructing the digit resource paths.
constexpr const char* kDigitResourcePathFormat =
    "/com/alanwj/mines-solver/digit-%1.bmp";

// Resource path for the digit that is completely turned off.
constexpr const char* kDigitOffResourcePath =
    "/com/alanwj/mines-solver/digit-off.bmp";

Counter::Counter(BaseObjectType* cobj, const Glib::RefPtr<Gtk::Builder>&)
    : Gtk::DrawingArea(cobj) {
  set_size_request(kWidth, kHeight);
  LoadPixbufs();
}

void Counter::SetValue(std::size_t value) {
  value_ = value;
  queue_draw();
}

bool Counter::on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
  std::size_t value = value_;

  for (std::size_t i = kNumDigits; i--;) {
    std::size_t d = value % 10;
    value /= 10;

    Glib::RefPtr<Gdk::Pixbuf> pixbuf;
    if (d == 0 && value == 0 && i != kNumDigits - 1) {
      pixbuf = digit_off_;
    } else {
      pixbuf = digit_[d];
    }
    Gdk::Cairo::set_source_pixbuf(cr, pixbuf, kDigitWidth * i, 0);
    cr->paint();
  }

  return false;
}

void Counter::LoadPixbufs() {
  for (std::size_t i = 0; i < 10; ++i) {
    digit_[i] = compat::CreatePixbufFromResource(
        Glib::ustring::compose(kDigitResourcePathFormat, i).c_str(),
        kDigitWidth, kDigitHeight);
    digit_off_ = compat::CreatePixbufFromResource(kDigitOffResourcePath,
                                                  kDigitWidth, kDigitHeight);
  }
}

}  // namespace ui
}  // namespace mines
