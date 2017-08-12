#include "mines/ui/gtk_ui.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <ctime>
#include <memory>
#include <queue>
#include <string>
#include <utility>

#include <gdkmm/general.h>
#include <gdkmm/pixbuf.h>
#include <glibmm/main.h>
#include <glibmm/refptr.h>
#include <glibmm/ustring.h>
#include <gtkmm/applicationwindow.h>
#include <gtkmm/box.h>
#include <gtkmm/builder.h>
#include <gtkmm/drawingarea.h>
#include <sigc++/sigc++.h>

#include "mines/compat/gdk_pixbuf.h"
#include "mines/compat/make_unique.h"
#include "mines/game/game.h"
#include "mines/game/grid.h"
#include "mines/solver/solver.h"
#include "mines/ui/resources.h"

namespace mines {
namespace ui {

namespace {

// Gets the named widget from the builder.
//
// This is a convenience function to make calling get_widget_derived easier.
template <typename T>
T* GetBuilderWidget(const Glib::RefPtr<Gtk::Builder>& builder,
                    const Glib::ustring& name) {
  T* widget = nullptr;
  builder->get_widget_derived(name, widget);
  return widget;
}

// Encapsulates a simple RGB color.
struct Color {
  double r;
  double g;
  double b;
};

// The dimensions of the actual area upon which the mine field will be drawn.
// This is a subset of the actual allocated area.
struct DrawingDimensions {
  std::size_t x;
  std::size_t y;
  std::size_t width;
  std::size_t height;
  std::size_t cell_size;
};

// The collection of pixbufs used in the mine field.
struct Pixbufs {
  Glib::RefPtr<Gdk::Pixbuf> mine;
  Glib::RefPtr<Gdk::Pixbuf> flag;
};

// A representation of the GUI's knowledge about a cell.
struct Cell {
  CellState state = CellState::COVERED;
  bool pressed = false;
  std::size_t adjacent_mines = 0;
};

// A flyweight class to encapsulate the process of drawing a cell.
//
// This exists primarily to separate this drawing logic from the MineField class
// to prevent it from growing too complex.
class CellDrawFlyweight {
 public:
  CellDrawFlyweight(const Cairo::RefPtr<Cairo::Context>& cr,
                    const DrawingDimensions& dim, const Pixbufs& pixbufs,
                    const Cell& cell, std::size_t row, std::size_t col)
      : cr_(cr),
        dim_(dim),
        pixbufs_(pixbufs),
        cell_(cell),
        row_(row),
        col_(col) {}

  CellDrawFlyweight(const CellDrawFlyweight&) = delete;
  CellDrawFlyweight& operator=(const CellDrawFlyweight&) = delete;
  CellDrawFlyweight(CellDrawFlyweight&&) = delete;
  CellDrawFlyweight& operator=(CellDrawFlyweight&&) = delete;

  void Draw() const {
    switch (cell_.state) {
      case CellState::UNCOVERED:
        DrawUncovered();
        break;
      case CellState::COVERED:
        if (cell_.pressed) {
          DrawPressed();
        } else {
          DrawCovered();
        }
        break;
      case CellState::FLAGGED:
        DrawFlagged();
        break;
      case CellState::MINE:
        DrawMine();
        break;
      case CellState::LOSING_MINE:
        DrawLosingMine();
        break;
      case CellState::BAD_FLAG:
        DrawBadFlag();
        break;
    };
  }

 private:
  // The color to use for numbers 1 through 8.
  static constexpr std::array<Color, 8> kNumberColor{{
      {0.0, 0.0, 1.0},  // 1
      {0.0, 0.5, 0.0},  // 2
      {1.0, 0.0, 0.0},  // 3
      {0.0, 0.0, 0.5},  // 4
      {0.5, 0.0, 0.0},  // 5
      {0.0, 0.5, 0.5},  // 6
      {0.5, 0.0, 0.5},  // 7
      {0.0, 0.0, 0.0},  // 8
  }};

  // Colors used in various parts of the cell.
  static constexpr Color kCellColor{0.76, 0.76, 0.76};
  static constexpr Color kLosingMineCellColor{1.0, 0.0, 0.0};
  static constexpr Color kCellBorderColor{0.5, 0.5, 0.5};
  static constexpr Color kLightBevelColor{1.0, 1.0, 1.0};
  static constexpr Color kDarkBevelColor{0.5, 0.5, 0.5};

