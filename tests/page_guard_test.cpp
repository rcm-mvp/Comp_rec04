#include <catch2/catch_test_macros.hpp>

#include "buffer/buffer_pool_manager.hpp"
#include "storage/disk_scheduler.hpp"
#include "storage/page_allocator.hpp"
#include "storage/page_store.hpp"

TEST_CASE("read guards release pins when dropped", "[guard]") {
  im110::MemoryPageStore store;
  im110::DiskScheduler scheduler{store};
  im110::MonotonicPageAllocator allocator;
  im110::BufferPoolManager bpm{2, scheduler, allocator};

  const auto page_id = bpm.NewPage();
  REQUIRE(page_id.has_value());

  {
    auto guard = bpm.CheckedReadPage(*page_id);
    REQUIRE(guard.has_value());
    REQUIRE(bpm.GetPinCount(*page_id) == 1);
  }

  REQUIRE(bpm.GetPinCount(*page_id) == 0);
}

TEST_CASE("write guards are move-only owners of a page pin", "[guard]") {
  im110::MemoryPageStore store;
  im110::DiskScheduler scheduler{store};
  im110::MonotonicPageAllocator allocator;
  im110::BufferPoolManager bpm{2, scheduler, allocator};

  const auto page_id = bpm.NewPage();
  REQUIRE(page_id.has_value());

  auto guard = bpm.CheckedWritePage(*page_id);
  REQUIRE(guard.has_value());
  REQUIRE(bpm.GetPinCount(*page_id) == 1);

  auto moved = std::move(*guard);
  REQUIRE(moved.IsValid());
  REQUIRE(bpm.GetPinCount(*page_id) == 1);

  moved.Drop();
  moved.Drop();
  REQUIRE(bpm.GetPinCount(*page_id) == 0);
}
