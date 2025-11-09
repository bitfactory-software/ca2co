#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <functional>
#include <generator>
#include <print>
#include <ranges>
#include <thread>
#include <string_view>
#include <tuple>
#include <optional>

#define CO_GO_CONTINUATION_TEST
#include <ca2co/continuation.hpp>

using namespace std::chrono_literals;

namespace {
namespace fixture {

std::thread a_thread; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
constexpr auto short_break = 10ms;
constexpr auto answer_number = 42;

void async_api_string_view_int(
    std::function<void(std::string_view, int)> const& callback) noexcept {
  a_thread = std::thread{[=] {
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(short_break);
    std::println("sleep on thread {}", std::this_thread::get_id());
    callback("hello world", answer_number);
    std::println("after call to continuation async_api");
  }};
}
ca2co::continuation<std::string_view, int> co_async_api_string_view_int() {
  co_return co_await ca2co::callback_async<std::string_view, int>(
      fixture::async_api_string_view_int);
};
ca2co::continuation<std::string_view, int> co_sync_api_string_view_int() {
  co_return std::make_tuple<std::string_view, int>(std::string_view("xy"), 2);
}

void async_api_int_loop(
    std::function<void(std::optional<std::generator<int>>)> const& callback) noexcept {
  a_thread = std::thread{[=] {
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(short_break);
    std::println("sleep on thread {}", std::this_thread::get_id());
    callback([] -> std::generator<int> {
      for (auto an_i : std::views::iota(0, 3)) co_yield an_i; // NOLINT
    }());
    std::println("after call to continuation async_api");
  }};
}
auto co_async_api_int_loop() {
  return ca2co::callback_async<std::optional<std::generator<int>>>(
      fixture::async_api_int_loop);
};

static_assert(
    !ca2co::is_noexept_callback_api_v<decltype(async_api_string_view_int),
                                      std::tuple<std::string_view, int>>);
static_assert(ca2co::is_noexept_callback_api_v<
              decltype(async_api_string_view_int), std::string_view, int>);
}  // namespace fixture
}  // namespace

TEST_CASE("sync_api_string_view_int") {
  static auto called = false;
  [&] -> ca2co::continuation<> { // NOLINT
    auto [a_s, an_i] = co_await fixture::co_sync_api_string_view_int();
    CHECK(a_s == "xy");
    CHECK(an_i == 2);
    called = true;
  }();
  CHECK(called);
}

TEST_CASE("async_api_string_view_int direct") {
  static auto called = false;
  [&] -> ca2co::continuation<> { // NOLINT
    auto [a_s, an_i] = co_await ca2co::callback_async<std::string_view, int>(
        fixture::async_api_string_view_int);
    CHECK(a_s == "hello world");
    CHECK(an_i == fixture::answer_number);
    called = true;
  }();
  fixture::a_thread.join();
  CHECK(called);
}

TEST_CASE("async_api_string_view_int indirect") {
  static auto called = false;
  [&] -> ca2co::continuation<> { // NOLINT
    auto [a_s, an_i] = co_await fixture::co_async_api_string_view_int();
    CHECK(a_s == "hello world");
    CHECK(an_i == fixture::answer_number);
    called = true;
  }();
  fixture::a_thread.join();
  CHECK(called);
}

TEST_CASE("async_api_int_loop") {
  static auto called = false;
  [&] -> ca2co::continuation<> { // NOLINT
    auto y = 0;
    for (auto an_i : *co_await fixture::co_async_api_int_loop()) {
      CHECK(an_i == y++);
    }
    CHECK(y == 3);
    called = true;
  }();
  fixture::a_thread.join();
  CHECK(called);
}
