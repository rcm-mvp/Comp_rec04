#include <chrono>
#include <future>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "storage/disk_scheduler.hpp"
#include "storage/page_store.hpp"

using namespace std::chrono_literals;

namespace {

auto WaitFor(std::future<bool>& future) -> bool {
  return future.wait_for(1s) == std::future_status::ready && future.get();
}

}  // namespace

TEST_CASE("disk scheduler completes write and read requests", "[disk]") {
  im110::MemoryPageStore store;
  im110::DiskScheduler scheduler{store};

  im110::PageBytes write_buffer{};
  write_buffer[0] = std::byte{0x42};
  write_buffer[17] = std::byte{0x7A};

  im110::DiskRequest write{
      .kind = im110::DiskRequest::Kind::Write,
      .page_id = 4,
      .data = write_buffer.data(),
      .completion = std::promise<bool>{},
  };
  auto write_future = write.completion.get_future();
  std::vector<im110::DiskRequest> writes;
  writes.push_back(std::move(write));
  scheduler.Schedule(std::move(writes));

  REQUIRE(WaitFor(write_future));

  im110::PageBytes read_buffer{};
  im110::DiskRequest read{
      .kind = im110::DiskRequest::Kind::Read,
      .page_id = 4,
      .data = read_buffer.data(),
      .completion = std::promise<bool>{},
  };
  auto read_future = read.completion.get_future();
  std::vector<im110::DiskRequest> reads;
  reads.push_back(std::move(read));
  scheduler.Schedule(std::move(reads));

  REQUIRE(WaitFor(read_future));
  REQUIRE(read_buffer[0] == std::byte{0x42});
  REQUIRE(read_buffer[17] == std::byte{0x7A});
}

TEST_CASE("disk scheduler handles batches in order", "[disk]") {
  im110::MemoryPageStore store;
  im110::DiskScheduler scheduler{store};

  im110::PageBytes first{};
  im110::PageBytes second{};
  first[0] = std::byte{0x01};
  second[0] = std::byte{0x02};

  im110::DiskRequest write_first{
      .kind = im110::DiskRequest::Kind::Write,
      .page_id = 9,
      .data = first.data(),
      .completion = std::promise<bool>{},
  };
  im110::DiskRequest write_second{
      .kind = im110::DiskRequest::Kind::Write,
      .page_id = 9,
      .data = second.data(),
      .completion = std::promise<bool>{},
  };
  auto first_future = write_first.completion.get_future();
  auto second_future = write_second.completion.get_future();

  std::vector<im110::DiskRequest> requests;
  requests.push_back(std::move(write_first));
  requests.push_back(std::move(write_second));
  scheduler.Schedule(std::move(requests));

  REQUIRE(WaitFor(first_future));
  REQUIRE(WaitFor(second_future));

  im110::PageBytes read_back{};
  store.ReadPage(9, std::span<std::byte, im110::PAGE_SIZE>{read_back});
  REQUIRE(read_back[0] == std::byte{0x02});
}