  // Sets the context source color.
  void SetColor(const Color& color) const {
    cr_->set_source_rgba(color.r, color.g, color.b, 1.0);
  }

  // Fills the cell with the specified color.
  void FillCell(const Color& color) const {
    SetColor(color);
    cr_->rectangle(0.0, 0.0, dim_.cell_size, dim_.cell_size);
    cr_->fill();
  }

  // Draws a sharp single pixel horizontal line of the specified width.
  void DrawHLine(std::size_t x, std::size_t y, std::size_t width) const {
    cr_->save();
    cr_->translate(0.5, 0.5);
    cr_->move_to(x, y);
    cr_->line_to(x + width - 1, y);
    cr_->restore();
  }

  // Draws a sharp single pixel vertical line of the specified height.
  void DrawVLine(std::size_t x, std::size_t y, std::size_t height) const {
    cr_->save();
    cr_->translate(0.5, 0.5);
    cr_->move_to(x, y);
    cr_->line_to(x, y + height - 1);
    cr_->restore();
  }

  // Draws a character in a cell.
  void DrawChar(char c) const {
    const char str[2] = {c, '\0'};

    cr_->select_font_face("cairo:monospace", Cairo::FONT_SLANT_NORMAL,
                          Cairo::FONT_WEIGHT_BOLD);
    // Empirically, 9/10 of the cell looks pretty good.
    cr_->set_font_size(.9 * dim_.cell_size);
    Cairo::TextExtents te;
    cr_->get_text_extents(str, te);

    cr_->move_to(dim_.cell_size / 2 - te.width / 2 - te.x_bearing,
                 dim_.cell_size / 2 - te.height / 2 - te.y_bearing);
    cr_->show_text(str);
  }

  // Draws an empty cell.
  void DrawEmpty(const Color& color) const {
    FillCell(color);
    SetColor(kCellBorderColor);
    if (row_ != 0) {
      DrawHLine(0, 0, dim_.cell_size);
    }
    if (col_ != 0) {
      DrawVLine(0, 0, dim_.cell_size);
    }
    cr_->stroke();
  }

  // Draws a cell in the UNCOVERED state with the number of adjacent mines.
  void DrawUncovered() const {
    DrawEmpty(kCellColor);
    if (cell_.adjacent_mines > 0 && cell_.adjacent_mines <= 8) {
      SetColor(kNumberColor[cell_.adjacent_mines - 1]);
      DrawChar('0' + cell_.adjacent_mines);
    }
  }

  // Draws a cell that is visually pressed by a mouse action.
  void DrawPressed() const {
    FillCell(kCellColor);
    SetColor(kDarkBevelColor);
    DrawHLine(0, 0, dim_.cell_size);
    DrawVLine(0, 0, dim_.cell_size);
    cr_->stroke();
  }

  // Draws a cell in the COVERED state.
  void DrawCovered() const {
    FillCell(kCellColor);

    // Light bevel on top.
    SetColor(kLightBevelColor);
    DrawHLine(0, 0, dim_.cell_size - 1);
    DrawHLine(0, 1, dim_.cell_size - 2);

    // Light bevel on left.
    DrawVLine(0, 0, dim_.cell_size - 1);
    DrawVLine(1, 0, dim_.cell_size - 2);
    cr_->stroke();

    // Dark bevel on bottom.
    SetColor(kDarkBevelColor);
    DrawHLine(1, dim_.cell_size - 1, dim_.cell_size - 1);
    DrawHLine(2, dim_.cell_size - 2, dim_.cell_size - 2);

    // Dark bevel on right.
    DrawVLine(dim_.cell_size - 1, 1, dim_.cell_size - 1);
    DrawVLine(dim_.cell_size - 2, 2, dim_.cell_size - 2);
    cr_->stroke();
  }

  // Draws the provided pixbuf in the center of the cell.
  void DrawPixbuf(const Glib::RefPtr<Gdk::Pixbuf>& pixbuf) const {
    cr_->save();
    Gdk::Cairo::set_source_pixbuf(
        cr_, pixbuf, dim_.cell_size / 2 - pixbuf->get_width() / 2,
        dim_.cell_size / 2 - pixbuf->get_height() / 2);
    cr_->paint();
    cr_->restore();
  }

  // Draws a cell in the FLAGGED state.
  void DrawFlagged() const {
    DrawCovered();
    DrawPixbuf(pixbufs_.flag);
  }

  // Draws a cell in the MINE state.
  void DrawMine() const {
    DrawEmpty(kCellColor);
    DrawPixbuf(pixbufs_.mine);
  }

