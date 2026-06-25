#include "buffer/buffer_pool_manager.hpp"

#include "storage/page_guard.hpp"

namespace im110 {

BufferPoolManager::BufferPoolManager(std::size_t num_frames, DiskScheduler& scheduler,
                                     PageAllocator& allocator)
    : num_frames_(num_frames), scheduler_(scheduler), allocator_(allocator), replacer_(num_frames) {
  frames_.reserve(num_frames_);
  for (std::size_t i = 0; i < num_frames_; ++i) {
    frames_.push_back(std::make_unique<FrameHeader>());
    free_frames_.push_back(static_cast<frame_id_t>(i));
  }
}

auto BufferPoolManager::NewPage() -> std::optional<page_id_t> {
  // TODO(student): allocate a page id, acquire a frame, install a zeroed page, and return the id.
  return std::nullopt;
}

auto BufferPoolManager::DeletePage(page_id_t page_id) -> bool {
  (void)page_id;
  // TODO(student): fail if pinned; otherwise remove from the buffer pool and deallocate.
  return false;
}

auto BufferPoolManager::CheckedReadPage(page_id_t page_id) -> std::optional<ReadPageGuard> {
  (void)page_id;
  // TODO(student): fetch page, pin it, and return a read guard.
  return std::nullopt;
}

auto BufferPoolManager::CheckedWritePage(page_id_t page_id) -> std::optional<WritePageGuard> {
  (void)page_id;
  // TODO(student): fetch page, pin it, and return a write guard.
  return std::nullopt;
}

auto BufferPoolManager::FlushPage(page_id_t page_id) -> bool {
  std::lock_guard guard(latch_);
  return FlushPageUnsafe(page_id);
}

void BufferPoolManager::FlushAllPages() {
  std::lock_guard guard(latch_);
  FlushAllPagesUnsafe();
}

auto BufferPoolManager::FlushPageUnsafe(page_id_t page_id) -> bool {
  (void)page_id;
  // TODO(student): write the page if resident and dirty. Caller must hold latch_.
  return false;
}

void BufferPoolManager::FlushAllPagesUnsafe() {
  // TODO(student): flush every resident dirty page. Caller must hold latch_.
}

auto BufferPoolManager::GetPinCount(page_id_t page_id) const -> std::size_t {
  (void)page_id;
  // TODO(student): return the pin count for resident pages; return 0 for non-resident pages.
  return 0;
}

void BufferPoolManager::DropReadGuard(frame_id_t frame_id) {
  (void)frame_id;
  // TODO(student): decrement pin count and update replacer evictability.
}

void BufferPoolManager::DropWriteGuard(frame_id_t frame_id) {
  (void)frame_id;
  // TODO(student): mark dirty, decrement pin count, and update replacer evictability.
}

auto BufferPoolManager::FlushFrame(frame_id_t frame_id) -> bool {
  (void)frame_id;
  // TODO(student): schedule a write for the frame's current page.
  return false;
}

}  // namespace im110
