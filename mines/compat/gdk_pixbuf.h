#ifndef MINES_COMPAT_GDK_PIXBUF_H_
#define MINES_COMPAT_GDK_PIXBUF_H_

#include <gdkmm/pixbuf.h>
#include <glibmm/refptr.h>

namespace mines {
namespace compat {

// Creates a Pixbuf from the specified global resource.
//
// This is a compatibility shim for older versions of glibmm, notably the
// versions supported on Ubuntu 14.04.
//
// When support for these older versions is dropped this can be replaced with a
// call to Gdk::Pixbuf::create_from_resource.
Glib::RefPtr<Gdk::Pixbuf> CreatePixbufFromResource(const char* resource_path,
                                                   int width, int height);

}  // namespace compat
}  // namespace mines

#endif  // MINES_COMPAT_GDK_PIXBUF_H_
