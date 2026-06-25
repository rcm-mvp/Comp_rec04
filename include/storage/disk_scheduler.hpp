#pragma once

#include <future>
#include <memory>
#include <thread>
#include <vector>

#include "common/blocking_queue.hpp"
#include "common/types.hpp"
#include "storage/page_store.hpp"

namespace im110 {

struct DiskRequest {
  enum class Kind { Read, Write, Deallocate };

  Kind kind;
  page_id_t page_id;
  std::byte* data;
  std::promise<bool> completion;
};

class DiskScheduler {
 public:
  explicit DiskScheduler(PageStore& page_store);
  ~DiskScheduler();

  DiskScheduler(const DiskScheduler&) = delete;
  auto operator=(const DiskScheduler&) -> DiskScheduler& = delete;

  // Batch overload. Each request carries its own completion promise; the
  // caller retrieves the futures before handing the requests in. Useful when
  // the implementation can take advantage of seeing several requests at once
  // (prefetching, write coalescing, parallel I/O).
  void Schedule(std::vector<DiskRequest> requests);

  // Single-shot overload. The scheduler creates the completion promise and
  // returns its future. Equivalent to a one-element batch, more ergonomic for
  // the common case.
  auto Schedule(DiskRequest::Kind kind, page_id_t page_id, std::byte* data) -> std::future<bool>;

 private:
  void WorkerLoop();

  PageStore& page_store_;
  BlockingQueue<DiskRequest> queue_;
  std::jthread worker_;
};

}  // namespace im110