  // Draws a cell in the LOSING_MINE state.
  void DrawLosingMine() const {
    DrawEmpty(kLosingMineCellColor);
    DrawPixbuf(pixbufs_.mine);
  }

  // Draws a cell in the BAD_FLAG state.
  void DrawBadFlag() const {
    DrawFlagged();
    cr_->save();
    SetColor({1.0, 0.0, 0.0});
    cr_->scale(dim_.cell_size, dim_.cell_size);
    cr_->set_line_width(.15);
    cr_->set_line_cap(Cairo::LINE_CAP_ROUND);

    const double margin = .15;
    cr_->move_to(margin, margin);
    cr_->line_to(1.0 - margin, 1.0 - margin);
    cr_->move_to(margin, 1.0 - margin);
    cr_->line_to(1.0 - margin, margin);
    cr_->stroke();

    cr_->restore();
  }

  const Cairo::RefPtr<Cairo::Context>& cr_;
  const DrawingDimensions& dim_;
  const Pixbufs& pixbufs_;
  const Cell& cell_;
  const std::size_t row_;
  const std::size_t col_;
};

constexpr std::array<Color, 8> CellDrawFlyweight::kNumberColor;
constexpr Color CellDrawFlyweight::kCellColor;
constexpr Color CellDrawFlyweight::kLosingMineCellColor;
constexpr Color CellDrawFlyweight::kCellBorderColor;
constexpr Color CellDrawFlyweight::kLightBevelColor;
constexpr Color CellDrawFlyweight::kDarkBevelColor;

// A mine field widget.
class MineField : public Gtk::DrawingArea, public EventSubscriber {
 public:
  // The size of the frame around the mine field.
  static constexpr std::size_t kFrameSize = 1;

  // The default (and minimum) cell size.
  static constexpr std::size_t kCellSize = 20;

  // Colors used in widget construction.
  static constexpr Color kFrameColor{0.5, 0.5, 0.5};

  // The time (in milliseconds) between processing each event.
  // A small visual delay between events gives a natural feel when areas are
  // uncovered.
  static constexpr unsigned int kEventTimeoutMs = 1;

  static constexpr const char* kMineResourcePath =
      "/com/alanwj/mines-solver/mine.svg";

  static constexpr const char* kFlagResourcePath =
      "/com/alanwj/mines-solver/flag.svg";

  MineField(BaseObjectType* cobj, const Glib::RefPtr<Gtk::Builder>&)
      : Gtk::DrawingArea(cobj) {}

  // Resets the internal state for a new game.
  void Reset(std::size_t rows, std::size_t cols) {
    rows_ = rows;
    cols_ = cols;
    grid_.Reset(rows_, cols_);

    const int min_width = kCellSize * cols + 2 * kFrameSize;
    const int min_height = kCellSize * rows + 2 * kFrameSize;

    set_size_request(min_width, min_height);

    UpdateDrawingDimensions(std::max(min_width, get_allocated_width()),
                            std::max(min_height, get_allocated_height()));
    UpdatePixbufs();

    mouse_state_ = MouseState();
    clicked_cell_ = CellRef::None();

    event_queue_ = std::queue<Event>();

    timeout_connection_.disconnect();

    queue_draw();

    // Note: Any connections to signal_action will remain valid.
  }

  // Updates the visual state based on the event.
  //
  // If the event is not adjacent to the last clicked cell, the event may be
  // queued for later handling.
  void NotifyEvent(const Event& event) final {
    // Don't wait for events adjacent to the clicked cell.
    if (clicked_cell_.cell != nullptr &&
        IsAdjacentToClickedCell(event.row, event.col)) {
      HandleEvent(event);
    } else {
      event_queue_.push(event);

      if (!timeout_connection_) {
        // Handle the rest of the events on a timeout.
        timeout_connection_ = Glib::signal_timeout().connect(
            sigc::mem_fun(this, &MineField::HandleEventFromQueue),
            kEventTimeoutMs);
      }
    }
  }

  // The signal sent when an Action is peformed on the mine field.
  //
  // The action type will be one of: UNCOVER, CHORD, or FLAG.
  sigc::signal<void, Action>& signal_action() { return signal_action_; }

 protected:
  // Recomputes values necessary to resize the mine field.
  bool on_configure_event(GdkEventConfigure* event) final {
    UpdateDrawingDimensions(event->width, event->height);
    UpdatePixbufs();
    return Gtk::DrawingArea::on_configure_event(event);
  }

