#include "mines/ui/elapsed_time_counter.h"

#include <cstddef>

#include <glibmm/main.h>
#include <sigc++/functors/mem_fun.h>

namespace mines {
namespace ui {

// The period, in milliseconds, between updates to the counter.
constexpr std::size_t kUpdatePeriodMs = 100;

ElapsedTimeCounter* ElapsedTimeCounter::Get(
    const Glib::RefPtr<Gtk::Builder>& builder) {
  ElapsedTimeCounter* elapsed_time_counter = nullptr;
  builder->get_widget_derived("elapsed-time-counter", elapsed_time_counter);
  return elapsed_time_counter;
}

ElapsedTimeCounter::ElapsedTimeCounter(
    BaseObjectType* cobj, const Glib::RefPtr<Gtk::Builder>& builder)
    : Counter(cobj, builder) {}

void ElapsedTimeCounter::Reset(Game& game) {
  timer_.disconnect();

  game_ = &game;
  game_->Subscribe(this);
  SetValue(0);
}

void ElapsedTimeCounter::NotifyEvent(const Event&) {
  if (!timer_) {
    timer_ = Glib::signal_timeout().connect(
        sigc::mem_fun(this, &ElapsedTimeCounter::UpdateElapsedTime),
        kUpdatePeriodMs);
  }
}

bool ElapsedTimeCounter::UpdateElapsedTime() {
  SetValue(game_->GetElapsedSeconds());
  return !game_->IsGameOver();
}

}  // namespace ui
}  // namespace mines
