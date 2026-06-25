#include "buffer/arc_replacer.hpp"

namespace im110 {

ArcReplacer::ArcReplacer(std::size_t num_frames) : replacer_size_(num_frames) {}

void ArcReplacer::RecordAccess(frame_id_t frame_id, page_id_t page_id) {
  (void)frame_id;
  (void)page_id;
  (void)replacer_size_;
  (void)mru_target_size_;
  // TODO(student): implement ARC state transitions.
}

void ArcReplacer::SetEvictable(frame_id_t frame_id, bool evictable) {
  (void)frame_id;
  (void)evictable;
  // TODO(student): track evictability and Size().
}

auto ArcReplacer::Evict() -> std::optional<frame_id_t> {
  // TODO(student): return an evictable victim frame, or std::nullopt.
  return std::nullopt;
}

void ArcReplacer::Remove(frame_id_t frame_id) {
  (void)frame_id;
  // TODO(student): remove an evictable frame from resident ARC state.
}

auto ArcReplacer::Size() const -> std::size_t {
  // TODO(student): return the number of evictable resident frames.
  std::lock_guard guard(mutex_);
  return evictable_count_;
}

}  // namespace im110
