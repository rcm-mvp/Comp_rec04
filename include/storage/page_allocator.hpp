#pragma once

#include <mutex>
#include <unordered_set>

#include "common/types.hpp"

namespace im110 {

class PageAllocator {
 public:
  virtual ~PageAllocator() = default;

  virtual auto AllocatePage() -> page_id_t = 0;
  virtual void DeallocatePage(page_id_t page_id) = 0;
  virtual auto IsAllocated(page_id_t page_id) const -> bool = 0;
};

class MonotonicPageAllocator final : public PageAllocator {
 public:
  auto AllocatePage() -> page_id_t override;
  void DeallocatePage(page_id_t page_id) override;
  auto IsAllocated(page_id_t page_id) const -> bool override;

 private:
  mutable std::mutex mutex_;
  page_id_t next_page_id_{0};
  std::unordered_set<page_id_t> allocated_;
};

}  // namespace im110
