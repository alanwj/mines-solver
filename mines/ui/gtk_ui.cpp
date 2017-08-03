#include "mines/ui/gtk_ui.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <queue>
#include <utility>

#include <glibmm/main.h>
#include <gtkmm/application.h>
#include <gtkmm/drawingarea.h>
#include <gtkmm/window.h>
#include <sigc++/sigc++.h>

#include "mines/compat/make_unique.h"
#include "mines/game/game.h"
#include "mines/game/grid.h"

namespace mines {
namespace ui {

namespace {

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
                    const DrawingDimensions& dim, const Cell& cell,
                    std::size_t row, std::size_t col)
      : cr_(cr), dim_(dim), cell_(cell), row_(row), col_(col) {}

  CellDrawFlyweight(const CellDrawFlyweight&) = delete;
  CellDrawFlyweight& operator=(const CellDrawFlyweight&) = delete;
  CellDrawFlyweight(CellDrawFlyweight&&) = delete;
  CellDrawFlyweight& operator=(CellDrawFlyweight&&) = delete;

  void Draw() const {
    switch (cell_.state) {
      default:
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

  // Draws a cell in the FLAGGED state.
  void DrawFlagged() const {
    DrawCovered();

    // TODO: Draw a proper flag.
    SetColor({1.0, 0.0, 0.0});
    cr_->save();
    cr_->scale(dim_.cell_size, dim_.cell_size);
    cr_->rectangle(.25, .25, .5, .5);
    cr_->fill();
    cr_->restore();
  }

  // Draws a cell in the MINE state.
  void DrawMine() const {
    DrawEmpty(kCellColor);

    // TODO: Draw a proper mine.
    SetColor({0.0, 0.0, 0.0});
    cr_->save();
    cr_->scale(dim_.cell_size, dim_.cell_size);
    cr_->rectangle(.25, .25, .5, .5);
    cr_->fill();
    cr_->restore();
  }

  // Draws a cell in the LOSING_MINE state.
  void DrawLosingMine() const {
    DrawEmpty(kLosingMineCellColor);

    // TODO: Draw a proper mine.
    SetColor({0.0, 0.0, 0.0});
    cr_->save();
    cr_->scale(dim_.cell_size, dim_.cell_size);
    cr_->rectangle(.25, .25, .5, .5);
    cr_->fill();
    cr_->restore();
  }

  const Cairo::RefPtr<Cairo::Context>& cr_;
  const DrawingDimensions& dim_;
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
class MineField : public Gtk::DrawingArea {
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

  MineField(std::size_t rows, std::size_t cols)
      : rows_(rows),
        cols_(cols),
        grid_(rows, cols),
        dim_{0, 0, kCellSize * cols + 2 * kFrameSize,
             kCellSize * rows + 2 * kFrameSize, kCellSize},
        clicked_cell_(CellRef::None()) {
    set_size_request(dim_.width, dim_.height);

    // DrawingArea subclasses have to explicitly ask for mouse events.
    add_events(Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK);
  }

  // Updates the visual state based on events.
  void Update(const std::vector<Event>& events) {
    for (const Event& event : events) {
      // Don't wait for events adjacent to the clicked cell.
      if (clicked_cell_.cell != nullptr &&
          IsAdjacentToClickedCell(event.row, event.col)) {
        HandleEvent(event);
      } else {
        event_queue_.push(event);
      }
    }
    if (!event_queue_.empty()) {
      // Handle the rest of the events on a timeout.
      Glib::signal_timeout().connect(
          sigc::mem_fun(this, &MineField::HandleEventFromQueue),
          kEventTimeoutMs);
    }
  }

  // The signal sent when an Action is peformed on the mine field.
  //
  // The action type will be one of: UNCOVER, CHORD, or FLAG.
  sigc::signal<void, Action>& signal_action() { return signal_action_; }

 protected:
  /// Recomputes the drawing dimensions.
  bool on_configure_event(GdkEventConfigure* event) final {
    dim_.cell_size = std::min((event->width - 2 * kFrameSize) / cols_,
                              (event->height - 2 * kFrameSize) / rows_);
    dim_.width = dim_.cell_size * cols_ + 2 * kFrameSize;
    dim_.height = dim_.cell_size * rows_ + 2 * kFrameSize;

    // Center the part of the drawing area we are actually using within the
    // total available space.
    dim_.x = event->width / 2 - dim_.width / 2;
    dim_.y = event->height / 2 - dim_.height / 2;

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

        CellDrawFlyweight flyweight(cr, dim_, grid_(row, col), row, col);
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

  // Updates the visual state based on the top event in the event queue.
  //
  // Returns true if there are more events to handle.
  bool HandleEventFromQueue() {
    if (event_queue_.empty()) {
      return false;
    }
    HandleEvent(event_queue_.front());
    event_queue_.pop();
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
      case Event::Type::QUIT:
        break;
      case Event::Type::SHOW_MINE:
        cell.state = CellState::MINE;
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
    return delta_row <= 1 || delta_col <= 1;
  }

  // The number of rows and columns in the mine field.
  const std::size_t rows_;
  const std::size_t cols_;

  // Knowledge about the grid of cells.
  Grid<Cell> grid_;

  // Drawing dimensions, updated each time a "configure-event" signal is sent.
  DrawingDimensions dim_;

  // Mouse event state tracking.
  MouseState mouse_state_;

  CellRef clicked_cell_;

  // Signal emitted when an action occurs.
  sigc::signal<void, Action> signal_action_;

  // Queue of events that still need to be handled.
  std::queue<Event> event_queue_;
};

constexpr Color MineField::kFrameColor;

// The main window for the game.
class MinesWindow : public Gtk::Window {
 public:
  static constexpr const char* kTitle = "Mines";
  static constexpr std::size_t kBorderWidth = 8;

  MinesWindow(Game& game, solver::Solver& solver)
      : game_(game), solver_(solver), field_(game.GetRows(), game.GetCols()) {
    set_title(kTitle);
    set_border_width(kBorderWidth);
    set_position(Gtk::WIN_POS_CENTER);

    field_.signal_action().connect(
        sigc::mem_fun(this, &MinesWindow::HandleAction));
    add(field_);
    show_all_children();
  }

 private:
  // Handles an action on the mine field.
  //
  // Executes the action, updates the mine field, and runs the solver.
  void HandleAction(Action action) {
    std::vector<Event> events = game_.Execute(action);
    field_.Update(events);
    solver_.Update(events);

    std::vector<Action> actions = solver_.Analyze();
    while (!actions.empty()) {
      for (const Action& action : actions) {
        events = game_.Execute(action);
        field_.Update(events);
        solver_.Update(events);
      }
      actions = solver_.Analyze();
    }
  }

  Game& game_;
  solver::Solver& solver_;
  MineField field_;
};

class GtkUiImpl : public GtkUi {
 public:
  static constexpr const char* kApplicationId = "com.alanwj.mines-solver";

  GtkUiImpl(int& argc, char**& argv)
      : application_(Gtk::Application::create(argc, argv, kApplicationId)) {}
  ~GtkUiImpl() final = default;

  void Play(Game& game, solver::Solver& solver) final {
    MinesWindow window(game, solver);
    application_->run(window);
  }

 private:
  Glib::RefPtr<Gtk::Application> application_;
};

}  // namespace

std::unique_ptr<GtkUi> NewGtkUi(int& argc, char**& argv) {
  return std::make_unique<GtkUiImpl>(argc, argv);
}

}  // namespace ui
}  // namespace mines