  // Draws the widget.
  bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) final {
    // Defaults for drawing lines.
    cr->set_line_width(1);
    cr->set_line_cap(Cairo::LINE_CAP_SQUARE);

    cr->translate(dim_.x, dim_.y);

    DrawFrame(cr);

    for (std::size_t row = 0; row < rows_; ++row) {
      for (std::size_t col = 0; col < cols_; ++col) {
        cr->save();
        cr->translate(col * dim_.cell_size + kFrameSize,
                      row * dim_.cell_size + kFrameSize);

        CellDrawFlyweight flyweight(cr, dim_, pixbufs_, grid_(row, col), row,
                                    col);
        flyweight.Draw();

        cr->restore();
      }
    }

    return false;
  }

  // Handles mouse button presses.
  bool on_button_press_event(GdkEventButton* event) final {
    if (event->type != GDK_BUTTON_PRESS) {
      return false;
    }

    if (event->button < 1 || event->button > MouseState::kMaxButton) {
      return false;
    }

    mouse_state_.btn[event->button - 1] = true;
    ++mouse_state_.count;

    // On the first button in a potential multi-button click, get the clicked
    // cell.
    if (mouse_state_.count == 1) {
      clicked_cell_ = GetCellRefFromPoint(event->x, event->y);
    }

    // If the click wasn't on a cell, there is no more to do.
    if (clicked_cell_.cell == nullptr) {
      return false;
    }

    if (mouse_state_.btn[0] || mouse_state_.btn[2]) {
      clicked_cell_.cell->pressed = true;
    }

    if (mouse_state_.btn[1] || (mouse_state_.btn[0] && mouse_state_.btn[2])) {
      clicked_cell_.cell->pressed = false;
      grid_.ForEachAdjacent(clicked_cell_.row, clicked_cell_.col,
                            [this](std::size_t row, std::size_t col) {
                              grid_(row, col).pressed = true;
                              return false;
                            });
    }

    queue_draw();

    return false;
  }

  // Handles mouse button releases.
  bool on_button_release_event(GdkEventButton* event) final {
    // If a new game is created while mouse buttons are held the count can
    // become inconsistent.
    if (mouse_state_.count == 0) {
      return false;
    }

    if (event->button < 1 || event->button > MouseState::kMaxButton) {
      return false;
    }

    --mouse_state_.count;

    // If buttons are still pressed continue waiting.
    if (mouse_state_.count != 0) {
      return false;
    }

    // If the release was in the same cell that was originally clicked, then
    // send an appropriate signal.
    CellRef ref = GetCellRefFromPoint(event->x, event->y);
    if (ref.cell != nullptr && ref.cell == clicked_cell_.cell) {
      if (mouse_state_.btn[1] || (mouse_state_.btn[0] && mouse_state_.btn[2])) {
        signal_action_.emit({Action::Type::CHORD, ref.row, ref.col});
      } else if (mouse_state_.btn[0]) {
        signal_action_.emit({Action::Type::UNCOVER, ref.row, ref.col});
      } else if (mouse_state_.btn[2]) {
        signal_action_.emit({Action::Type::FLAG, ref.row, ref.col});
      }
    }

    // Reset the UI to visually unpress buttons.
    if (clicked_cell_.cell != nullptr) {
      clicked_cell_.cell->pressed = false;
      grid_.ForEachAdjacent(clicked_cell_.row, clicked_cell_.col,
                            [this](std::size_t row, std::size_t col) {
                              grid_(row, col).pressed = false;
                              return false;
                            });
      queue_draw();
    }

    // Reset the mouse state.
    mouse_state_ = MouseState();

    return false;
  }

 private:
  // Reference to a cell and the associated row and column.
  struct CellRef {
    Cell* cell;
    std::size_t row;
    std::size_t col;

    static CellRef None() { return {nullptr, 0, 0}; }
  };

  // The state of the mouse.
  struct MouseState {
    static constexpr std::size_t kMaxButton = 3;

    // Indicates which buttons have been pressed during this (possibly
    // multi-button) mouse event.
    bool btn[kMaxButton] = {false};
    std::size_t count = 0;
  };

