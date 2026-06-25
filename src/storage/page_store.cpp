#include "storage/page_store.hpp"

#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <thread>

namespace im110 {

MemoryPageStore::MemoryPageStore(std::chrono::milliseconds latency) : latency_(latency) {}

void MemoryPageStore::ReadPage(page_id_t page_id, std::span<std::byte, PAGE_SIZE> dst) {
  MaybeSleep();
  std::lock_guard guard(mutex_);

  if (deleted_pages_.contains(page_id)) {
    throw std::runtime_error("MemoryPageStore: read of deallocated page id " +
                             std::to_string(page_id));
  }

  std::ranges::fill(dst, std::byte{0});
  const auto iter = pages_.find(page_id);
  if (iter != pages_.end()) {
    std::ranges::copy(iter->second, dst.begin());
  }
}

void MemoryPageStore::WritePage(page_id_t page_id, std::span<const std::byte, PAGE_SIZE> src) {
  MaybeSleep();
  std::lock_guard guard(mutex_);
  PageBytes bytes{};
  std::ranges::copy(src, bytes.begin());
  pages_[page_id] = bytes;
  deleted_pages_.erase(page_id);
}

void MemoryPageStore::DeallocatePage(page_id_t page_id) {
  MaybeSleep();
  std::lock_guard guard(mutex_);
  pages_.erase(page_id);
  deleted_pages_.insert(page_id);
}

auto MemoryPageStore::StoredPageCount() const -> std::size_t {
  std::lock_guard guard(mutex_);
  return pages_.size();
}

void MemoryPageStore::MaybeSleep() const {
  if (latency_.count() > 0) {
    std::this_thread::sleep_for(latency_);
  }
}

FlatFilePageStore::FlatFilePageStore(std::filesystem::path path) : path_(std::move(path)) {
  std::ofstream create(path_, std::ios::binary | std::ios::app);
  if (!create) {
    throw std::runtime_error("could not create page-store file");
  }
}

void FlatFilePageStore::ReadPage(page_id_t page_id, std::span<std::byte, PAGE_SIZE> dst) {
  std::lock_guard guard(mutex_);

  if (deleted_pages_.contains(page_id)) {
    throw std::runtime_error("FlatFilePageStore: read of deallocated page id " +
                             std::to_string(page_id));
  }

  std::ranges::fill(dst, std::byte{0});
  std::ifstream file(path_, std::ios::binary);
  if (!file) {
    throw std::runtime_error("could not open page-store file for reading");
  }

  file.seekg(0, std::ios::end);
  const auto size = static_cast<std::uint64_t>(file.tellg());
  const auto offset = Offset(page_id);
  if (offset >= size) {
    return;
  }

  file.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
  file.read(reinterpret_cast<char*>(dst.data()), static_cast<std::streamsize>(PAGE_SIZE));
}

void FlatFilePageStore::WritePage(page_id_t page_id, std::span<const std::byte, PAGE_SIZE> src) {
  std::lock_guard guard(mutex_);
  std::fstream file(path_, std::ios::binary | std::ios::in | std::ios::out);
  if (!file) {
    throw std::runtime_error("could not open page-store file for writing");
  }

  file.seekp(static_cast<std::streamoff>(Offset(page_id)), std::ios::beg);
  file.write(reinterpret_cast<const char*>(src.data()), static_cast<std::streamsize>(PAGE_SIZE));
  if (!file) {
    throw std::runtime_error("could not write page-store page");
  }
  deleted_pages_.erase(page_id);
}

void FlatFilePageStore::DeallocatePage(page_id_t page_id) {
  std::lock_guard guard(mutex_);
  deleted_pages_.insert(page_id);
}

auto FlatFilePageStore::Path() const -> const std::filesystem::path& { return path_; }

auto FlatFilePageStore::Offset(page_id_t page_id) -> std::uint64_t {
  if (page_id < 0) {
    throw std::invalid_argument("negative page id");
  }
  return static_cast<std::uint64_t>(page_id) * PAGE_SIZE;
}

}  // namespace im110
