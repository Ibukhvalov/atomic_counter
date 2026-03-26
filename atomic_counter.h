#pragma once

#include <atomic>
#include <cassert>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace atomic_counter {

namespace {

constexpr std::uint64_t bit_mask(unsigned bits) noexcept {
  return bits >= 64 ? ~std::uint64_t{0} : ((std::uint64_t{1} << bits) - 1U);
}

template <typename T>
constexpr bool is_valid_word = std::is_integral_v<T> && std::is_unsigned_v<T> && !std::is_same_v<T, bool>;

template <typename T>
constexpr unsigned word_bits = std::numeric_limits<T>::digits;

}

template <typename... Ts>
class counter;

template <typename T>
class counter<T> {
  static_assert(is_valid_word<T>, "counter words must be unsigned integral types");

 public:
  static constexpr unsigned total_bits = word_bits<T>;
  static constexpr std::uint64_t maxValue = bit_mask(total_bits);

  explicit counter(std::atomic<T>& word) noexcept : word_(word) {}

  void reset(std::uint64_t newValue = 0) noexcept {
    assert(newValue <= maxValue);
    word_.store(static_cast<T>(newValue), std::memory_order_relaxed);
  }

  std::uint64_t fetch_add() noexcept {
    return static_cast<std::uint64_t>(
        word_.fetch_add(T{1}, std::memory_order_acq_rel));
  }

 private:
  std::uint64_t load() const noexcept {
    return static_cast<std::uint64_t>(word_.load(std::memory_order_acquire));
  }

  void bump() noexcept {
    word_.fetch_add(T{1}, std::memory_order_acq_rel);
  }

  std::atomic<T>& word_;

  template <typename...>
  friend class counter;
};

template <typename Low, typename... HighTs>
class counter<Low, HighTs...> {
  static_assert(is_valid_word<Low>, "counter words must be unsigned integral types");
  static_assert(word_bits<Low> >= 2,
                "every non-top word needs at least one payload bit and one phase bit");

  using high_counter = counter<HighTs...>;

 public:
  static constexpr unsigned low_word_bits = word_bits<Low>;
  static constexpr unsigned low_significant_bits = low_word_bits - 1;
  static constexpr std::uint64_t low_significant_mask = bit_mask(low_significant_bits);
  static constexpr unsigned total_bits = low_significant_bits + high_counter::total_bits;
  static_assert(total_bits <= 64, "counter must fit into uint64_t");
  static constexpr std::uint64_t maxValue = bit_mask(total_bits);

  explicit counter(std::atomic<Low>& low, std::atomic<HighTs>&... high_words) noexcept
      : low_(low), high_(high_words...) {}

  void reset(std::uint64_t newValue = 0) noexcept {
    assert(newValue <= maxValue);

    const std::uint64_t high_value = newValue >> low_significant_bits;
    const auto low_payload = static_cast<Low>(newValue & low_significant_mask);
    const auto phase = static_cast<Low>((high_value & 1ULL) << low_significant_bits);

    high_.reset(high_value);
    low_.store(static_cast<Low>(low_payload | phase), std::memory_order_relaxed);
  }

  std::uint64_t fetch_add() noexcept {
    const std::uint64_t high_before = high_.load();
    const Low old_low = low_.fetch_add(Low{1}, std::memory_order_acq_rel);

    const std::uint64_t old_low_u64 = static_cast<std::uint64_t>(old_low);
    const std::uint64_t old_phase = old_low_u64 >> low_significant_bits;
    const std::uint64_t high_corrected =
        high_before + static_cast<std::uint64_t>((high_before & 1ULL) != old_phase);

    if ((old_low_u64 & low_significant_mask) == low_significant_mask) {
      high_.bump();
    }

    return (high_corrected << low_significant_bits) | (old_low_u64 & low_significant_mask);
  }

 private:
  std::uint64_t load() const noexcept {
    const std::uint64_t high_before = high_.load();
    const auto low_now =
        static_cast<std::uint64_t>(low_.load(std::memory_order_acquire));
    const std::uint64_t phase = low_now >> low_significant_bits;
    const std::uint64_t high_corrected =
        high_before + static_cast<std::uint64_t>((high_before & 1ULL) != phase);

    return (high_corrected << low_significant_bits) | (low_now & low_significant_mask);
  }

  void bump() noexcept {
    const std::uint64_t old_low = static_cast<std::uint64_t>(
        low_.fetch_add(Low{1}, std::memory_order_acq_rel));

    if ((old_low & low_significant_mask) == low_significant_mask) {
      high_.bump();
    }
  }

  std::atomic<Low>& low_;
  high_counter high_;

  template <typename...>
  friend class counter;
};

template <typename... Ts>
counter(std::atomic<Ts>&...) -> counter<Ts...>;

}  // namespace atomic_counter
