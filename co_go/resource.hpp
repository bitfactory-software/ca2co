#include <cassert>
#include <coroutine>

namespace co_go {

template <typename T>
class resource {
 public:
  struct promise_type;

 private:
  using handle_type = std::coroutine_handle<promise_type>;

 public:
  resource(handle_type coro) : coro_(coro) {}

  resource(const resource&) = delete;
  resource& operator=(const resource&) = delete;
  resource(resource&& from) : coro_(std::exchange(from.coro_, nullptr)) {}
  resource& operator=(resource&& from) {
    std::destroy_at(this);
    std::construct_at(this, std::move(from));
    return *this;
  }

  ~resource() {
    if (!coro_) return;
    coro_.resume();        // Resume the coroutine from the co_yield point
    assert(coro_.done());  // Assert that the coroutine ylelds only once
    coro_.destroy();       // Clean up
  }

  struct promise_type {
    const T* yielded_value_p = nullptr;

    auto initial_suspend() noexcept { return std::suspend_never{}; }
    auto final_suspend() noexcept { return std::suspend_always{}; }
    void return_void() noexcept {}
    void unhandled_exception() { throw; }
    std::suspend_always yield_value(const T& value) noexcept {
      yielded_value_p = &value;
      return {};
    }
    // get_return_object is called before initial_suspend(), so before the
    // pre-construction user code. However, conversion from the return type to
    // resource happens after initial_suspend() and after that code.
    // This allows us to return an intermediate object convertible to
    // resource to delay construction of the resource object.
    handle_type get_return_object() { return handle_type::from_promise(*this); }
  };

  const T& operator*() const noexcept {
    assert(!coro_.done());  // Assert that the coroutine ylelds
    assert(coro_.promise().yielded_value_p);
    return *coro_.promise().yielded_value_p;
  }

  const T* operator->() const noexcept {
    assert(!coro_.done());  // Assert that the coroutine ylelds
    return coro_.promise().yielded_value_p;
  }

 private:
  handle_type coro_;
};

}  // namespace co_go
