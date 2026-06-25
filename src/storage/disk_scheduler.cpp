#include "storage/disk_scheduler.hpp"

namespace im110 {

DiskScheduler::DiskScheduler(PageStore& page_store) : page_store_(page_store), worker_() {
  // TODO(student): start the worker thread.
}

DiskScheduler::~DiskScheduler() {
  // TODO(student): close the queue and join the worker deterministically.
}

void DiskScheduler::Schedule(std::vector<DiskRequest> requests) {
  (void)requests;
  // TODO(student): move all requests into the blocking queue.
}

auto DiskScheduler::Schedule(DiskRequest::Kind kind, page_id_t page_id, std::byte* data)
    -> std::future<bool> {
  DiskRequest request{.kind = kind, .page_id = page_id, .data = data, .completion = {}};
  auto future = request.completion.get_future();
  std::vector<DiskRequest> batch;
  batch.push_back(std::move(request));
  Schedule(std::move(batch));
  return future;
}

void DiskScheduler::WorkerLoop() {
  (void)page_store_;
  // TODO(student): pop requests until the queue closes and call the page store.
}

}  // namespace im110