  // Computes the cell from mouse event coordinates.
  CellRef GetCellRefFromPoint(int x, int y) {
    if (x < 0 || y < 0) {
      return CellRef::None();
    }

    const std::size_t ux = x;
    const std::size_t uy = y;
    const std::size_t min_x = dim_.x + kFrameSize;
    const std::size_t max_x = min_x + dim_.width - 2 * kFrameSize - 1;
    const std::size_t min_y = dim_.y + kFrameSize;
    const std::size_t max_y = min_y + dim_.height - 2 * kFrameSize - 1;
    if (ux < min_x || ux > max_x || uy < min_y || uy > max_y) {
      return CellRef::None();
    }

    const std::size_t row = (uy - min_y) / dim_.cell_size;
    const std::size_t col = (ux - min_x) / dim_.cell_size;
    return CellRef{&grid_(row, col), row, col};
  }

  // Recomputes the drawing dimensions.
  void UpdateDrawingDimensions(int width, int height) {
    dim_.cell_size = std::min((width - 2 * kFrameSize) / cols_,
                              (height - 2 * kFrameSize) / rows_);
    dim_.width = dim_.cell_size * cols_ + 2 * kFrameSize;
    dim_.height = dim_.cell_size * rows_ + 2 * kFrameSize;

    // Center the part of the drawing area we are actually using within the
    // total available space.
    dim_.x = width / 2 - dim_.width / 2;
    dim_.y = height / 2 - dim_.height / 2;
  }

  // Loads a pixbuf from a resource path at current cell dimensions.
  Glib::RefPtr<Gdk::Pixbuf> LoadPixbuf(const char* resource_path) const {
    return compat::CreatePixbufFromResource(resource_path, 0.7 * dim_.cell_size,
                                            0.7 * dim_.cell_size);
  }

  // Reload all pixbufs at current cell dimensions.
  void UpdatePixbufs() {
    pixbufs_.mine = LoadPixbuf(kMineResourcePath);
    pixbufs_.flag = LoadPixbuf(kFlagResourcePath);
  }

  // Sets the context color.
  void SetColor(const Cairo::RefPtr<Cairo::Context>& cr,
                const Color& color) const {
    cr->set_source_rgba(color.r, color.g, color.b, 1.0);
  }

  // Draws the frame surrounding the mine field.
  void DrawFrame(const Cairo::RefPtr<Cairo::Context>& cr) {
    SetColor(cr, kFrameColor);
    cr->rectangle(0.0, 0.0, dim_.width, kFrameSize);
    cr->rectangle(0.0, 0.0, kFrameSize, dim_.height);
    cr->rectangle(0.0, dim_.height - kFrameSize, dim_.width, kFrameSize);
    cr->rectangle(dim_.width - kFrameSize, 0.0, kFrameSize, dim_.height);
    cr->fill();
  }

  // Updates the visual state based on the top several events in the event
  // queue.
  //
  // Returns true if there are more events to handle.
  bool HandleEventFromQueue() {
    // Handle 3 events at a time because 1ms is too slow for the timer but
    // there is no way to set a submillisecond timer.
    for (std::size_t i = 0; i < 3 && !event_queue_.empty(); ++i) {
      HandleEvent(event_queue_.front());
      event_queue_.pop();
    }
    return !event_queue_.empty();
  }

  // Updates the visual state based on the event.
  void HandleEvent(const Event& event) {
    Cell& cell = grid_(event.row, event.col);
    switch (event.type) {
      case Event::Type::UNCOVER:
        cell.state = CellState::UNCOVERED;
        cell.adjacent_mines = event.adjacent_mines;
        break;
      case Event::Type::FLAG:
        cell.state = CellState::FLAGGED;
        break;
      case Event::Type::UNFLAG:
        cell.state = CellState::COVERED;
        break;
      case Event::Type::WIN:
        break;
      case Event::Type::LOSS:
        cell.state = CellState::LOSING_MINE;
        break;
      case Event::Type::IDENTIFY_MINE:
        cell.state = CellState::MINE;
        break;
      case Event::Type::IDENTIFY_BAD_FLAG:
        cell.state = CellState::BAD_FLAG;
        break;
    }
    queue_draw();
  }

  // Returns true if the specified row and column are in or adjacent to the
  // last clicked cell.
  bool IsAdjacentToClickedCell(std::size_t row, std::size_t col) const {
    const std::size_t delta_row =
        std::max(clicked_cell_.row, row) - std::min(clicked_cell_.row, row);
    const std::size_t delta_col =
        std::max(clicked_cell_.col, col) - std::min(clicked_cell_.col, col);
    return delta_row <= 1 && delta_col <= 1;
  }

