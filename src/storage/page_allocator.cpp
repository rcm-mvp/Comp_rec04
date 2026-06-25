#include "storage/page_allocator.hpp"

namespace im110 {

auto MonotonicPageAllocator::AllocatePage() -> page_id_t {
  std::lock_guard guard(mutex_);
  const auto page_id = next_page_id_++;
  allocated_.insert(page_id);
  return page_id;
}

void MonotonicPageAllocator::DeallocatePage(page_id_t page_id) {
  std::lock_guard guard(mutex_);
  allocated_.erase(page_id);
}

auto MonotonicPageAllocator::IsAllocated(page_id_t page_id) const -> bool {
  std::lock_guard guard(mutex_);
  return allocated_.contains(page_id);
}

}  // namespace im110
