#include "mines/ui/remaining_mines_counter.h"

namespace mines {
namespace ui {

RemainingMinesCounter::RemainingMinesCounter(
    BaseObjectType* cobj, const Glib::RefPtr<Gtk::Builder>& builder)
    : Counter(cobj, builder) {}

void RemainingMinesCounter::Reset(Game& game) {
  flags_ = 0;
  game_ = &game;
  game_->Subscribe(this);
  SetValue(game_->GetMines());
}

void RemainingMinesCounter::NotifyEvent(const Event& event) {
  switch (event.type) {
    case Event::Type::FLAG:
      ++flags_;
      UpdateCount();
      break;
    case Event::Type::UNFLAG:
      if (flags_ > 0) {
        --flags_;
      }
      UpdateCount();
      break;
    default:
      break;
  }
}

void RemainingMinesCounter::UpdateCount() {
  const std::size_t mines = game_->GetMines();
  SetValue(flags_ < mines ? mines - flags_ : 0);
}

}  // namespace ui
}  // namespace mines
