#pragma once

#include <atomic>
#include <cstddef>
#include <shared_mutex>
#include <span>

#include "common/types.hpp"

namespace im110 {

class BufferPoolManager;

// FrameHeader represents one in-memory frame. The BufferPoolManager owns the
// frame and updates the bookkeeping fields under its latch (pin_count_ is
// atomic so it can be incremented/decremented on the guard fast path). Guards
// never read the bookkeeping fields directly; they only access the page bytes
// and the per-frame shared mutex through the accessors below.
class FrameHeader {
 public:
  FrameHeader() = default;

  FrameHeader(const FrameHeader&) = delete;
  auto operator=(const FrameHeader&) -> FrameHeader& = delete;

  auto GetData() const -> std::span<const std::byte, PAGE_SIZE> {
    return std::span<const std::byte, PAGE_SIZE>{data_};
  }
  auto GetMutableData() -> std::span<std::byte, PAGE_SIZE> {
    return std::span<std::byte, PAGE_SIZE>{data_};
  }
  auto GetMutex() const -> std::shared_mutex& { return page_latch_; }

 private:
  friend class BufferPoolManager;

  void Reset() {
    page_id_ = INVALID_PAGE_ID;
    pin_count_.store(0, std::memory_order_release);
    is_dirty_ = false;
    data_.fill(std::byte{0});
  }

  page_id_t page_id_{INVALID_PAGE_ID};
  std::atomic<std::size_t> pin_count_{0};
  bool is_dirty_{false};
  PageBytes data_{};
  mutable std::shared_mutex page_latch_;
};

}  // namespace im110
