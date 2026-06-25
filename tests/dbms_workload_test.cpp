#include <array>
#include <atomic>
#include <cstring>
#include <thread>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "buffer/buffer_pool_manager.hpp"
#include "storage/disk_scheduler.hpp"
#include "storage/page_allocator.hpp"
#include "storage/page_store.hpp"

namespace {

void StoreCounter(std::span<std::byte, im110::PAGE_SIZE> page, std::uint64_t value) {
  std::memcpy(page.data(), &value, sizeof(value));
}

auto LoadCounter(std::span<const std::byte, im110::PAGE_SIZE> page) -> std::uint64_t {
  std::uint64_t value = 0;
  std::memcpy(&value, page.data(), sizeof(value));
  return value;
}

}  // namespace

TEST_CASE("workload: hot pages survive a cold scan by correctness, not by residency guarantee",
          "[workload]") {
  im110::MemoryPageStore store;
  im110::DiskScheduler scheduler{store};
  im110::MonotonicPageAllocator allocator;
  im110::BufferPoolManager bpm{8, scheduler, allocator};

  std::array<im110::page_id_t, 4> hot_pages{};
  for (auto& page_id : hot_pages) {
    const auto allocated = bpm.NewPage();
    REQUIRE(allocated.has_value());
    page_id = *allocated;
  }

  for (std::uint64_t round = 0; round < 20; ++round) {
    for (auto page_id : hot_pages) {
      auto guard = bpm.CheckedWritePage(page_id);
      REQUIRE(guard.has_value());
      StoreCounter(guard->GetDataMut(), round + 1);
    }
  }

  for (int i = 0; i < 40; ++i) {
    const auto cold = bpm.NewPage();
    REQUIRE(cold.has_value());
    auto guard = bpm.CheckedWritePage(*cold);
    REQUIRE(guard.has_value());
    StoreCounter(guard->GetDataMut(), static_cast<std::uint64_t>(1000 + i));
  }

  for (auto page_id : hot_pages) {
    auto guard = bpm.CheckedReadPage(page_id);
    REQUIRE(guard.has_value());
    REQUIRE(LoadCounter(guard->GetData()) == 20);
  }
}

TEST_CASE("workload: concurrent updates to disjoint pages remain durable", "[workload]") {
  im110::MemoryPageStore store;
  im110::DiskScheduler scheduler{store};
  im110::MonotonicPageAllocator allocator;
  im110::BufferPoolManager bpm{6, scheduler, allocator};

  std::vector<im110::page_id_t> pages;
  for (int i = 0; i < 6; ++i) {
    const auto page_id = bpm.NewPage();
    REQUIRE(page_id.has_value());
    pages.push_back(*page_id);
  }

  std::atomic_bool worker_ok{true};
  std::vector<std::thread> workers;
  for (std::size_t i = 0; i < pages.size(); ++i) {
    workers.emplace_back([&, i] {
      for (std::uint64_t value = 1; value <= 50; ++value) {
        auto guard = bpm.CheckedWritePage(pages[i]);
        if (!guard.has_value()) {
          worker_ok = false;
          return;
        }
        StoreCounter(guard->GetDataMut(), (i + 1) * 1000 + value);
      }
    });
  }

  for (auto& worker : workers) {
    worker.join();
  }
  REQUIRE(worker_ok.load());

  bpm.FlushAllPages();

  for (std::size_t i = 0; i < pages.size(); ++i) {
    auto guard = bpm.CheckedReadPage(pages[i]);
    REQUIRE(guard.has_value());
    REQUIRE(LoadCounter(guard->GetData()) == (i + 1) * 1000 + 50);
  }
}

TEST_CASE("workload: concurrent readers and an independent writer leave no pins", "[workload]") {
  im110::MemoryPageStore store;
  im110::DiskScheduler scheduler{store};
  im110::MonotonicPageAllocator allocator;
  im110::BufferPoolManager bpm{4, scheduler, allocator};

  const auto read_page = bpm.NewPage();
  const auto write_page = bpm.NewPage();
  REQUIRE(read_page.has_value());
  REQUIRE(write_page.has_value());

  {
    auto guard = bpm.CheckedWritePage(*read_page);
    REQUIRE(guard.has_value());
    StoreCounter(guard->GetDataMut(), 777);
  }

  std::atomic_bool worker_ok{true};
  std::vector<std::thread> readers;
  for (int thread_id = 0; thread_id < 8; ++thread_id) {
    readers.emplace_back([&] {
      for (int iteration = 0; iteration < 200; ++iteration) {
        auto guard = bpm.CheckedReadPage(*read_page);
        if (!guard.has_value() || LoadCounter(guard->GetData()) != 777) {
          worker_ok = false;
          return;
        }
      }
    });
  }

  std::thread writer([&] {
    for (std::uint64_t value = 1; value <= 200; ++value) {
      auto guard = bpm.CheckedWritePage(*write_page);
      if (!guard.has_value()) {
        worker_ok = false;
        return;
      }
      StoreCounter(guard->GetDataMut(), value);
    }
  });

  for (auto& reader : readers) {
    reader.join();
  }
  writer.join();

  REQUIRE(worker_ok.load());
  REQUIRE(bpm.GetPinCount(*read_page) == 0);
  REQUIRE(bpm.GetPinCount(*write_page) == 0);

  bpm.FlushAllPages();

  {
    auto guard = bpm.CheckedReadPage(*write_page);
    REQUIRE(guard.has_value());
    REQUIRE(LoadCounter(guard->GetData()) == 200);
  }
}
