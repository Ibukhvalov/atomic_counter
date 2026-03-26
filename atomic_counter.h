#pragma once

#include <atomic>
#include <cstdint>

namespace atomic_counter {

static uint32_t lowSignificantMask = 0x7fffffff;

 inline std::uint64_t fetch_add(std::atomic<std::uint32_t>& low,
                               std::atomic<std::uint32_t>& high) noexcept {
  const std::uint32_t high_before = high.load(std::memory_order_acquire);
  const std::uint32_t old_low = low.fetch_add(1u, std::memory_order_acq_rel);

  const std::uint32_t old_phase = old_low >> 31;
  std::uint32_t high_corrected = high_before + ((high_before & 1u) != old_phase);

  if ((old_low & lowSignificantMask) == lowSignificantMask) {
    high.fetch_add(1U, std::memory_order_release);
  }

  return (static_cast<std::uint64_t>(high_corrected) << 31) |
         static_cast<std::uint64_t>(old_low & lowSignificantMask);
}

}  // namespace atomic_counter
