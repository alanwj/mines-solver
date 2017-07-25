#ifndef MAKE_UNIQUE_H_
#define MAKE_UNIQUE_H_

#include "config.h"

#if !HAVE_CXX14

#include <memory>
#include <utility>

namespace std {

// This is an implementation of make_unique, introduced in C++14.
//
// If the compiler supports C++14 its version will be used instead.
template <typename T, typename... Args>
unique_ptr<T> make_unique(Args&&... args) {
  return unique_ptr<T>(new T(forward<Args>(args)...));
}

}  // namespace std

#endif  // HAVE_CXX14

#endif  // MAKE_UNIQUE_H_
