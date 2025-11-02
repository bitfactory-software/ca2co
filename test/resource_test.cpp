#include <catch2/catch_test_macros.hpp>
#include <co_go/resource.hpp>
#include <mutex>
#include <print>

TEST_CASE("simple [resource]") {
  std::mutex mutex;
  const bool mt_run = true;
  bool run = false;
  bool catched = false;
  try {
    [[maybe_unused]] auto logger{[&](bool log) -> co_go::resource<std::mutex> {
      if (log) std::println("Before locking");
      if (mt_run) mutex.lock();
      co_yield mutex;
      if (mt_run) mutex.unlock();
      if (log) std::println("After locking");
    }(true)};
    std::println("process...");
    run = true;
  } catch (...) {
    catched = true;
  }
  CHECK(run);
  CHECK(!catched);
}

TEST_CASE("throw [resource]") {
  std::mutex mutex;
  const bool mt_run = true;
  bool run = false;
  bool catched = false;
  try {
    [[maybe_unused]] auto logger{[&](bool log) -> co_go::resource<std::mutex> {
      if (log) std::println("Before locking");
      if (mt_run) mutex.lock();
      co_yield mutex;
      if (mt_run) mutex.unlock();
      if (log) std::println("After locking");
    }(true)};
    throw 0;
    // std::println("process...");  // unreachable code
    // run = true;// unreachable code
  } catch (...) {
    catched = true;
  }
  CHECK(!run);
  CHECK(catched);
}
