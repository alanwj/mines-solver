#include "mines/ui/mine_field.h"

#include <algorithm>

#include <cairomm/enums.h>
#include <cairomm/types.h>
#include <gdkmm/general.h>
#include <glibmm/main.h>
#include <sigc++/functors/mem_fun.h>

#include "mines/compat/gdk_pixbuf.h"

namespace mines {
namespace ui {

using detail::DrawingDimensions;
using detail::Pixbufs;
using detail::Cell;

namespace {

// The size of the frame around the mine field.
constexpr std::size_t kFrameSize = 1;

// The default (and minimum) cell size.
constexpr std::size_t kCellSize = 20;

// The time (in milliseconds) between processing each event.
// A small visual delay between events gives a natural feel when areas are
// uncovered.
constexpr unsigned int kEventTimeoutMs = 1;

// Resource path for the mine graphic.
constexpr const char* kMineResourcePath = "/com/alanwj/mines-solver/mine.svg";

// Resource path for the flag graphic.
constexpr const char* kFlagResourcePath = "/com/alanwj/mines-solver/flag.svg";

// Encapsulates a simple RGB color.
struct Color {
  double r;
  double g;
  double b;
};

// The color to use for numbers 1 through 8.
constexpr std::array<Color, 8> kNumberColor{{
    {0.0, 0.0, 1.0},  // 1
    {0.0, 0.5, 0.0},  // 2
    {1.0, 0.0, 0.0},  // 3
    {0.0, 0.0, 0.5},  // 4
    {0.5, 0.0, 0.0},  // 5
    {0.0, 0.5, 0.5},  // 6
    {0.5, 0.0, 0.5},  // 7
    {0.0, 0.0, 0.0},  // 8
}};

// Colors used in widget construction.
constexpr Color kFrameColor{0.5, 0.5, 0.5};
constexpr Color kCellColor{0.76, 0.76, 0.76};
constexpr Color kLosingMineCellColor{1.0, 0.0, 0.0};
constexpr Color kCellBorderColor{0.5, 0.5, 0.5};
constexpr Color kLightBevelColor{1.0, 1.0, 1.0};
constexpr Color kDarkBevelColor{0.5, 0.5, 0.5};

// Sets the source color in the specified context.
void SetColor(const Cairo::RefPtr<Cairo::Context>& cr, const Color& color) {
  cr->set_source_rgb(color.r, color.g, color.b);
}

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
  // Fills the cell with the specified color.
  void FillCell(const Color& color) const {
    SetColor(cr_, color);
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
    SetColor(cr_, kCellBorderColor);
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
      SetColor(cr_, kNumberColor[cell_.adjacent_mines - 1]);
      DrawChar('0' + cell_.adjacent_mines);
    }
  }

  // Draws a cell that is visually pressed by a mouse action.
  void DrawPressed() const {
    FillCell(kCellColor);
    SetColor(cr_, kDarkBevelColor);
    DrawHLine(0, 0, dim_.cell_size);
    DrawVLine(0, 0, dim_.cell_size);
    cr_->stroke();
  }

