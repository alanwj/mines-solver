#ifndef MINES_UI_ELAPSED_TIME_COUNTER_H_
#define MINES_UI_ELAPSED_TIME_COUNTER_H_

#include <glibmm/refptr.h>
#include <gtkmm/builder.h>
#include <sigc++/connection.h>

#include "mines/game/game.h"
#include "mines/ui/counter.h"

namespace mines {
namespace ui {

// A counter widget to display the elapsed time.
//
// When subscribed to a game via Reset this counter will automatically update.
class ElapsedTimeCounter : public Counter, public EventSubscriber {
 public:
  // Gets the ElapsedTimeCounter from the builder.
  static ElapsedTimeCounter* Get(const Glib::RefPtr<Gtk::Builder>& builder);

  // Contructs an ElapsedTimeCounter from the underlying C object and a builder.
  //
  // This constructor supports Gtk::Builder::get_widget_derived.
  ElapsedTimeCounter(BaseObjectType* cobj,
                     const Glib::RefPtr<Gtk::Builder>& builder);

  // Resets the counter for a new game.
  //
  // The counter will subscribe to the game for event notifications.
  void Reset(Game& game);

 private:
  // On the first notification, the counter will begin updating at regular
  // intervals. Subsequent notifications are ignored.
  void NotifyEvent(const Event&) final;

  // Updates the displayed elapsed time.
  bool UpdateElapsedTime();

  // A connection to Glib::signal_timeout.
  sigc::connection timer_;

  // The game to which this counter is subscribed.
  Game* game_ = nullptr;
};

}  // namespace ui
}  // namespace mines

#endif  // MINES_UI_ELAPSED_TIME_COUNTER_H_
