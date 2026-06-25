#include "storage/page_guard.hpp"

#include <utility>

#include "buffer/buffer_pool_manager.hpp"

namespace im110 {

ReadPageGuard::ReadPageGuard(BufferPoolManager* bpm, page_id_t page_id, frame_id_t frame_id,
                             FrameHeader* frame)
    : bpm_(bpm), page_id_(page_id), frame_id_(frame_id), frame_(frame), is_valid_(true) {
  // TODO(student): acquire the frame's shared page latch.
}

ReadPageGuard::~ReadPageGuard() { Drop(); }

ReadPageGuard::ReadPageGuard(ReadPageGuard&& other) noexcept {
  *this = std::move(other);
}

auto ReadPageGuard::operator=(ReadPageGuard&& other) noexcept -> ReadPageGuard& {
  if (this != &other) {
    Drop();
    bpm_ = std::exchange(other.bpm_, nullptr);
    page_id_ = std::exchange(other.page_id_, INVALID_PAGE_ID);
    frame_id_ = std::exchange(other.frame_id_, INVALID_FRAME_ID);
    frame_ = std::exchange(other.frame_, nullptr);
    latch_ = std::move(other.latch_);
    is_valid_ = std::exchange(other.is_valid_, false);
  }
  return *this;
}

void ReadPageGuard::Drop() {
  if (!is_valid_) {
    return;
  }
  // TODO(student): release the shared page latch and notify the buffer pool.
  is_valid_ = false;
}

auto ReadPageGuard::PageId() const -> page_id_t { return page_id_; }

auto ReadPageGuard::GetData() const -> std::span<const std::byte, PAGE_SIZE> {
  return frame_->GetData();
}

auto ReadPageGuard::Flush() -> bool {
  if (!is_valid_ || bpm_ == nullptr) {
    return false;
  }
  // TODO(student): flush the frame while preserving guard ownership.
  return false;
}

auto ReadPageGuard::IsValid() const -> bool { return is_valid_; }

WritePageGuard::WritePageGuard(BufferPoolManager* bpm, page_id_t page_id, frame_id_t frame_id,
                               FrameHeader* frame)
    : bpm_(bpm), page_id_(page_id), frame_id_(frame_id), frame_(frame), is_valid_(true) {
  // TODO(student): acquire the frame's exclusive page latch.
}

WritePageGuard::~WritePageGuard() { Drop(); }

WritePageGuard::WritePageGuard(WritePageGuard&& other) noexcept {
  *this = std::move(other);
}

auto WritePageGuard::operator=(WritePageGuard&& other) noexcept -> WritePageGuard& {
  if (this != &other) {
    Drop();
    bpm_ = std::exchange(other.bpm_, nullptr);
    page_id_ = std::exchange(other.page_id_, INVALID_PAGE_ID);
    frame_id_ = std::exchange(other.frame_id_, INVALID_FRAME_ID);
    frame_ = std::exchange(other.frame_, nullptr);
    latch_ = std::move(other.latch_);
    is_valid_ = std::exchange(other.is_valid_, false);
  }
  return *this;
}

void WritePageGuard::Drop() {
  if (!is_valid_) {
    return;
  }
  // TODO(student): mark dirty, release the exclusive page latch, and notify the buffer pool.
  is_valid_ = false;
}

auto WritePageGuard::Flush() -> bool {
  if (!is_valid_ || bpm_ == nullptr) {
    return false;
  }
  // TODO(student): flush the frame while preserving guard ownership.
  return false;
}

auto WritePageGuard::PageId() const -> page_id_t { return page_id_; }

auto WritePageGuard::GetData() const -> std::span<const std::byte, PAGE_SIZE> {
  return frame_->GetData();
}

auto WritePageGuard::GetDataMut() -> std::span<std::byte, PAGE_SIZE> {
  return frame_->GetMutableData();
}

auto WritePageGuard::IsValid() const -> bool { return is_valid_; }

}  // namespace im110
