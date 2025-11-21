#include <catch2/catch_test_macros.hpp>
#include <chrono>  // NOLINT(misc-include-cleaner)
#include <functional>
#include <iostream>
#include <thread>

#define CA2CO_TEST
#include <ca2co/continuation.hpp>

using namespace std::chrono_literals;

namespace {
int step = 1;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
constexpr auto short_break = 10ms;  // NOLINT(misc-include-cleaner)
constexpr auto answer_number = 42;

// + lib callback style
void int_callback_api(                                    // NOLINT
    std::function<void(int)> const& callback) noexcept {  // NOLINT
  std::cout << "before callback\n";
  callback(answer_number);
  std::cout << "after callback\n";
};
// - lib callback style
// + lib wrapped for coro style
auto int_recieve_coro() { return ca2co::callback_sync<int>(int_callback_api); };
// - lib wrapped for coro style
ca2co::continuation<int> int_recieve_coro_indirect() {
  auto x = co_await ca2co::callback_sync<int>(int_callback_api);
  std::cout << "int_recieve_coro_indirect after callback" << x << "\n";
  co_return x + 1;
};
// - lib wrapped for coro style

}  // namespace

TEST_CASE("int [continuation]") {
  // + call callback style
  step = 1;
  CHECK(step == 1);
  int_callback_api([&](int _42) {
    std::cout << "recieving 42\n";
    CHECK(answer_number == _42);
  });
  // - app callback style

  [] -> ca2co::continuation<> {
    // call coro style must exist inside a coro
    auto _42 = co_await int_recieve_coro();
    std::cout << "recieving 42\n";
    CHECK(answer_number == _42);
  }();

  [] -> ca2co::continuation<> {
    // call coro style must exist inside a coro
    auto _43 = co_await int_recieve_coro_indirect();
    std::cout << "recieving 43\n";
    CHECK(43 == _43);
  }();
}

namespace {
void async_api(std::function<void(int)> const& continuation) noexcept {
  auto t = std::thread {
    [=] {
      std::this_thread::sleep_for(short_break);
      std::cout << "sleep on thread " << std::this_thread::get_id() << "\n";
      continuation(answer_number);
      std::cout << "after call to continuation async_api\n";
    }
  };
  t.join();
}

ca2co::continuation<int> async_api_coro() {
  co_return co_await ca2co::callback_sync<int>(async_api);
};
ca2co::continuation<int> async_api_coro_indirect() {
  auto x = co_await async_api_coro();
  CHECK(x == answer_number);
  co_return x + 1;
};
}  // namespace

TEST_CASE("int async [continuation]") {
  static auto id_start = std::this_thread::get_id();
  static auto called = false;
  [] -> ca2co::continuation<> {
    // call coro style must exist inside a coro
    auto _42 = co_await async_api_coro();  // blocks!
    std::cout << "recieving 42\n";
    called = true;
    CHECK(answer_number == _42);
    CHECK(id_start == std::this_thread::get_id());
  }();
  CHECK(called);
}

TEST_CASE("int async indirect [continuation]") {
  static auto id_start = std::this_thread::get_id();
  static auto called = false;
  [] -> ca2co::continuation<> {
    // call coro style must exist inside a coro
    auto _43 = co_await async_api_coro_indirect();
    std::cout << "recieving 43\n";
    called = true;
    CHECK(43 == _43);
    CHECK(id_start == std::this_thread::get_id());
  }();
  CHECK(called);
}
