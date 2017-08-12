#ifndef MINES_GAME_GRID_H_
#define MINES_GAME_GRID_H_

#include <cstddef>
#include <memory>

namespace mines {

// Represents a two dimensional grid of Cells.
template <typename Cell>
class Grid {
 public:
  Grid() : Grid(0, 0) {}

  Grid(std::size_t rows, std::size_t cols) { Reset(rows, cols); }

  ~Grid() = default;

  // Copyable.
  Grid(const Grid&) = default;
  Grid& operator=(const Grid&) = default;

  // Movable.
  Grid(Grid&&) = default;
  Grid& operator=(Grid&&) = default;

  // Resets the grid with new set of cells at the specified dimensions.
  void Reset(std::size_t rows, std::size_t cols) {
    rows_ = rows;
    cols_ = cols;
    if (rows > 0 && cols > 0) {
      cells_.reset(new std::unique_ptr<Cell[]>[rows]);
      for (std::size_t row = 0; row < rows; ++row) {
        cells_[row].reset(new Cell[cols]);
      }
    }
  }

  // Returns the number of rows.
  std::size_t GetRows() const { return rows_; }

  // Returns the number of columns.
  std::size_t GetCols() const { return cols_; }

  // Returns true if the given row and column are valid.
  bool IsValid(std::size_t row, std::size_t col) const {
    return row < rows_ && col < cols_;
  }

  // Returns the Cell at the specified row and column.
  const Cell& operator()(std::size_t row, std::size_t col) const {
    return cells_[row][col];
  }

  // Returns the Cell at the specified row and column.
  Cell& operator()(std::size_t row, std::size_t col) {
    return cells_[row][col];
  }

  // Calls the provided function object for each Cell in the grid.
  //
  // The function should be callable as:
  //   fn(row, col, cell);
  template <class Fn>
  void ForEach(Fn fn) {
    for (std::size_t row = 0; row < rows_; ++row) {
      for (std::size_t col = 0; col < cols_; ++col) {
        fn(row, col, cells_[row][col]);
      }
    }
  }

  // Calls the provided function object for each of the valid adjacent cells.
  //
  // The function should be callable as:
  //   bool v = fn(row, col);
  //
  // Returns the number of function calls that returned true.
  template <class Fn>
  std::size_t ForEachAdjacent(std::size_t row, std::size_t col, Fn fn) const {
    // Note: This relies on the fact that unsigned underflow is well defined.
    std::size_t count = 0;
    count += IsValid(row - 1, col - 1) && fn(row - 1, col - 1) ? 1 : 0;
    count += IsValid(row - 1, col - 0) && fn(row - 1, col - 0) ? 1 : 0;
    count += IsValid(row - 1, col + 1) && fn(row - 1, col + 1) ? 1 : 0;
    count += IsValid(row - 0, col - 1) && fn(row - 0, col - 1) ? 1 : 0;
    count += IsValid(row + 1, col + 1) && fn(row + 1, col + 1) ? 1 : 0;
    count += IsValid(row + 1, col - 0) && fn(row + 1, col - 0) ? 1 : 0;
    count += IsValid(row + 1, col - 1) && fn(row + 1, col - 1) ? 1 : 0;
    count += IsValid(row - 0, col + 1) && fn(row - 0, col + 1) ? 1 : 0;
    return count;
  }

 private:
  std::size_t rows_;
  std::size_t cols_;
  std::unique_ptr<std::unique_ptr<Cell[]>[]> cells_;
};

}  // namespace mines

#endif  // MINES_GAME_GRID_H_
