#include <cstring>

#include <catch2/catch_test_macros.hpp>

#include "buffer/buffer_pool_manager.hpp"
#include "storage/disk_scheduler.hpp"
#include "storage/page_allocator.hpp"
#include "storage/page_store.hpp"

namespace {

void WriteInt(std::span<std::byte, im110::PAGE_SIZE> page, std::uint64_t value) {
  std::memcpy(page.data(), &value, sizeof(value));
}

auto ReadInt(std::span<const std::byte, im110::PAGE_SIZE> page) -> std::uint64_t {
  std::uint64_t value = 0;
  std::memcpy(&value, page.data(), sizeof(value));
  return value;
}

}  // namespace

TEST_CASE("buffer pool persists dirty pages through eviction", "[bpm]") {
  im110::MemoryPageStore store;
  im110::DiskScheduler scheduler{store};
  im110::MonotonicPageAllocator allocator;
  im110::BufferPoolManager bpm{1, scheduler, allocator};

  const auto first = bpm.NewPage();
  REQUIRE(first.has_value());

  {
    auto guard = bpm.CheckedWritePage(*first);
    REQUIRE(guard.has_value());
    WriteInt(guard->GetDataMut(), 1234);
  }

  const auto second = bpm.NewPage();
  REQUIRE(second.has_value());
  REQUIRE(*second != *first);

  {
    auto guard = bpm.CheckedReadPage(*first);
    REQUIRE(guard.has_value());
    REQUIRE(ReadInt(guard->GetData()) == 1234);
  }
}

TEST_CASE("buffer pool reports failure when every frame is pinned", "[bpm]") {
  im110::MemoryPageStore store;
  im110::DiskScheduler scheduler{store};
  im110::MonotonicPageAllocator allocator;
  im110::BufferPoolManager bpm{1, scheduler, allocator};

  const auto first = bpm.NewPage();
  REQUIRE(first.has_value());

  auto guard = bpm.CheckedReadPage(*first);
  REQUIRE(guard.has_value());
  REQUIRE_FALSE(bpm.NewPage().has_value());
}

TEST_CASE("delete fails for pinned resident pages", "[bpm]") {
  im110::MemoryPageStore store;
  im110::DiskScheduler scheduler{store};
  im110::MonotonicPageAllocator allocator;
  im110::BufferPoolManager bpm{2, scheduler, allocator};

  const auto page_id = bpm.NewPage();
  REQUIRE(page_id.has_value());

  auto guard = bpm.CheckedReadPage(*page_id);
  REQUIRE(guard.has_value());
  REQUIRE_FALSE(bpm.DeletePage(*page_id));
}
