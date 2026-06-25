#pragma once

#include <chrono>
#include <filesystem>
#include <mutex>
#include <span>
#include <unordered_map>
#include <unordered_set>

#include "common/types.hpp"

namespace im110 {

// PageStore is the durable page-id-to-bytes layer.
//
// Contract:
//   * ReadPage(p, dst)
//       - If p was previously written and not since deallocated, dst is filled
//         with the most recent bytes written.
//       - If p was never written, dst is zero-filled.
//       - If p was previously deallocated and not since rewritten, throws
//         std::runtime_error.
//   * WritePage(p, src) records src as the current bytes for p. If p had been
//     deallocated, it becomes live again with the new contents.
//   * DeallocatePage(p) marks p as deallocated. Subsequent reads of p throw
//     until p is written again.
//
// The contract above applies within a single PageStore object's lifetime.
// FlatFilePageStore keeps the deallocation set in memory only, so tombstones
// are not preserved across object reconstruction; this is consistent with the
// fact that the flat file format has no on-disk metadata or free list.
class PageStore {
 public:
  virtual ~PageStore() = default;

  virtual void ReadPage(page_id_t page_id, std::span<std::byte, PAGE_SIZE> dst) = 0;
  virtual void WritePage(page_id_t page_id, std::span<const std::byte, PAGE_SIZE> src) = 0;
  virtual void DeallocatePage(page_id_t page_id) = 0;
};

class MemoryPageStore final : public PageStore {
 public:
  explicit MemoryPageStore(std::chrono::milliseconds latency = std::chrono::milliseconds{0});

  void ReadPage(page_id_t page_id, std::span<std::byte, PAGE_SIZE> dst) override;
  void WritePage(page_id_t page_id, std::span<const std::byte, PAGE_SIZE> src) override;
  void DeallocatePage(page_id_t page_id) override;

  [[nodiscard]] auto StoredPageCount() const -> std::size_t;

 private:
  void MaybeSleep() const;

  mutable std::mutex mutex_;
  std::unordered_map<page_id_t, PageBytes> pages_;
  std::unordered_set<page_id_t> deleted_pages_;
  std::chrono::milliseconds latency_;
};

class FlatFilePageStore final : public PageStore {
 public:
  explicit FlatFilePageStore(std::filesystem::path path);

  void ReadPage(page_id_t page_id, std::span<std::byte, PAGE_SIZE> dst) override;
  void WritePage(page_id_t page_id, std::span<const std::byte, PAGE_SIZE> src) override;
  void DeallocatePage(page_id_t page_id) override;

  [[nodiscard]] auto Path() const -> const std::filesystem::path&;

 private:
  static auto Offset(page_id_t page_id) -> std::uint64_t;

  mutable std::mutex mutex_;
  std::filesystem::path path_;
  std::unordered_set<page_id_t> deleted_pages_;
};

}  // namespace im110
