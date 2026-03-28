#include "atomic_counter_variadic.h"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>

namespace {

void test_single_word() {
  std::atomic<std::uint32_t> word{0};
  atomic_counter::counter c(word);

  for (std::uint64_t i = 0; i != 100000; ++i) {
    assert(c.fetch_add() == i);
  }
}

void test_two_words_boundary() {
  std::atomic<std::uint32_t> low{0};
  std::atomic<std::uint32_t> high{0};
  atomic_counter::counter c(low, high);
  c.reset((std::uint64_t{10} << 31) | (std::uint64_t{1} << 31) - 2);

  const std::uint64_t base = (std::uint64_t{10} << 31) | ((std::uint64_t{1} << 31) - 2);
  assert(c.fetch_add() == base);
  assert(c.fetch_add() == base + 1);
  assert(c.fetch_add() == base + 2);
  assert(c.fetch_add() == base + 3);
}

void test_variadic_reset() {
  std::atomic<std::uint16_t> low{0};
  std::atomic<std::uint32_t> high{0};
  atomic_counter::counter c(low, high);
  c.reset(255);
  assert(c.fetch_add() == 255);
  assert(c.fetch_add() == 256);
}

void test_three_words_boundary() {
  std::atomic<std::uint16_t> a{0};
  std::atomic<std::uint16_t> b{0};
  std::atomic<std::uint32_t> c_word{0};
  atomic_counter::counter c(a, b, c_word);
  constexpr std::uint64_t low_base = (std::uint64_t{1} << 15);
  constexpr std::uint64_t middle_base = (std::uint64_t{1} << 30);
  const std::uint64_t start = middle_base - 2;
  c.reset(start);
  assert(c.fetch_add() == start);
  assert(c.fetch_add() == start + 1);
  assert(c.fetch_add() == start + 2);
  assert(c.fetch_add() == start + 3);
  (void)low_base;
}

void test_multithreaded_uniqueness() {
  std::atomic<std::uint16_t> low{0};
  std::atomic<std::uint32_t> high{0};
  atomic_counter::counter c(low, high);

  constexpr int kThreads = 4;
  constexpr int kPerThread = 50000;

  std::vector<std::uint64_t> values(static_cast<std::size_t>(kThreads) * kPerThread);
  std::vector<std::thread> threads;
  threads.reserve(kThreads);

  for (int t = 0; t != kThreads; ++t) {
    threads.emplace_back([&, t] {
      for (int i = 0; i != kPerThread; ++i) {
        values[static_cast<std::size_t>(t) * kPerThread + i] = c.fetch_add();
      }
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }

  std::sort(values.begin(), values.end());
  for (std::uint64_t i = 0; i != values.size(); ++i) {
    assert(values[static_cast<std::size_t>(i)] == i);
  }
}

}  // namespace

int main() {
 test_single_word();
  test_two_words_boundary();
  test_variadic_reset();
  test_three_words_boundary();
  test_multithreaded_uniqueness();
  std::cout << "ok\n";
}
