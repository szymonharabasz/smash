/*
 *
 *    Copyright (c) 2014
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#ifndef SRC_INCLUDE_CXX14COMPAT_H_
#define SRC_INCLUDE_CXX14COMPAT_H_

namespace Smash {

/**
 * Will be in C++14's standard library
 *
 * \see http://en.cppreference.com/w/cpp/memory/unique_ptr/make_unique
 */
template <typename T, typename... Args>
inline std::unique_ptr<T> make_unique(Args &&... args) {
  return std::unique_ptr<T>{new T{std::forward<Args>(args)...}};
}

}  // namespace Smash

#endif  // SRC_INCLUDE_CXX14COMPAT_H_