  // The number of rows and columns in the mine field.
  std::size_t rows_;
  std::size_t cols_;

  // Knowledge about the grid of cells.
  Grid<Cell> grid_;

  // Drawing dimensions, updated each time a "configure-event" signal is sent.
  DrawingDimensions dim_;

  // Pixbufs necessary to draw the mine field.
  Pixbufs pixbufs_;

  // Mouse event state tracking.
  MouseState mouse_state_;

  // The most recently clicked cell.
  CellRef clicked_cell_;

  // Queue of events that still need to be handled.
  std::queue<Event> event_queue_;

  // Connection to the timeout signal.
  sigc::connection timeout_connection_;

  // Signal emitted when an action occurs.
  sigc::signal<void, Action> signal_action_;
};

constexpr Color MineField::kFrameColor;

// The main window for the game.
class MinesWindow : public Gtk::ApplicationWindow {
 public:
  MinesWindow(BaseObjectType* cobj, const Glib::RefPtr<Gtk::Builder>& builder)
      : Gtk::ApplicationWindow(cobj),
        field_(GetBuilderWidget<MineField>(builder, "mine-field")),
        solver_algorithm_(solver::Algorithm::NONE) {
    add_action("new", sigc::mem_fun(this, &MinesWindow::NewGame));
    solver_action_ = add_action_radio_string(
        "solver", sigc::mem_fun(this, &MinesWindow::NewSolverAlgorithm),
        "none");

    field_->signal_action().connect(
        sigc::mem_fun(this, &MinesWindow::HandleAction));

    NewGame();
  }

 private:
  // Starts a new game.
  void NewGame() {
    game_ = mines::NewGame(16, 30, 99, std::time(nullptr));
    solver_ = solver::New(solver_algorithm_, *game_);

    game_->Subscribe(field_);
    field_->Reset(game_->GetRows(), game_->GetCols());
  }

  // Changes the solver algorithm and starts a new game.
  void NewSolverAlgorithm(const Glib::ustring& target) {
    solver_action_->change_state(target);

    if (target == "none") {
      solver_algorithm_ = solver::Algorithm::NONE;
    } else if (target == "local") {
      solver_algorithm_ = solver::Algorithm::LOCAL;
    } else {
      solver_algorithm_ = solver::Algorithm::NONE;
    }

    NewGame();
  }

  // Handles an action on the mine field.
  //
  // Executes the action, updates the mine field, and runs the solver.
  void HandleAction(Action action) {
    game_->Execute(action);

    // Execute all actions recommended by the solver.
    std::vector<Action> actions;
    do {
      actions = solver_->Analyze();
      game_->Execute(actions);
    } while (!actions.empty());
  }

  // The mine field.
  MineField* field_;

  // Menu action for selection the solver algorithm.
  Glib::RefPtr<Gio::SimpleAction> solver_action_;

  // The algorithm used in current and new games.
  solver::Algorithm solver_algorithm_;

  // The current game.
  std::unique_ptr<Game> game_;

  // The current solver.
  std::unique_ptr<solver::Solver> solver_;
};

// The master GTK application.
class MinesApplication : public Gtk::Application {
 public:
  static constexpr const char* kApplicationId = "com.alanwj.mines-solver";
  static constexpr const char* kUiResourcePath =
      "/com/alanwj/mines-solver/mines-solver.ui";

  MinesApplication() : Gtk::Application(kApplicationId) {}

 protected:
  void on_startup() final {
    Gtk::Application::on_startup();
    ui_register_resource();

    builder_ = Gtk::Builder::create_from_resource(kUiResourcePath);
    auto menu = Glib::RefPtr<Gio::MenuModel>::cast_dynamic(
        builder_->get_object("menu"));

    set_menubar(menu);
  }

  void on_activate() final {
    Gtk::Application::on_activate();

    window_.reset(GetBuilderWidget<MinesWindow>(builder_, "app-window"));
    add_window(*window_);
    window_->signal_hide().connect(
        sigc::mem_fun(this, &MinesApplication::OnHideWindow));
    window_->present();
  }

 private:
  void OnHideWindow() { window_.reset(); }

  Glib::RefPtr<Gtk::Builder> builder_;
  std::unique_ptr<MinesWindow> window_;
};

}  // namespace

Glib::RefPtr<Gtk::Application> NewGtkUi() {
  return Glib::RefPtr<Gtk::Application>(new MinesApplication());
}

}  // namespace ui
}  // namespace mines
