#pragma once

#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

#include "buffer/arc_replacer.hpp"
#include "common/types.hpp"
#include "storage/disk_scheduler.hpp"
#include "storage/frame_header.hpp"
#include "storage/page_allocator.hpp"
#include "storage/page_guard.hpp"

namespace im110 {

// this donesnt work we need full include
// class ReadPageGuard;
// class WritePageGuard;

class BufferPoolManager {
 public:
  BufferPoolManager(std::size_t num_frames, DiskScheduler& scheduler, PageAllocator& allocator);
  ~BufferPoolManager() = default;

  BufferPoolManager(const BufferPoolManager&) = delete;
  auto operator=(const BufferPoolManager&) -> BufferPoolManager& = delete;

  auto NewPage() -> std::optional<page_id_t>;
  auto DeletePage(page_id_t page_id) -> bool;
  auto CheckedReadPage(page_id_t page_id) -> std::optional<ReadPageGuard>;
  auto CheckedWritePage(page_id_t page_id) -> std::optional<WritePageGuard>;

  // Safe flush variants acquire the BPM latch internally.
  auto FlushPage(page_id_t page_id) -> bool;
  void FlushAllPages();

  // Unsafe flush variants assume the caller already holds the BPM latch.
  // Useful from inside other BPM methods that need to flush a victim while
  // already in a latched critical section.
  auto FlushPageUnsafe(page_id_t page_id) -> bool;
  void FlushAllPagesUnsafe();

  auto GetPinCount(page_id_t page_id) const -> std::size_t;

 private:
  friend class ReadPageGuard;
  friend class WritePageGuard;

  void DropReadGuard(frame_id_t frame_id);
  void DropWriteGuard(frame_id_t frame_id);
  auto FlushFrame(frame_id_t frame_id) -> bool;

  std::size_t num_frames_;
  DiskScheduler& scheduler_;
  PageAllocator& allocator_;
  ArcReplacer replacer_;

  mutable std::mutex latch_;
  std::vector<std::unique_ptr<FrameHeader>> frames_;
  std::deque<frame_id_t> free_frames_;
  std::unordered_map<page_id_t, frame_id_t> page_table_;
};

}  // namespace im110
