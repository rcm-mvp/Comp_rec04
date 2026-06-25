#pragma once

#include <condition_variable>
#include <deque>
#include <mutex>
#include <optional>

namespace im110 {

template <typename T>
class BlockingQueue {
 public:
  BlockingQueue() = default;
  BlockingQueue(const BlockingQueue&) = delete;
  auto operator=(const BlockingQueue&) -> BlockingQueue& = delete;

  void Push(T item) {
    {
      std::lock_guard guard(mutex_);
      if (closed_) {
        return;
      }
      queue_.push_back(std::move(item));
    }
    cv_.notify_one();
  }

  auto Pop() -> std::optional<T> {
    std::unique_lock lock(mutex_);
    cv_.wait(lock, [this] { return closed_ || !queue_.empty(); });

    if (queue_.empty()) {
      return std::nullopt;
    }

    T item = std::move(queue_.front());
    queue_.pop_front();
    return item;
  }

  void Close() {
    {
      std::lock_guard guard(mutex_);
      closed_ = true;
    }
    cv_.notify_all();
  }

 private:
  std::mutex mutex_;
  std::condition_variable cv_;
  std::deque<T> queue_;
  bool closed_{false};
};

}  // namespace im110
