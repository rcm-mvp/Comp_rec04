#include "storage/disk_scheduler.hpp"

namespace im110 {

DiskScheduler::DiskScheduler(PageStore& page_store) : page_store_(page_store), worker_() {
  // TODO(student): start the worker thread.

  worker_ = std::jthread([this] { this->WorkerLoop(); });
}

DiskScheduler::~DiskScheduler() {
  // TODO(student): close the queue and join the worker deterministically.

  queue_.Close();
}

void DiskScheduler::Schedule(std::vector<DiskRequest> requests) {
  // TODO(student): move all requests into the blocking queue.

  for (auto& req : requests) {
    queue_.Push(std::move(req));
  }
}

auto DiskScheduler::Schedule(DiskRequest::Kind kind, page_id_t page_id,
                             std::byte* data) -> std::future<bool> {
  DiskRequest request{.kind = kind, .page_id = page_id, .data = data, .completion = {}};
  auto future = request.completion.get_future();
  std::vector<DiskRequest> batch;
  batch.push_back(std::move(request));
  Schedule(std::move(batch));
  return future;
}

void DiskScheduler::WorkerLoop() {
  // TODO(student): pop requests until the queue closes and call the page store.

  while (true) {
    auto request = queue_.Pop();

    if (!request.has_value()) {
      return;
    }

    DiskRequest& req = request.value();
    bool success = true;

    try {
      switch (req.kind) {
      case DiskRequest::Kind::Read:
        page_store_.ReadPage(req.page_id, std::span<std::byte, PAGE_SIZE>(req.data, PAGE_SIZE));
        break;
      case DiskRequest::Kind::Write:
        page_store_.WritePage(req.page_id,
                              std::span<const std::byte, PAGE_SIZE>(req.data, PAGE_SIZE));
        break;
      case DiskRequest::Kind::Deallocate:
        page_store_.DeallocatePage(req.page_id);
        break;
      }

    } catch (...) {
      success = false;
    }

    req.completion.set_value(success);
  }
}

} // namespace im110
