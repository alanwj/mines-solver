#ifndef MINES_COMPAT_MAKE_UNIQUE_H_
#define MINES_COMPAT_MAKE_UNIQUE_H_

#include <memory>
#include <utility>

// This is a rough equivalent of make_unique, introduced in C++14.
template <typename T, typename... Args>
std::unique_ptr<T> MakeUnique(Args&&... args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

#endif  // MINES_COMPAT_MAKE_UNIQUE_H_
