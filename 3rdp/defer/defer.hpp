/*
https://oded.dev/2017/10/05/go-defer-in-cpp/
*/

#pragma once

#include <functional>

class ScopeGuard {
 public:
  template<class Callable>
  ScopeGuard(Callable &&fn) : fn_(std::forward<Callable>(fn)) {}

  ScopeGuard(ScopeGuard &&other) : fn_(std::move(other.fn_)) {
    other.fn_ = nullptr;
  }

  ~ScopeGuard() {
    // must not throw
    if (fn_) fn_();
  }

  ScopeGuard(const ScopeGuard &) = delete;
  void operator=(const ScopeGuard &) = delete;
  void detach() {
    fn_ = nullptr;
  }

 private:
  std::function<void()> fn_;
};

#define DEFER_CONCAT_(a, b) a ## b
#define DEFER_CONCAT(a, b) DEFER_CONCAT_(a,b)
#define DEFER(fn) ScopeGuard DEFER_CONCAT(__defer__, __LINE__) = [&] ( ) { fn ; }