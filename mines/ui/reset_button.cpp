#include "mines/ui/reset_button.h"

#include <cstddef>

#include <sigc++/functors/mem_fun.h>

#include "mines/compat/gdk_pixbuf.h"

namespace mines {
namespace ui {

// The square size of the smiley pixbufs.
constexpr std::size_t kPixbufSize = 30;

// The resource paths for the smiley graphics.
constexpr const char* kSmileyHappyResourcePath =
    "/com/alanwj/mines-solver/smiley-happy.svg";
constexpr const char* kSmileyCryResourcePath =
    "/com/alanwj/mines-solver/smiley-cry.svg";
constexpr const char* kSmileyCoolResourcePath =
    "/com/alanwj/mines-solver/smiley-cool.svg";
constexpr const char* kSmileyScaredResourcePath =
    "/com/alanwj/mines-solver/smiley-scared.svg";

ResetButton* ResetButton::Get(const Glib::RefPtr<Gtk::Builder>& builder,
                              MineField& mine_field) {
  ResetButton* reset_button = nullptr;
  builder->get_widget_derived("reset-button", reset_button);

  // Connect the reset button to the mine field.
  mine_field.signal_button_press_event().connect(
      sigc::mem_fun(reset_button, &ResetButton::OnMineFieldButtonDown));
  mine_field.signal_button_release_event().connect(
      sigc::mem_fun(reset_button, &ResetButton::OnMineFieldButtonRelease));

  return reset_button;
}

ResetButton::ResetButton(BaseObjectType* cobj,
                         const Glib::RefPtr<Gtk::Builder>&)
    : Gtk::Button(cobj) {
  set_image(smiley_happy_);
}

void ResetButton::Reset(Game& game) {
  game_ = &game;
  game_->Subscribe(this);
  UpdateImage();
}

void ResetButton::on_realize() {
  using compat::CreatePixbufFromResource;

  Gtk::Button::on_realize();

  smiley_happy_.set(CreatePixbufFromResource(kSmileyHappyResourcePath,
                                             kPixbufSize, kPixbufSize));
  smiley_cry_.set(CreatePixbufFromResource(kSmileyCryResourcePath, kPixbufSize,
                                           kPixbufSize));
  smiley_cool_.set(CreatePixbufFromResource(kSmileyCoolResourcePath,
                                            kPixbufSize, kPixbufSize));
  smiley_scared_.set(CreatePixbufFromResource(kSmileyScaredResourcePath,
                                              kPixbufSize, kPixbufSize));
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
