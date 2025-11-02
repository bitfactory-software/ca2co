#include <coroutine>
#include <exception>

namespace co_go {

template <typename R>
struct continuation {
  template <typename HandleReturn>
  struct basic_promise_type : HandleReturn {
    continuation<R> get_return_object(this auto& self) {
      return continuation<R>{
          std::coroutine_handle<basic_promise_type>::from_promise(self)};
    }

    struct await_continuation {
      await_continuation() noexcept {}
      bool await_ready() const noexcept { return false; }
      void await_suspend(
          std::coroutine_handle<basic_promise_type> this_coroutine) noexcept {
        auto& promise = this_coroutine.promise();
        if (promise.calling_coroutine_) promise.calling_coroutine_.resume();
        if (!promise.awaited_) this_coroutine.destroy();
      }
      void await_resume() noexcept {}
    };
    auto initial_suspend() noexcept { return std::suspend_never{}; }
    auto final_suspend() noexcept { return await_continuation{}; }
    void unhandled_exception() noexcept {
      exception_ = std::current_exception();
    }
    std::coroutine_handle<> calling_coroutine_ = {};
    std::exception_ptr exception_ = {};
    bool sync_ = true;
    bool awaited_ = true;
  };
  template <typename R>
  struct handle_return {
    void return_value(R result) { result_ = result; }
    R result_ = {};
  };
  template <>
  struct handle_return<void> {
    void return_void() {};
  };
  using promise_type = basic_promise_type<handle_return<R>>;

  continuation(const continuation&) = delete;
  continuation& operator=(const continuation&) = delete;
  continuation& operator=(continuation&& r) noexcept = delete;

  continuation() noexcept = default;
  explicit continuation(std::coroutine_handle<promise_type> coroutine)
      : coroutine_(coroutine) {}
  ~continuation() noexcept {
    if (!coroutine_) return;
    if (coroutine_.promise().sync_)
      coroutine_.destroy();
    else
      coroutine_.promise().awaited_ = false;
  }

  bool await_ready() const noexcept { return false; }
  void await_suspend(auto calling_coroutine) noexcept {
    if (!coroutine_ || coroutine_.promise().sync_)
      calling_coroutine.resume();
    else
      build_async_chain(this->coroutine_, calling_coroutine);
  }
  auto await_resume() {
    if (!coroutine_) return R{};
    if (auto exception = coroutine_.promise().exception_)
      std::rethrow_exception(exception);
    auto result = coroutine_.promise().result_;
    if (!coroutine_.promise().awaited_) coroutine_.destroy();
    return result;
  }

 private:
  static void build_async_chain(auto suspended_coroutine,
                                auto calling_coroutine) {
    suspended_coroutine.promise().calling_coroutine_ = calling_coroutine;
    calling_coroutine.promise().sync_ = false;
  }
  std::coroutine_handle<promise_type> coroutine_;
};

template <typename R, typename Api>
struct continuation_awaiter {
  bool await_ready() { return false; }
  void await_suspend(auto calling_continuation) {
    bool called = false;
    api_([this, calling_continuation, called](const R& r) mutable {
      if (called) return;
      called = true;
      result_ = r;
      calling_continuation.resume();
    });
  }
  R await_resume() { return result_; }
  const Api api_;
  R result_ = {};
};
template <typename Api>
struct continuation_awaiter<void, Api> {
  bool await_ready() { return false; }
  void await_suspend(auto calling_continuation) {
    bool called = false;
    api_([this, calling_continuation, called]() mutable {
      if (called) return;
      called = true;
      calling_continuation.resume();
    });
  }
  void await_resume() {}
  const Api api_;
};
template <typename R, typename Api>
auto await_callback(Api&& api) {
  return continuation_awaiter<R, std::decay_t<Api>>{std::move(api)};
}

}  // namespace co_go
