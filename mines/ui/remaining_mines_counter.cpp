#include "mines/ui/remaining_mines_counter.h"

namespace mines {
namespace ui {

RemainingMinesCounter* RemainingMinesCounter::Get(
    const Glib::RefPtr<Gtk::Builder>& builder) {
  RemainingMinesCounter* remaining_mines_counter = nullptr;
  builder->get_widget_derived("remaining-mines-counter",
                              remaining_mines_counter);
  return remaining_mines_counter;
}

RemainingMinesCounter::RemainingMinesCounter(
    BaseObjectType* cobj, const Glib::RefPtr<Gtk::Builder>& builder)
    : Counter(cobj, builder) {}

void RemainingMinesCounter::NotifyEventSubscription(Game* game) {
  game_ = game;
  flags_ = 0;
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
