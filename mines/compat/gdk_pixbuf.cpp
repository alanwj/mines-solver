#include "mines/compat/gdk_pixbuf.h"

#include <glibmm/error.h>
#include <gtk/gtk.h>

namespace mines {
namespace compat {

Glib::RefPtr<Gdk::Pixbuf> CreatePixbufFromResource(const char* resource_path,
                                                   int width, int height) {
  GError* error = nullptr;
  GdkPixbuf* pixbuf = gdk_pixbuf_new_from_resource_at_scale(
      resource_path, width, height, false, &error);
  if (error != nullptr) {
    throw Glib::Error(error);
  }
  return Glib::wrap(pixbuf);
}

}  // namespace compat
}  // namespace mines
