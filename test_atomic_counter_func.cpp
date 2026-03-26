#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>

#include "atomic_counter_func.h"

namespace {

void test_sequential_zero() {
  std::atomic<std::uint32_t> low{0};
  std::atomic<std::uint32_t> high{0};

  for (std::uint64_t i = 0; i != 100000; ++i) {
    assert(fetch_add(low, high) == i);
  }
}

void test_boundary() {
  const std::uint32_t high_bits = 10;
  const std::uint32_t low_bits = low_significant_mask - 2;
  std::atomic<std::uint32_t> high{high_bits};
  std::atomic<std::uint32_t> low{low_bits | ((high_bits & 1U) << 31)};

  const std::uint64_t base = (static_cast<std::uint64_t>(high_bits) << 31) | low_bits;
  assert(fetch_add(low, high) == base);
  assert(fetch_add(low, high) == base + 1);
  assert(fetch_add(low, high) == base + 2);
  assert(fetch_add(low, high) == base + 3);
}

void test_multithreaded_uniqueness() {
  std::atomic<std::uint32_t> low{0};
  std::atomic<std::uint32_t> high{0};

  constexpr int kThreads = 4;
  constexpr int kPerThread = 50000;

  std::vector<std::uint64_t> values(static_cast<std::size_t>(kThreads) * kPerThread);
  std::vector<std::thread> threads;
  threads.reserve(kThreads);

  for (int t = 0; t != kThreads; ++t) {
    threads.emplace_back([&, t] {
      for (int i = 0; i != kPerThread; ++i) {
        values[static_cast<std::size_t>(t) * kPerThread + i] = fetch_add(low, high);
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
  test_sequential_zero();
  test_boundary();
  test_multithreaded_uniqueness();
  std::cout << "ok\n";
}
