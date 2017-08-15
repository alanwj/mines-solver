#ifndef MINES_UI_REMAINING_MINES_COUNTER_H_
#define MINES_UI_REMAINING_MINES_COUNTER_H_

#include <cstddef>

#include <glibmm/refptr.h>
#include <gtkmm/builder.h>

#include "mines/game/game.h"
#include "mines/ui/counter.h"

namespace mines {
namespace ui {

// A counter widget to display the remaining number of mines.
//
// When subscribed to a game via Reset this counter will automatically update.
class RemainingMinesCounter : public Counter, public EventSubscriber {
 public:
  // Contructs a RemainingMinesCounter from the underlying C object and a
  // builder.
  //
  // This constructor supports Gtk::Builder::get_widget_derived.
  RemainingMinesCounter(BaseObjectType* cobj,
                        const Glib::RefPtr<Gtk::Builder>& builder);

  // Resets the counter for a new game.
  //
  // The counter will subscribe to the game for event notifications.
  void Reset(Game& game);

 private:
  // Updates the counter based on the event.
  void NotifyEvent(const Event& event) final;

  // Updates the currently displayed count.
  void UpdateCount();

  // The number of flags that have been placed.
  std::size_t flags_ = 0;

  // The game to which this counter is subscribed.
  Game* game_ = nullptr;
};

}  // namespace ui
}  // namespace mines

#endif  // MINES_UI_REMAINING_MINES_COUNTER_H_
