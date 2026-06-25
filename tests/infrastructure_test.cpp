#include <array>
#include <chrono>
#include <filesystem>
#include <future>
#include <thread>

#include <catch2/catch_test_macros.hpp>

#include "common/blocking_queue.hpp"
#include "storage/page_allocator.hpp"
#include "storage/page_store.hpp"

using namespace std::chrono_literals;

TEST_CASE("MemoryPageStore returns zeroes for unwritten pages", "[infra]") {
  im110::MemoryPageStore store;
  im110::PageBytes page{};
  page.fill(std::byte{0xAA});

  store.ReadPage(99, std::span<std::byte, im110::PAGE_SIZE>{page});

  for (auto byte : page) {
    REQUIRE(byte == std::byte{0});
  }
}

TEST_CASE("MemoryPageStore writes, reads, and deallocates pages", "[infra]") {
  im110::MemoryPageStore store;
  im110::PageBytes write{};
  write[0] = std::byte{0x11};
  write[4096] = std::byte{0x22};

  store.WritePage(7, std::span<const std::byte, im110::PAGE_SIZE>{write});

  im110::PageBytes read{};
  store.ReadPage(7, std::span<std::byte, im110::PAGE_SIZE>{read});
  REQUIRE(read[0] == std::byte{0x11});
  REQUIRE(read[4096] == std::byte{0x22});

  store.DeallocatePage(7);
  read.fill(std::byte{0xFF});
  REQUIRE_THROWS_AS(store.ReadPage(7, std::span<std::byte, im110::PAGE_SIZE>{read}),
                    std::runtime_error);

  // Writing a deallocated page id revives it as a fresh page.
  im110::PageBytes rewrite{};
  rewrite[0] = std::byte{0x33};
  store.WritePage(7, std::span<const std::byte, im110::PAGE_SIZE>{rewrite});
  read.fill(std::byte{0xFF});
  store.ReadPage(7, std::span<std::byte, im110::PAGE_SIZE>{read});
  REQUIRE(read[0] == std::byte{0x33});
}

TEST_CASE("FlatFilePageStore persists page bytes across instances", "[infra]") {
  const auto path = std::filesystem::temp_directory_path() / "im110_flat_file_page_store_test.db";
  std::filesystem::remove(path);

  im110::PageBytes write{};
  write[13] = std::byte{0xAB};
  write[8191] = std::byte{0xCD};

  {
    im110::FlatFilePageStore store{path};
    store.WritePage(2, std::span<const std::byte, im110::PAGE_SIZE>{write});
  }

  {
    im110::FlatFilePageStore store{path};
    im110::PageBytes read{};
    store.ReadPage(2, std::span<std::byte, im110::PAGE_SIZE>{read});
    REQUIRE(read[13] == std::byte{0xAB});
    REQUIRE(read[8191] == std::byte{0xCD});
  }

  std::filesystem::remove(path);
}

TEST_CASE("MonotonicPageAllocator tracks allocated pages", "[infra]") {
  im110::MonotonicPageAllocator allocator;

  const auto first = allocator.AllocatePage();
  const auto second = allocator.AllocatePage();

  REQUIRE(first == 0);
  REQUIRE(second == 1);
  REQUIRE(allocator.IsAllocated(first));
  REQUIRE(allocator.IsAllocated(second));

  allocator.DeallocatePage(first);
  REQUIRE_FALSE(allocator.IsAllocated(first));
  REQUIRE(allocator.IsAllocated(second));
}

TEST_CASE("BlockingQueue transfers values and closes deterministically", "[infra]") {
  im110::BlockingQueue<int> queue;
  std::promise<int> observed;
  auto future = observed.get_future();

  std::thread consumer([&] {
    auto value = queue.Pop();
    observed.set_value(value.value_or(-1));
  });

  queue.Push(42);
  REQUIRE(future.wait_for(1s) == std::future_status::ready);
  REQUIRE(future.get() == 42);
  consumer.join();

  queue.Close();
  REQUIRE_FALSE(queue.Pop().has_value());
}
