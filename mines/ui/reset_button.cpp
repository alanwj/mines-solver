#include "mines/ui/reset_button.h"

#include <cstddef>

#include <sigc++/functors/mem_fun.h>

#include "mines/compat/gdk_pixbuf.h"

namespace mines {
namespace ui {

constexpr std::size_t kPixbufSize = 30;
constexpr const char* kSmileyHappyResourcePath =
    "/com/alanwj/mines-solver/smiley-happy.svg";
constexpr const char* kSmileyCryResourcePath =
    "/com/alanwj/mines-solver/smiley-cry.svg";
constexpr const char* kSmileyCoolResourcePath =
    "/com/alanwj/mines-solver/smiley-cool.svg";
constexpr const char* kSmileyScaredResourcePath =
    "/com/alanwj/mines-solver/smiley-scared.svg";

ResetButton::ResetButton(BaseObjectType* cobj,
                         const Glib::RefPtr<Gtk::Builder>&)
    : Gtk::Button(cobj),
      smiley_happy_(CreateSmiley(kSmileyHappyResourcePath)),
      smiley_cry_(CreateSmiley(kSmileyCryResourcePath)),
      smiley_cool_(CreateSmiley(kSmileyCoolResourcePath)),
      smiley_scared_(CreateSmiley(kSmileyScaredResourcePath)) {
  set_image(smiley_happy_);
}

void ResetButton::ConnectToMineField(MineField& mine_field) {
  mine_field.signal_button_press_event().connect(
      sigc::mem_fun(this, &ResetButton::OnMineFieldButtonDown));
  mine_field.signal_button_release_event().connect(
      sigc::mem_fun(this, &ResetButton::OnMineFieldButtonRelease));
}

void ResetButton::Reset(Game& game) {
  game_ = &game;
  game_->Subscribe(this);
  UpdateImage();
}

void ResetButton::NotifyEvent(const Event& event) {
  switch (event.type) {
    case Event::Type::WIN:
    case Event::Type::LOSS:
      UpdateImage();
      break;
    default:
      // We don't care about other event types.
      break;
  }
}

Glib::RefPtr<Gdk::Pixbuf> ResetButton::CreateSmiley(const char* resource_path) {
  return compat::CreatePixbufFromResource(resource_path, kPixbufSize,
                                          kPixbufSize);
}

void ResetButton::UpdateImage() {
  if (game_ == nullptr) {
    set_image(smiley_happy_);
    return;
  }

  switch (game_->GetState()) {
    case Game::State::NEW:
    case Game::State::PLAYING:
      set_image(smiley_happy_);
      break;
    case Game::State::WIN:
      set_image(smiley_cool_);
      break;
    case Game::State::LOSS:
      set_image(smiley_cry_);
      break;
  }
}

bool ResetButton::OnMineFieldButtonDown(GdkEventButton*) {
  if (game_ != nullptr && !game_->IsGameOver()) {
    set_image(smiley_scared_);
  }
  return false;
}

bool ResetButton::OnMineFieldButtonRelease(GdkEventButton*) {
  UpdateImage();
  return false;
}

}  // namespace ui
}  // namespace mines
