// Performance comparison between the two DiskScheduler::Schedule overloads.
//
// This is an informational benchmark, not a correctness gate. It submits N
// writes through each API to a MemoryPageStore configured with a small
// per-operation latency, then prints the wall-clock time taken.
//
// In the provided single-worker FIFO scheduler, the two APIs should have
// effectively identical throughput: every request is dispatched serially by
// the same worker thread regardless of how it arrived. The single-shot
// overload pays a small per-call overhead (constructing a promise, taking the
// queue mutex once per request); the batch overload amortizes that across the
// vector. The intent of this benchmark is to make that comparison visible and
// to give a future implementation a baseline to optimize against (parallel
// workers, write coalescing, prefetching).

#include <array>
#include <chrono>
#include <cstdio>
#include <future>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "storage/disk_scheduler.hpp"
#include "storage/page_store.hpp"

namespace {

// clock_t  colided with  default clock beacasue of thar rename
using perf_clock_t = std::chrono::steady_clock;

constexpr int kNumRequests = 200;
constexpr auto kStoreLatency = std::chrono::milliseconds{1};

auto MakeBuffers() -> std::vector<im110::PageBytes> {
  std::vector<im110::PageBytes> buffers(kNumRequests);
  for (int i = 0; i < kNumRequests; ++i) {
    buffers[i].fill(static_cast<std::byte>(i & 0xFF));
  }
  return buffers;
}

} // namespace

TEST_CASE("disk scheduler: single-shot vs batched throughput", "[perf]") {
  im110::MemoryPageStore single_store{kStoreLatency};
  im110::MemoryPageStore batched_store{kStoreLatency};
  im110::DiskScheduler single_scheduler{single_store};
  im110::DiskScheduler batched_scheduler{batched_store};

  auto single_buffers = MakeBuffers();
  auto batched_buffers = MakeBuffers();

  // Single-shot path.
  const auto single_start = perf_clock_t::now();
  std::vector<std::future<bool>> single_futures;
  single_futures.reserve(kNumRequests);
  for (int i = 0; i < kNumRequests; ++i) {
    single_futures.push_back(
        single_scheduler.Schedule(im110::DiskRequest::Kind::Write, i, single_buffers[i].data()));
  }
  for (auto& f : single_futures) {
    REQUIRE(f.get());
  }
  const auto single_elapsed = perf_clock_t::now() - single_start;

  // Batched path.
  const auto batched_start = perf_clock_t::now();
  std::vector<im110::DiskRequest> batch;
  batch.reserve(kNumRequests);
  std::vector<std::future<bool>> batched_futures;
  batched_futures.reserve(kNumRequests);
  for (int i = 0; i < kNumRequests; ++i) {
    im110::DiskRequest r{
        .kind = im110::DiskRequest::Kind::Write,
        .page_id = i,
        .data = batched_buffers[i].data(),
        .completion = {},
    };
    batched_futures.push_back(r.completion.get_future());
    batch.push_back(std::move(r));
  }
  batched_scheduler.Schedule(std::move(batch));
  for (auto& f : batched_futures) {
    REQUIRE(f.get());
  }
  const auto batched_elapsed = perf_clock_t::now() - batched_start;

  const auto to_ms = [](auto d) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(d).count();
  };
  std::printf("[perf] single-shot: %lld ms, batched: %lld ms (n=%d, latency=%lldms)\n",
              static_cast<long long>(to_ms(single_elapsed)),
              static_cast<long long>(to_ms(batched_elapsed)), kNumRequests,
              static_cast<long long>(kStoreLatency.count()));

  // Sanity: both stores should now hold every written page.
  REQUIRE(single_store.StoredPageCount() == kNumRequests);
  REQUIRE(batched_store.StoredPageCount() == kNumRequests);
}
