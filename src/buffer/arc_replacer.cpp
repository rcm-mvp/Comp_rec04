#include "buffer/arc_replacer.hpp"

#include <algorithm>
#include <cassert>

namespace im110 {

ArcReplacer::ArcReplacer(std::size_t num_frames) : replacer_size_(num_frames) {}

void ArcReplacer::RecordAccess(frame_id_t frame_id, page_id_t page_id) {

  // (void)frame_id;
  // (void)page_id;
  // (void)replacer_size_;
  // (void)mru_target_size_;
  // TODO(student): implement ARC state transitions.

  std::lock_guard guard(mutex_);

  // Frame id must be valid.
  assert(frame_id >= 0 && static_cast<std::size_t>(frame_id) < replacer_size_);

  // check if frame already has a entry
  auto fs_it = frame_state_.find(frame_id);

  // 1. Page is in mru_ or mfu_ (true cache hit).
  // Remove the frame from its current resident list and
  // insert it at the front of mfu_. If it was in mru_, remove it from mru_; if it was in mfu_,
  // remove it from mfu_. In both cases, update resident_pos_[frame_id], insert frame_id into
  // mfu_members_, and keep the frame’s evictability flag unchanged. A hit changes replacement
  // order, not pin state.

  if (fs_it != frame_state_.end() && fs_it->second.page_id == page_id) {
    // move frame to front of mfu_
    bool was_in_mfu = mfu_members_.count(frame_id) != 0;
    if (was_in_mfu) {
      mfu_.erase(resident_pos_[frame_id]);
    } else {
      mru_.erase(resident_pos_[frame_id]);
    }
    mfu_.push_front(frame_id);
    resident_pos_[frame_id] = mfu_.begin();
    mfu_members_.insert(frame_id);
    return;
  }

  // If the frame has stale state for a different page, remove it first.
  if (fs_it != frame_state_.end()) {
    bool was_in_mfu = mfu_members_.count(frame_id) != 0;

    if (was_in_mfu) {
      mfu_.erase(resident_pos_[frame_id]);

    } else {
      mru_.erase(resident_pos_[frame_id]);
    }

    resident_pos_.erase(frame_id);
    mfu_members_.erase(frame_id);

    if (fs_it->second.evictable) {
      evictable_count_--;
    }
    frame_state_.erase(fs_it);
  }

  // 2. Page is in mru_ghost_ (pseudo-hit on recency).
  //    Adapt the target upward, then bring the page into mfu_.
  //    - If mru_ghost_.size() >= mfu_ghost_.size(): mru_target_size_ += 1.
  //    - Else: mru_target_size_ += max(mfu_ghost_.size() / mru_ghost_.size(), 1)
  //      (integer division).
  //    - Clamp mru_target_size_ to replacer_size_.
  //    - Remove the entry from mru_ghost_, initialize
  //      frame_state_[frame_id] with page_id and evictable = false, insert
  //      frame_id at the front of mfu_, and update all resident indexes.

  bool found_in_mru_ghost = false;
  std::list<page_id_t>::iterator mru_ghost_iter;

  for (auto it = mru_ghost_.begin(); it != mru_ghost_.end(); ++it) {
    if (*it == page_id) {
      found_in_mru_ghost = true;
      mru_ghost_iter = it;

      break;
    }
  }

  if (found_in_mru_ghost) {

    if (mru_ghost_.size() >= mfu_ghost_.size()) {
      mru_target_size_ += 1;

    } else {
      mru_target_size_ += std::max(mfu_ghost_.size() / mru_ghost_.size(), std::size_t{1});
    }

    mru_target_size_ = std::min(mru_target_size_, replacer_size_);

    mru_ghost_.erase(mru_ghost_iter);
    frame_state_[frame_id] = {page_id, false};
    mfu_.push_front(frame_id);
    resident_pos_[frame_id] = mfu_.begin();
    mfu_members_.insert(frame_id);

    return;
  }

  // 3. Page is in mfu_ghost_ (pseudo-hit on frequency).
  //    Adapt downward, then bring the page into mfu_.
  //    - If mfu_ghost_.size() >= mru_ghost_.size(): mru_target_size_ -= 1.
  //    - Else: mru_target_size_ -= max(mru_ghost_.size() / mfu_ghost_.size(), 1).
  //    - Clamp mru_target_size_ at 0.
  //    - Remove the entry from mfu_ghost_, initialize
  //      frame_state_[frame_id] with page_id and evictable = false, insert
  //      frame_id at the front of mfu_, and update all resident indexes.

  bool found_in_mfu_ghost = false;
  std::list<page_id_t>::iterator mfu_ghost_iter;

  for (auto it = mfu_ghost_.begin(); it != mfu_ghost_.end(); ++it) {
    if (*it == page_id) {
      found_in_mfu_ghost = true;
      mfu_ghost_iter = it;

      break;
    }
  }

  if (found_in_mfu_ghost) {
    if (mfu_ghost_.size() >= mru_ghost_.size()) {
      mru_target_size_ -= 1;

    } else {
      mru_target_size_ -= std::max(mru_ghost_.size() / mfu_ghost_.size(), std::size_t{1});
    }

    mru_target_size_ = std::max(mru_target_size_, std::size_t{0});

    mfu_ghost_.erase(mfu_ghost_iter);
    frame_state_[frame_id] = {page_id, false};
    mfu_.push_front(frame_id);
    resident_pos_[frame_id] = mfu_.begin();
    mfu_members_.insert(frame_id);

    return;
  }

  // 4. Page is in none of the four lists (cold miss).
  //    Make room in the ghost lists first, then add the page to the front of
  //    mru_.
  //    - If mru_.size() + mru_ghost_.size() == replacer_size_:
  //      - If mru_ghost_ is non-empty: evict its tail.
  //      - Else (the entire budget is held by mru_ because mru_ghost_ is
  //        empty): the buffer pool has guaranteed that a frame was already
  //        produced by Evict() on the same call path, so this case does not
  //        arise in practice. A defensive implementation may either treat it
  //        as a no-op or assert.
  //    - Else (the sum is strictly less than replacer_size_):
  //      - If mru_.size() + mru_ghost_.size() + mfu_.size() + mfu_ghost_.size()
  //        == 2 * replacer_size_ and mfu_ghost_ is non-empty: evict the tail
  //        of mfu_ghost_.
  //    - Initialize frame_state_[frame_id] with page_id and evictable =
  //      false, insert frame_id at the front of mru_, and update
  //      resident_pos_[frame_id]. Ensure that frame_id is not in
  //      mfu_members_.

  if (mru_.size() + mru_ghost_.size() == replacer_size_) {
    if (!mru_ghost_.empty()) {
      mru_ghost_.pop_back();
    }

  } else if (mru_.size() + mru_ghost_.size() + mfu_.size() + mfu_ghost_.size() ==
                 2 * replacer_size_ &&
             !mfu_ghost_.empty()) {
    mfu_ghost_.pop_back();
  }

  frame_state_[frame_id] = {page_id, false};
  mru_.push_front(frame_id);
  resident_pos_[frame_id] = mru_.begin();
  mfu_members_.erase(frame_id);
}

void ArcReplacer::SetEvictable(frame_id_t frame_id, bool evictable) {
  // (void)frame_id;
  // (void)evictable;
  // TODO(student): track evictability and Size().

  std::lock_guard guard(mutex_);

  auto it = frame_state_.find(frame_id);
  if (it == frame_state_.end()) {
    return;
  }
  if (it->second.evictable == evictable) {
    return;
  }

  it->second.evictable = evictable;
  if (evictable) {
    evictable_count_++;
  } else {
    evictable_count_--;
  }
}

auto ArcReplacer::Evict() -> std::optional<frame_id_t> {

  // TODO(student): return an evictable victim frame, or std::nullopt.

  std::lock_guard guard(mutex_);

  // which side to evict

  bool prefer_mru = (mru_.size() >= mru_target_size_);

  std::optional<frame_id_t> victim = std::nullopt;

  if (prefer_mru) {

    // Try to find an evictable frame in mru_ (scan from tail = LRU)
    // iterrate forward and keep the last one we find
    // last in forawrd is closest to tail

    frame_id_t mru_victim = INVALID_FRAME_ID;
    bool found_mru = false;

    for (auto it = mru_.begin(); it != mru_.end(); ++it) {
      if (frame_state_[*it].evictable) {
        mru_victim = *it;
        found_mru = true;
      }
    }

    if (found_mru) {
      page_id_t victim_page = frame_state_[mru_victim].page_id;
      mru_.erase(resident_pos_[mru_victim]);
      resident_pos_.erase(mru_victim);
      mfu_members_.erase(mru_victim);
      frame_state_.erase(mru_victim);
      evictable_count_--;
      mru_ghost_.push_front(victim_page);
      victim = mru_victim;

    } else {
      frame_id_t mfu_victim = INVALID_FRAME_ID;
      bool found_mfu = false;

      for (auto it = mfu_.begin(); it != mfu_.end(); ++it) {
        if (frame_state_[*it].evictable) {
          mfu_victim = *it;
          found_mfu = true;
        }
      }

      if (found_mfu) {
        page_id_t victim_page = frame_state_[mfu_victim].page_id;
        mfu_.erase(resident_pos_[mfu_victim]);
        resident_pos_.erase(mfu_victim);
        mfu_members_.erase(mfu_victim);
        frame_state_.erase(mfu_victim);
        evictable_count_--;
        mfu_ghost_.push_front(victim_page);
        victim = mfu_victim;
      }
    }

  } else {
    frame_id_t mfu_victim = INVALID_FRAME_ID;
    bool found_mfu = false;

    for (auto it = mfu_.begin(); it != mfu_.end(); ++it) {
      if (frame_state_[*it].evictable) {
        mfu_victim = *it;
        found_mfu = true;
      }
    }

    if (found_mfu) {
      page_id_t victim_page = frame_state_[mfu_victim].page_id;
      mfu_.erase(resident_pos_[mfu_victim]);
      resident_pos_.erase(mfu_victim);
      mfu_members_.erase(mfu_victim);
      frame_state_.erase(mfu_victim);
      evictable_count_--;
      mfu_ghost_.push_front(victim_page);
      victim = mfu_victim;

    } else {
      // Fallback: try mru_
      frame_id_t mru_victim = INVALID_FRAME_ID;
      bool found_mru = false;

      for (auto it = mru_.begin(); it != mru_.end(); ++it) {
        if (frame_state_[*it].evictable) {
          mru_victim = *it;
          found_mru = true;
        }
      }

      if (found_mru) {
        page_id_t victim_page = frame_state_[mru_victim].page_id;
        mru_.erase(resident_pos_[mru_victim]);
        resident_pos_.erase(mru_victim);
        mfu_members_.erase(mru_victim);
        frame_state_.erase(mru_victim);
        evictable_count_--;
        mru_ghost_.push_front(victim_page);
        victim = mru_victim;
      }
    }
  }

  return victim;
}

void ArcReplacer::Remove(frame_id_t frame_id) {
  // (void)frame_id;
  // TODO(student): remove an evictable frame from resident ARC state.
  std::lock_guard guard(mutex_);

  auto it = frame_state_.find(frame_id);
  if (it == frame_state_.end()) {
    return;
  }

  if (!it->second.evictable) {
    return;
  }

  // Remove from resident list and indexes.
  bool was_in_mfu = mfu_members_.count(frame_id) != 0;
  if (was_in_mfu) {
    mfu_.erase(resident_pos_[frame_id]);

  } else {
    mru_.erase(resident_pos_[frame_id]);
  }

  resident_pos_.erase(frame_id);
  mfu_members_.erase(frame_id);
  frame_state_.erase(it);
  evictable_count_--;
}

auto ArcReplacer::Size() const -> std::size_t {
  // TODO(student): return the number of evictable resident frames.
  std::lock_guard guard(mutex_);
  return evictable_count_;
}

} // namespace im110
