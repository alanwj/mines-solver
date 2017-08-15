#ifndef MINES_UI_MINE_FIELD_H_
#define MINES_UI_MINE_FIELD_H_

#include <array>
#include <cstddef>
#include <queue>

#include <cairomm/context.h>
#include <cairomm/refptr.h>
#include <gdkmm/pixbuf.h>
#include <glibmm/refptr.h>
#include <gtkmm/builder.h>
#include <gtkmm/drawingarea.h>
#include <sigc++/connection.h>
#include <sigc++/signal.h>

#include "mines/game/game.h"
#include "mines/game/grid.h"

namespace mines {
namespace ui {

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

// A mine field widget.
class MineField : public Gtk::DrawingArea, public EventSubscriber {
 public:
  MineField(BaseObjectType* cobj, const Glib::RefPtr<Gtk::Builder>&);

  // Resets the internal state for a new game.
  void Reset(Game& game);

  // Updates the visual state based on the event.
  //
  // If the event is not adjacent to the last clicked cell, the event may be
  // queued for later handling.
  void NotifyEvent(const Event& event) final;

  // The signal sent when an Action is peformed on the mine field.
  //
  // The action type will be one of: UNCOVER, CHORD, or FLAG.
  sigc::signal<void, Action>& signal_action() { return signal_action_; }

 protected:
  // Recomputes values necessary to resize the mine field.
  bool on_configure_event(GdkEventConfigure* event) final;

  // Draws the widget.
  bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) final;

  // Handles mouse button presses.
  bool on_button_press_event(GdkEventButton* event) final;

  // Handles mouse button releases.
  bool on_button_release_event(GdkEventButton* event) final;

 private:
  // Reference to a cell and the associated row and column.
  struct CellRef {
    Cell* cell;
    std::size_t row;
    std::size_t col;

    static CellRef None() { return {nullptr, 0, 0}; }
  };

  // A rectangular region of one or more cells, represented as the location of
  // the top left and bottom right cells.
  struct CellRegion {
    // Constructs a cell region containing the specified cell.
    CellRegion(std::size_t row, std::size_t col) {
      min_row = row;
      max_row = row;
      min_col = col;
      max_col = col;
    }

    // Ensures that the specified cell is included in the region.
    void Include(std::size_t row, std::size_t col) {
      min_row = std::min(row, min_row);
      max_row = std::max(row, max_row);
      min_col = std::min(col, min_col);
      max_col = std::max(col, max_col);
    }

    std::size_t min_row;
    std::size_t max_row;
    std::size_t min_col;
    std::size_t max_col;
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
  CellRef GetCellRefFromPoint(int x, int y);

  // Computes the x coordinate, in pixels, of cells in the specified column.
  std::size_t GetCellX(std::size_t col) const;

  // Computes the y coordinate, in pixels, of cells in the specified row.
  std::size_t GetCellY(std::size_t row) const;

  // Requests a redraw clipped to the specified cell.
  void QueueDrawCell(std::size_t row, std::size_t col);

  // Requests a redraw clipped to the specified cell region.
  void QueueDrawCellRegion(const CellRegion& region);

  // Recomputes the drawing dimensions.
  void UpdateDrawingDimensions(int width, int height);

  // Loads a pixbuf from a resource path at current cell dimensions.
  Glib::RefPtr<Gdk::Pixbuf> LoadPixbuf(const char* resource_path) const;

  // Reload all pixbufs at current cell dimensions.
  void UpdatePixbufs();

  // Sets the context color.
  void SetColor(const Cairo::RefPtr<Cairo::Context>& cr,
                const Color& color) const;

  // Draws the frame surrounding the mine field.
  void DrawFrame(const Cairo::RefPtr<Cairo::Context>& cr);

  // Updates the visual state based on the top several events in the event
  // queue.
  //
  // Returns true if there are more events to handle.
  bool HandleEventFromQueue();

  // Updates the visual state based on the event.
  void HandleEvent(const Event& event);

  // Returns true if the specified row and column are in or adjacent to the
  // last clicked cell.
  bool IsAdjacentToClickedCell(std::size_t row, std::size_t col) const;

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

}  // namespace ui
}  // namespace mines

#endif  // MINES_UI_MINE_FIELD_H_
