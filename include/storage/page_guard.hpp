#pragma once

#include <cstddef>
#include <mutex>
#include <shared_mutex>
#include <span>

#include "common/types.hpp"
#include "storage/frame_header.hpp"

namespace im110 {

class BufferPoolManager;

class ReadPageGuard {
 public:
  ReadPageGuard() = default;
  ReadPageGuard(BufferPoolManager* bpm, page_id_t page_id, frame_id_t frame_id, FrameHeader* frame);
  ~ReadPageGuard();

  ReadPageGuard(const ReadPageGuard&) = delete;
  auto operator=(const ReadPageGuard&) -> ReadPageGuard& = delete;

  ReadPageGuard(ReadPageGuard&& other) noexcept;
  auto operator=(ReadPageGuard&& other) noexcept -> ReadPageGuard&;

  void Drop();
  auto Flush() -> bool;
  [[nodiscard]] auto PageId() const -> page_id_t;
  [[nodiscard]] auto GetData() const -> std::span<const std::byte, PAGE_SIZE>;
  [[nodiscard]] auto IsValid() const -> bool;

 private:
  BufferPoolManager* bpm_{nullptr};
  page_id_t page_id_{INVALID_PAGE_ID};
  frame_id_t frame_id_{INVALID_FRAME_ID};
  FrameHeader* frame_{nullptr};
  std::shared_lock<std::shared_mutex> latch_;
  bool is_valid_{false};
};

class WritePageGuard {
 public:
  WritePageGuard() = default;
  WritePageGuard(BufferPoolManager* bpm, page_id_t page_id, frame_id_t frame_id, FrameHeader* frame);
  ~WritePageGuard();

  WritePageGuard(const WritePageGuard&) = delete;
  auto operator=(const WritePageGuard&) -> WritePageGuard& = delete;

  WritePageGuard(WritePageGuard&& other) noexcept;
  auto operator=(WritePageGuard&& other) noexcept -> WritePageGuard&;

  void Drop();
  auto Flush() -> bool;
  [[nodiscard]] auto PageId() const -> page_id_t;
  [[nodiscard]] auto GetData() const -> std::span<const std::byte, PAGE_SIZE>;
  [[nodiscard]] auto GetDataMut() -> std::span<std::byte, PAGE_SIZE>;
  [[nodiscard]] auto IsValid() const -> bool;

 private:
  BufferPoolManager* bpm_{nullptr};
  page_id_t page_id_{INVALID_PAGE_ID};
  frame_id_t frame_id_{INVALID_FRAME_ID};
  FrameHeader* frame_{nullptr};
  std::unique_lock<std::shared_mutex> latch_;
  bool is_valid_{false};
};

}  // namespace im110
