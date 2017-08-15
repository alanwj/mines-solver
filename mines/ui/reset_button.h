#ifndef MINES_UI_RESET_BUTTON_H_
#define MINES_UI_RESET_BUTTON_H_

#include <gdkmm/pixbuf.h>
#include <glibmm/refptr.h>
#include <gtkmm/builder.h>
#include <gtkmm/button.h>
#include <gtkmm/image.h>

#include "mines/game/game.h"
#include "mines/ui/mine_field.h"

namespace mines {
namespace ui {

// The reset (and status) button.
//
// This button is used to start a new game, and provide real-time feedback on
// the game state via the image displayed.
class ResetButton : public Gtk::Button, public EventSubscriber {
 public:
  ResetButton(BaseObjectType* cobj, const Glib::RefPtr<Gtk::Builder>&);

  // Connects the button to Minefield mouse events, allowing it to change images
  // based on mouse state.
  void ConnectToMineField(MineField& mine_field);

  // Resets the button for a new game.
  //
  // The button will subscribe to the game for event notifications.
  void Reset(Game& game);

  // Updates the button image based on event notifications.
  void NotifyEvent(const Event& event) final;

 private:
  // Creates the pixbuf for a smiley from the provided resource path.
  static Glib::RefPtr<Gdk::Pixbuf> CreateSmiley(const char* resource_path);

  // Updates the button image based on game state.
  void UpdateImage();

  // Updates the image when a mouse button is pressed on the mine field.
  bool OnMineFieldButtonDown(GdkEventButton*);

  // Updates the image when a mouse button is released on the mine field.
  bool OnMineFieldButtonRelease(GdkEventButton*);

  Gtk::Image smiley_happy_;
  Gtk::Image smiley_cry_;
  Gtk::Image smiley_cool_;
  Gtk::Image smiley_scared_;

  Game* game_ = nullptr;
};

}  // namespace ui
}  // namespace mines

#endif  // MINES_UI_RESET_BUTTON_H_