  // Draws a cell in the COVERED state.
  void DrawCovered() const {
    FillCell(kCellColor);

    // Light bevel on top.
    SetColor(cr_, kLightBevelColor);
    DrawHLine(0, 0, dim_.cell_size - 1);
    DrawHLine(0, 1, dim_.cell_size - 2);

    // Light bevel on left.
    DrawVLine(0, 0, dim_.cell_size - 1);
    DrawVLine(1, 0, dim_.cell_size - 2);
    cr_->stroke();

    // Dark bevel on bottom.
    SetColor(cr_, kDarkBevelColor);
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
    SetColor(cr_, {1.0, 0.0, 0.0});
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

}  // namespace

MineField::MineField(BaseObjectType* cobj, const Glib::RefPtr<Gtk::Builder>&)
    : Gtk::DrawingArea(cobj) {}

void MineField::Reset(Game& game) {
  game.Subscribe(this);

  rows_ = game.GetRows();
  cols_ = game.GetCols();
  grid_.Reset(rows_, cols_);

  const int min_width = kCellSize * cols_ + 2 * kFrameSize;
  const int min_height = kCellSize * rows_ + 2 * kFrameSize;

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

void MineField::NotifyEvent(const Event& event) {
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

void MineField::on_size_allocate(Gtk::Allocation& allocation) {
  Gtk::DrawingArea::on_size_allocate(allocation);
  UpdateDrawingDimensions(allocation.get_width(), allocation.get_height());
  UpdatePixbufs();
}

bool MineField::on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
  // We start by computing the top left (min) and bottom right (max) cell that
  // will need to be drawn.
  double clip_x;
  double clip_y;
  double clip_width;
  double clip_height;
  cr->get_clip_extents(clip_x, clip_y, clip_width, clip_height);

  CellRef min_cell = GetCellRefFromPoint(clip_x, clip_y);
  if (min_cell.cell == nullptr) {
    min_cell = CellRef{nullptr, 0, 0};
  }

  CellRef max_cell = GetCellRefFromPoint(clip_width - 1.0, clip_height - 1.0);
  if (max_cell.cell == nullptr) {
    max_cell = CellRef{nullptr, rows_ - 1, cols_ - 1};
  }

  // Defaults for drawing lines.
  cr->set_line_width(1);
  cr->set_line_cap(Cairo::LINE_CAP_SQUARE);

  cr->save();
  cr->translate(dim_.x, dim_.y);
  DrawFrame(cr);
  cr->restore();

  for (std::size_t row = min_cell.row; row <= max_cell.row; ++row) {
    for (std::size_t col = min_cell.col; col <= max_cell.col; ++col) {
      cr->save();
      cr->translate(GetCellX(col), GetCellY(row));

      CellDrawFlyweight flyweight(cr, dim_, pixbufs_, grid_(row, col), row,
                                  col);
      flyweight.Draw();

      cr->restore();
    }
  }

  return false;
}

bool MineField::on_button_press_event(GdkEventButton* event) {
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

  CellRegion region(clicked_cell_.row, clicked_cell_.col);

  if (mouse_state_.btn[0] || mouse_state_.btn[2]) {
    clicked_cell_.cell->pressed = true;
  }

  if (mouse_state_.btn[1] || (mouse_state_.btn[0] && mouse_state_.btn[2])) {
    clicked_cell_.cell->pressed = false;
    grid_.ForEachAdjacent(clicked_cell_.row, clicked_cell_.col,
                          [this, &region](std::size_t row, std::size_t col) {
                            region.Include(row, col);
                            grid_(row, col).pressed = true;
                            return false;
                          });
  }

  QueueDrawCellRegion(region);

  return false;
}

bool MineField::on_button_release_event(GdkEventButton* event) {
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
    CellRegion region(clicked_cell_.row, clicked_cell_.col);
    clicked_cell_.cell->pressed = false;
    grid_.ForEachAdjacent(clicked_cell_.row, clicked_cell_.col,
                          [this, &region](std::size_t row, std::size_t col) {
                            region.Include(row, col);
                            grid_(row, col).pressed = false;
                            return false;
                          });
    QueueDrawCellRegion(region);
  }

  // Reset the mouse state.
  mouse_state_ = MouseState();

  return false;
}

MineField::CellRef MineField::GetCellRefFromPoint(int x, int y) {
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

std::size_t MineField::GetCellX(std::size_t col) const {
  return dim_.x + kFrameSize + col * dim_.cell_size;
}

std::size_t MineField::GetCellY(std::size_t row) const {
  return dim_.y + kFrameSize + row * dim_.cell_size;
}

void MineField::QueueDrawCell(std::size_t row, std::size_t col) {
  queue_draw_area(GetCellX(col), GetCellY(row), dim_.cell_size, dim_.cell_size);
}

void MineField::QueueDrawCellRegion(const CellRegion& region) {
  queue_draw_area(GetCellX(region.min_col), GetCellY(region.min_row),
                  (region.max_col - region.min_col + 1) * dim_.cell_size,
                  (region.max_row - region.min_row + 1) * dim_.cell_size);
}

void MineField::UpdateDrawingDimensions(int width, int height) {
  dim_.cell_size = std::min((width - 2 * kFrameSize) / cols_,
                            (height - 2 * kFrameSize) / rows_);
  dim_.width = dim_.cell_size * cols_ + 2 * kFrameSize;
  dim_.height = dim_.cell_size * rows_ + 2 * kFrameSize;

  // Center the part of the drawing area we are actually using within the
  // total available space.
  dim_.x = width / 2 - dim_.width / 2;
  dim_.y = height / 2 - dim_.height / 2;
}

Glib::RefPtr<Gdk::Pixbuf> MineField::LoadPixbuf(
    const char* resource_path) const {
  return compat::CreatePixbufFromResource(resource_path, 0.7 * dim_.cell_size,
                                          0.7 * dim_.cell_size);
}

void MineField::UpdatePixbufs() {
  pixbufs_.mine = LoadPixbuf(kMineResourcePath);
  pixbufs_.flag = LoadPixbuf(kFlagResourcePath);
}

void MineField::DrawFrame(const Cairo::RefPtr<Cairo::Context>& cr) {
  SetColor(cr, kFrameColor);
  cr->rectangle(0.0, 0.0, dim_.width, kFrameSize);
  cr->rectangle(0.0, 0.0, kFrameSize, dim_.height);
  cr->rectangle(0.0, dim_.height - kFrameSize, dim_.width, kFrameSize);
  cr->rectangle(dim_.width - kFrameSize, 0.0, kFrameSize, dim_.height);
  cr->fill();
}

bool MineField::HandleEventFromQueue() {
  // Handle 3 events at a time because 1ms is too slow for the timer but
  // there is no way to set a submillisecond timer.
  for (std::size_t i = 0; i < 3 && !event_queue_.empty(); ++i) {
    HandleEvent(event_queue_.front());
    event_queue_.pop();
  }
  return !event_queue_.empty();
}

void MineField::HandleEvent(const Event& event) {
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
  QueueDrawCell(event.row, event.col);
}

bool MineField::IsAdjacentToClickedCell(std::size_t row,
                                        std::size_t col) const {
  const std::size_t delta_row =
      std::max(clicked_cell_.row, row) - std::min(clicked_cell_.row, row);
  const std::size_t delta_col =
      std::max(clicked_cell_.col, col) - std::min(clicked_cell_.col, col);
  return delta_row <= 1 && delta_col <= 1;
}

}  // namespace ui
}  // namespace mines
