#ifndef MINES_GAME_GRID_H_
#define MINES_GAME_GRID_H_

#include <algorithm>
#include <cstddef>
#include <vector>

namespace mines {

// Represents a two dimensional grid of Cells.
template <typename Cell>
class Grid {
 public:
  Grid(std::size_t rows, std::size_t cols)
      : cells_(std::max(rows, std::size_t{1}),
               std::vector<Cell>(std::max(cols, std::size_t{1}))) {}

  ~Grid() = default;

  // Copyable.
  Grid(const Grid&) = default;
  Grid& operator=(const Grid&) = default;

  // Movable.
  Grid(Grid&&) = default;
  Grid& operator=(Grid&&) = default;

  // Returns the number of rows.
  std::size_t GetRows() const { return cells_.size(); }

  // Returns the number of columns.
  std::size_t GetCols() const { return cells_[0].size(); }

  // Returns true if the given row and column are valid.
  bool IsValid(std::size_t row, std::size_t col) const {
    return row < GetRows() && col < GetCols();
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
    const std::size_t rows = GetRows();
    const std::size_t cols = GetCols();
    for (std::size_t row = 0; row < rows; ++row) {
      for (std::size_t col = 0; col < cols; ++col) {
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
  std::vector<std::vector<Cell>> cells_;
};

}  // namespace mines

#endif  // MINES_GAME_GRID_H_
