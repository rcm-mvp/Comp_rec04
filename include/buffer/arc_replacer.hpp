#pragma once

#include <cstddef>
#include <list>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <unordered_set>

#include "common/types.hpp"

namespace im110 {

class ArcReplacer {
 public:
  explicit ArcReplacer(std::size_t num_frames);

  ArcReplacer(const ArcReplacer&) = delete;
  auto operator=(const ArcReplacer&) -> ArcReplacer& = delete;

  void RecordAccess(frame_id_t frame_id, page_id_t page_id);
  void SetEvictable(frame_id_t frame_id, bool evictable);
  auto Evict() -> std::optional<frame_id_t>;
  void Remove(frame_id_t frame_id);
  auto Size() const -> std::size_t;

 private:
  struct FrameState {
    page_id_t page_id{INVALID_PAGE_ID};
    bool evictable{false};
  };

  // Students may replace these fields if their implementation preserves the public contract.
  std::size_t replacer_size_;
  mutable std::mutex mutex_;
  std::size_t evictable_count_{0};
  std::size_t mru_target_size_{0};

  std::list<frame_id_t> mru_;
  std::list<frame_id_t> mfu_;
  std::list<page_id_t> mru_ghost_;
  std::list<page_id_t> mfu_ghost_;

  std::unordered_map<frame_id_t, FrameState> frame_state_;
  std::unordered_map<frame_id_t, std::list<frame_id_t>::iterator> resident_pos_;
  std::unordered_set<frame_id_t> mfu_members_;
};

}  // namespace im110
